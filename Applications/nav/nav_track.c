/**
 * @file nav_track.c
 * @brief 灰度条巡线模块源文件
 *
 * 通过8路灰度传感器（TRACK模块）的数字量数据计算巡线偏差，
 * 使用PID控制将偏差映射为转向角速度，配合前进速度实现沿灰度条巡线。
 *
 * 控制约定：
 * - 传感器bit7~bit0对应探头1~8（自左向右排列）
 * - 偏差error范围约[-3.5, 3.5]，正值表示线在车体右侧
 * - 正角速度 = 左转（与运动学解算一致），因此线在右侧时输出负角速度
 */

#include "nav_track.h"
#include "track.h"
#include "motion_control.h"
#include "nav_math.h"
#include "zdt_v5_cfg.h"
#include "Events.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "log.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include <string.h>
#include <stdlib.h>
#include "nav_track_cfg.h"

#if !MOTOR_VELOCITY_MODE
    #error "MOTOR_VELOCITY_MODE must be enabled for line following"
#endif

/**
 * @brief 巡线控制结构体
 */
typedef struct {
    NavTrackState_t state;          // 巡线状态
    NavTrackParams_t params;        // 巡线参数
    float error;                    // 当前偏差（原始）
    float error_smooth;             // 滤波后的偏差（用于PID）
    float last_error;               // 上一次偏差
    float integral;                 // 偏差积分项
    uint32_t last_tick;             // 上一次控制时间戳
    uint32_t run_start_tick;        // 巡线开始时间戳
    uint32_t lost_start_tick;       // 开始丢线时间戳
    bool lost_flag;                 // 丢线标志
    bool smooth_initialized;        // 滤波是否已初始化（首帧直接取原始值，避免微分突变）
    /* 弯道自动检测状态（跑道形状：直线+半圆交替） */
    float error_avg;                // error 慢速滤波（用于检测稳态偏置）
    bool curve_active;              // 是否处于弯道状态
    int8_t curve_dir;               // 当前弯道逻辑方向：1=需右转(error_avg>0), -1=需左转(error_avg<0)
    uint32_t curve_enter_tick;      // 进入弯道计时
    uint32_t curve_exit_tick;       // 退出弯道计时
} NavTrack_t;

static NavTrack_t *g_nav_track = NULL;
static bool is_init = false;

/* 传感器权重：边缘探头权重更大，使偏差过渡更平滑 */
static const float k_sensor_weight[NAV_TRACK_SENSOR_NUM] = {
    8.0f, 3.0f, 2.0f, 1.0f, 1.0f, 2.0f, 3.0f, 8.0f
};

/* 静态函数声明 */
static bool compute_error(uint8_t digital_data, float *error);
static float pid_compute(float error, float dt);
static void ntrack_show_status(void);

/**
 * @brief 根据数字量计算巡线偏差
 * @param digital_data 数字量（bit7~bit0对应探头1~8）
 * @param error 输出偏差（正=线在右）
 * @return 是否有效（false表示丢线）
 */
static bool compute_error(uint8_t digital_data, float *error) {
    float weighted = 0.0f;
    float weight_sum = 0.0f;

    for (uint8_t i = 0; i < NAV_TRACK_SENSOR_NUM; i++) {
        uint8_t on_line = (digital_data >> (7 - i)) & 0x01;
        if (g_nav_track->params.line_polarity == 0) {
            on_line = !on_line;
        }

        if (on_line) {
            weighted += k_sensor_weight[i] * (float)i;
            weight_sum += k_sensor_weight[i];
        }
    }

    if (weight_sum < 0.001f) {
        return false;   // 所有探头均未检测到线
    }

    *error = weighted / weight_sum - (NAV_TRACK_SENSOR_NUM - 1) / 2.0f;
    return true;
}

/**
 * @brief PID偏差控制器
 * @param error 当前偏差
 * @param dt 控制周期 (s)
 * @return 转向角速度（正值=左转）
 */
static float pid_compute(float error, float dt) {
    if (dt <= 0.0f) dt = 0.01f;

    g_nav_track->integral += error * dt;
    float integral_limit = g_nav_track->params.max_yaw_speed /
                           ((g_nav_track->params.ki > 0.001f) ? g_nav_track->params.ki : 1.0f);
    g_nav_track->integral = clamp(g_nav_track->integral, -integral_limit, integral_limit);

    float derivative = (error - g_nav_track->last_error) / dt;
    g_nav_track->last_error = error;

    /* 微分项限幅：防止误差快速反向时转向猛打（转弯摆头根源） */
    float d_term = g_nav_track->params.kd * derivative;
    float d_max = g_nav_track->params.max_yaw_speed * NAV_TRACK_DERIV_LIMIT_RATIO;
    d_term = clamp(d_term, -d_max, d_max);

    float output = g_nav_track->params.kp * error
                 + g_nav_track->params.ki * g_nav_track->integral
                 + d_term;
    output = clamp(output, -g_nav_track->params.max_yaw_speed, g_nav_track->params.max_yaw_speed);
    return output;
}

/**
 * @brief 初始化巡线模块
 * @return 初始化状态
 */
bool Nav_Track_Init(void) {
    g_nav_track = pvPortMalloc(sizeof(NavTrack_t));
    if (g_nav_track == NULL) {
        return false;
    }
    memset(g_nav_track, 0, sizeof(NavTrack_t));

    g_nav_track->state = NAV_TRACK_STATE_IDLE;
    g_nav_track->params.forward_speed = NAV_TRACK_FORWARD_SPEED;      // 前进速度 (cm/s)
    g_nav_track->params.kp = NAV_TRACK_KP;                 // P系数
    g_nav_track->params.ki = NAV_TRACK_KI;                  // I系数
    g_nav_track->params.kd = NAV_TRACK_KD;                  // D系数
    g_nav_track->params.max_yaw_speed = NAV_TRACK_MAX_YAW_SPEED;     // 最大转向速度 (deg/s)
    g_nav_track->params.line_polarity = NAV_TRACK_LINE_POLARITY_POSITIVE;         // 线极性 (1:正, 0:负)
    g_nav_track->params.steering_dir = NAV_TRACK_STEERING_DIR_LEFT;                 // 转向方向 (1:左转, -1:右转)
    g_nav_track->params.run_time_ms = NAV_TRACK_RUN_TIME_MS;     // 巡线运行时间 (ms)
    g_nav_track->params.path_radius_cm = NAV_TRACK_PATH_RADIUS_CM; // 路径半径(cm)，0=禁用前馈

    is_init = true;
    return true;
}

/**
 * @brief 开始巡线
 * @return 启动状态
 */
bool Nav_Track_Start(void) {
    if (!is_init) return false;

    /* 确保灰度模块处于数字量输出模式 */
    if (!Track_SetDigitalMode()) return false;

    g_nav_track->state = NAV_TRACK_STATE_RUNNING;
    g_nav_track->run_start_tick = osKernelGetTickCount();
    g_nav_track->last_tick = osKernelGetTickCount();
    g_nav_track->error = 0.0f;
    g_nav_track->error_smooth = 0.0f;
    g_nav_track->last_error = 0.0f;
    g_nav_track->integral = 0.0f;
    g_nav_track->lost_flag = false;
    g_nav_track->lost_start_tick = 0;
    g_nav_track->smooth_initialized = false;
    g_nav_track->error_avg = 0.0f;
    g_nav_track->curve_active = false;
    g_nav_track->curve_dir = 0;
    g_nav_track->curve_enter_tick = 0;
    g_nav_track->curve_exit_tick = 0;

    MotionControl_SetMotionParams(g_nav_track->params.forward_speed,
                                   g_nav_track->params.max_yaw_speed,
                                   80.0f, 100.0f);

    logInfo("Nav track started, speed: %.1f cm/s, kp: %.2f, ki: %.3f, kd: %.2f",
            g_nav_track->params.forward_speed,
            g_nav_track->params.kp,
            g_nav_track->params.ki,
            g_nav_track->params.kd);
    return true;
}

/**
 * @brief 停止巡线
 */
void Nav_Track_Stop(void) {
    if (!is_init) return;

    MotionControl_Stop();
    g_nav_track->state = NAV_TRACK_STATE_IDLE;
    g_nav_track->lost_flag = false;
    g_nav_track->lost_start_tick = 0;
}

/**
 * @brief 获取巡线状态
 * @return 巡线状态
 */
NavTrackState_t Nav_Track_GetState(void) {
    return g_nav_track->state;
}

/**
 * @brief 获取巡线参数
 * @param params 输出参数结构体指针
 */
void Nav_Track_GetParams(NavTrackParams_t *params) {
    if (!is_init || params == NULL) return;
    *params = g_nav_track->params;
}

/**
 * @brief 设置巡线参数
 * @param params 参数结构体指针
 */
void Nav_Track_SetParams(const NavTrackParams_t *params) {
    if (!is_init || params == NULL) return;
    g_nav_track->params = *params;
}

/**
 * @brief 巡线任务
 *
 * 主循环周期：NAV_TRACK_UPDATE_MS
 * 流程：读取灰度数据 → 计算偏差 → PID控制 → 输出速度
 */
void Nav_Track_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!is_init) vTaskDelete(NULL);

    TickType_t last_wake_time = xTaskGetTickCount();
    TrackData_t track_data;
    uint32_t last_dbg_tick = 0;

    for (;;) {
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(NAV_TRACK_UPDATE_MS));

        if (g_nav_track->state != NAV_TRACK_STATE_RUNNING) {
            g_nav_track->last_tick = osKernelGetTickCount();
            continue;
        }

        uint32_t current_tick = osKernelGetTickCount();
        float dt = (current_tick - g_nav_track->last_tick) / 1000.0f;
        g_nav_track->last_tick = current_tick;

        /* 调试日志节流：每100ms打印一次，避免刷屏 */
        bool dbg_allow = (current_tick - last_dbg_tick) >= 100;
        if (dbg_allow) last_dbg_tick = current_tick;

        /* 运行时长检测 */
        if (g_nav_track->params.run_time_ms > 0 &&
            (current_tick - g_nav_track->run_start_tick) >= g_nav_track->params.run_time_ms) {
            Nav_Track_Stop();
            logInfo("Nav track finished by timeout");
            continue;
        }

        if (!Track_GetData(&track_data)) {
            continue;
        }

        float error;
        if (compute_error(track_data.digitalData, &error)) {
            bool was_lost = g_nav_track->lost_flag;
            g_nav_track->error = error;
            g_nav_track->lost_flag = false;
            g_nav_track->lost_start_tick = 0;

            /* 启动/丢线后首帧：直接取原始值初始化滤波与微分基准，避免微分突变 */
            if (was_lost) {
                g_nav_track->integral = 0.0f;
                g_nav_track->error_smooth = error;
                g_nav_track->last_error = error;
                g_nav_track->smooth_initialized = true;
            } else if (!g_nav_track->smooth_initialized) {
                g_nav_track->error_smooth = error;
                g_nav_track->last_error = error;
                g_nav_track->smooth_initialized = true;
            } else {
                /* 偏差低通滤波：消除1bit数字量在相邻探头间的抖动 */
                g_nav_track->error_smooth = g_nav_track->error_smooth * (1.0f - NAV_TRACK_ERROR_ALPHA) +
                                            error * NAV_TRACK_ERROR_ALPHA;
            }
            error = g_nav_track->error_smooth;

            /* 路径曲率前馈：按 error 稳态偏置自动检测直线/弯道
             * 直线段 error_avg≈0 → 前馈=0，行为与原版一致；
             * 半圆段 error_avg 持续偏一侧 → 前馈提供 v/R 稳态转向，PID 仅补小偏差。
             * 跑道两端半圆方向相反，由 error_avg 符号自动区分。 */
            float ff_yaw = 0.0f;
            if (g_nav_track->params.path_radius_cm > 1.0f) {
                /* error 慢速滤波（首帧直接取原始值，避免从0慢慢爬） */
                if (!g_nav_track->smooth_initialized ||
                    fabsf(g_nav_track->error_avg) < 0.001f) {
                    g_nav_track->error_avg = error;
                } else {
                    g_nav_track->error_avg = g_nav_track->error_avg *
                                              (1.0f - NAV_TRACK_CURVE_DETECT_ALPHA) +
                                              error * NAV_TRACK_CURVE_DETECT_ALPHA;
                }

                if (!g_nav_track->curve_active) {
                    /* 检测进入弯道：|error_avg| 超阈值持续确认时间 */
                    if (fabsf(g_nav_track->error_avg) > NAV_TRACK_CURVE_ENTER_THRESHOLD) {
                        if (g_nav_track->curve_enter_tick == 0) {
                            g_nav_track->curve_enter_tick = current_tick;
                        } else if ((current_tick - g_nav_track->curve_enter_tick) >=
                                   NAV_TRACK_CURVE_CONFIRM_MS) {
                            g_nav_track->curve_active = true;
                            g_nav_track->curve_dir = (g_nav_track->error_avg > 0) ? 1 : -1;
                            g_nav_track->curve_exit_tick = 0;
                            logInfo("Curve entered, dir=%d (1=R, -1=L)", g_nav_track->curve_dir);
                        }
                    } else {
                        g_nav_track->curve_enter_tick = 0;
                    }
                } else {
                    /* 检测退出弯道：|error_avg| 低于阈值持续确认时间 */
                    if (fabsf(g_nav_track->error_avg) < NAV_TRACK_CURVE_EXIT_THRESHOLD) {
                        if (g_nav_track->curve_exit_tick == 0) {
                            g_nav_track->curve_exit_tick = current_tick;
                        } else if ((current_tick - g_nav_track->curve_exit_tick) >=
                                   NAV_TRACK_CURVE_CONFIRM_MS) {
                            g_nav_track->curve_active = false;
                            g_nav_track->curve_enter_tick = 0;
                            logInfo("Curve exited");
                        }
                    } else {
                        g_nav_track->curve_exit_tick = 0;
                    }
                    /* 弯道中：前馈方向锁死为进入时的方向，不随瞬时 error 翻转
                     * 半圆是连续转弯，PID 过冲导致 err 短暂反号是正常的，
                     * 不能据此翻转前馈方向（否则雪上加霜）。
                     * 跑道两端方向相反由"退出再进入"自动处理：avg 回零→退出→下次进入重新判断 */
                    float ff_mag = (g_nav_track->params.forward_speed /
                                    g_nav_track->params.path_radius_cm) * RAD_TO_DEG;
                    ff_yaw = -(float)g_nav_track->curve_dir * ff_mag *
                             g_nav_track->params.steering_dir;
                    ff_yaw = clamp(ff_yaw, -g_nav_track->params.max_yaw_speed,
                                   g_nav_track->params.max_yaw_speed);
                }
            }

            /* 转向死区：接近居中时仅输出前馈，防止蛇形（仍更新微分基准） */
            if (fabsf(error) < NAV_TRACK_YAW_DEADBAND) {
                g_nav_track->last_error = error;
                MotionControl_SetVelocity(g_nav_track->params.forward_speed, 0.0f, ff_yaw);
                if (dbg_allow) logDebug("ntrack err: %.2f, yaw: %.1f (ff:%.1f), avg:%.2f",
                                        error, ff_yaw, ff_yaw, g_nav_track->error_avg);
                continue;
            }

            float pid_yaw = pid_compute(error, dt);
            /* 线在右(误差为正)时右转(负角速度)，与丢线搜索方向保持一致 */
            float yaw_speed = -pid_yaw * g_nav_track->params.steering_dir + ff_yaw;
            yaw_speed = clamp(yaw_speed, -g_nav_track->params.max_yaw_speed,
                              g_nav_track->params.max_yaw_speed);

            /* 速度-转向协调：转向需求大时自动减速，提高弯道转弯能力
             * 注意：steer_ratio 只看 PID 部分，前馈不算"过冲"，避免稳态前馈持续减速 */
            float steer_ratio = fabsf(-pid_yaw * g_nav_track->params.steering_dir) /
                                g_nav_track->params.max_yaw_speed;
            float vx = g_nav_track->params.forward_speed *
                       (1.0f - NAV_TRACK_SPEED_REDUCE_RATIO * steer_ratio);
            vx = clamp(vx, g_nav_track->params.forward_speed * 0.35f,
                       g_nav_track->params.forward_speed);

            MotionControl_SetVelocity(vx, 0.0f, yaw_speed);

            if (dbg_allow) logDebug("ntrack err: %.2f, yaw: %.1f (ff:%.1f), vx: %.1f, avg:%.2f",
                                     error, yaw_speed, ff_yaw, vx, g_nav_track->error_avg);
        } else {
            /* 丢线：原地转向搜索 */
            if (!g_nav_track->lost_flag) {
                g_nav_track->lost_flag = true;
                g_nav_track->lost_start_tick = current_tick;
            }

            if ((current_tick - g_nav_track->lost_start_tick) >= NAV_TRACK_LOST_TIMEOUT_MS) {
                Nav_Track_Stop();
                logWarning("Nav track lost line too long, stopped");
                continue;
            }

            float search_dir = (g_nav_track->error >= 0.0f) ? -1.0f : 1.0f;
            float search_yaw = search_dir * g_nav_track->params.max_yaw_speed *
                               NAV_TRACK_SEARCH_YAW_RATIO * g_nav_track->params.steering_dir;
            MotionControl_SetVelocity(0.0f, 0.0f, search_yaw);
        }
    }
}

/**
 * @brief 显示巡线状态与参数
 */
static void ntrack_show_status(void) {
    const char *state_str = (g_nav_track->state == NAV_TRACK_STATE_RUNNING) ? "Running" : "Idle";
    logPrintln("State: %s\r\n"
               "Speed: %.1f cm/s, MaxYaw: %.1f deg/s\r\n"
               "PID: kp=%.2f, ki=%.3f, kd=%.2f\r\n"
               "Polarity: %d, Dir: %d, RunTime: %u ms\r\n"
               "Error: %.2f",
               state_str,
               g_nav_track->params.forward_speed,
               g_nav_track->params.max_yaw_speed,
               g_nav_track->params.kp,
               g_nav_track->params.ki,
               g_nav_track->params.kd,
               g_nav_track->params.line_polarity,
               g_nav_track->params.steering_dir,
               g_nav_track->params.run_time_ms,
               g_nav_track->error);
}

/**
 * @brief 解析浮点参数
 * @param str 参数字符串
 * @param out 输出浮点值
 * @return 是否解析成功
 */
static bool parse_float(const char *str, float *out) {
    char *endptr = NULL;
    *out = strtof(str, &endptr);
    return (endptr != str && *endptr == '\0');
}

/* Shell 子命令 */
static void ntrack_start_shell(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (!Nav_Track_Start()) logWarning("Nav track start failed");
}

static void ntrack_stop_shell(int argc, char *argv[]) {
    (void)argc; (void)argv;
    Nav_Track_Stop();
    logPrintln("Nav track stopped");
}

static void ntrack_sta_shell(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    ntrack_show_status();
}

/**
 * @brief 实时调试视图：显示探头状态、偏差与丢线状态
 */
static void ntrack_view_shell(int argc, char *argv[]) {
    (void)argc; (void)argv;
    if (!is_init) {
        logWarning("Nav track module not initialized");
        return;
    }

    /* 确保灰度模块处于数字量输出模式，以便读取实时数据 */
    Track_SetDigitalMode();

    logPrintln("Nav Track Debug View - Press ^C to exit\r\n"
               "Probe:  1 2 3 4 5 6 7 8 (1=left)");

    Shell *shell = shellGetCurrent();
    if (shell == NULL) return;

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    TrackData_t data;
    for (;;) {
        uint8_t byte = 0;
        if (shell->read((char *)&byte, 1)) {
            if (byte == 0x03) break;
        }

        if (!Track_GetData(&data)) {
            osDelay(50);
            continue;
        }

        float error = 0.0f;
        bool valid = compute_error(data.digitalData, &error);

        logPrintln("\033[3A\033[2K\rLine:   %d %d %d %d %d %d %d %d\r\n"
                   "Error:  %.2f (%s)\r\n"
                   "Raw:    0x%02X",
                   (data.digitalData & 0x80) ? 1 : 0,
                   (data.digitalData & 0x40) ? 1 : 0,
                   (data.digitalData & 0x20) ? 1 : 0,
                   (data.digitalData & 0x10) ? 1 : 0,
                   (data.digitalData & 0x08) ? 1 : 0,
                   (data.digitalData & 0x04) ? 1 : 0,
                   (data.digitalData & 0x02) ? 1 : 0,
                   (data.digitalData & 0x01) ? 1 : 0,
                   error,
                   valid ? "on line" : "lost",
                   data.digitalData);

        osDelay(50);
    }

    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("\033[4A\033[J");
}

/**
 * @brief 设置前向速度
 */
static void ntrack_speed_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack speed <cm/s>"); return; }
    float val;
    if (!parse_float(argv[1], &val)) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.forward_speed = val;
    logPrintln("forward speed = %.1f cm/s", g_nav_track->params.forward_speed);
}

/**
 * @brief 设置P系数
 */
static void ntrack_kp_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack kp <value>"); return; }
    float val;
    if (!parse_float(argv[1], &val)) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.kp = val;
    logPrintln("kp = %.2f", g_nav_track->params.kp);
}

/**
 * @brief 设置I系数
 */
static void ntrack_ki_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack ki <value>"); return; }
    float val;
    if (!parse_float(argv[1], &val)) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.ki = val;
    logPrintln("ki = %.3f", g_nav_track->params.ki);
}

/**
 * @brief 设置D系数
 */
static void ntrack_kd_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack kd <value>"); return; }
    float val;
    if (!parse_float(argv[1], &val)) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.kd = val;
    logPrintln("kd = %.2f", g_nav_track->params.kd);
}

/**
 * @brief 设置最大转向速度
 */
static void ntrack_yaw_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack yaw <deg/s>"); return; }
    float val;
    if (!parse_float(argv[1], &val)) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.max_yaw_speed = val;
    logPrintln("max yaw speed = %.1f deg/s", g_nav_track->params.max_yaw_speed);
}

/**
 * @brief 设置线极性
 */
static void ntrack_pol_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack pol <1|0>"); return; }
    int32_t val = strtol(argv[1], NULL, 10);
    if (val != 0 && val != 1) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.line_polarity = (int8_t)val;
    logPrintln("line polarity = %d", g_nav_track->params.line_polarity);
}

/**
 * @brief 设置转向方向
 */
static void ntrack_dir_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack dir <1|-1>"); return; }
    int32_t val = strtol(argv[1], NULL, 10);
    if (val != 1 && val != -1) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.steering_dir = (int8_t)val;
    logPrintln("steering direction = %d", g_nav_track->params.steering_dir);
}

/**
 * @brief 设置巡线运行时间
 */
static void ntrack_time_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    if (argc != 2) { logPrintln("Usage: ntrack time <ms> (0=endless)"); return; }
    int32_t val = strtol(argv[1], NULL, 10);
    if (val < 0) { logPrintln("invalid value: %s", argv[1]); return; }
    g_nav_track->params.run_time_ms = (uint32_t)val;
    logPrintln("run time = %u ms", g_nav_track->params.run_time_ms);
}

static void ntrack_info_shell(int argc, char *argv[]) {
    if (!is_init) { logWarning("Nav track module not initialized"); return; }
    logPrintln("Nav track module info");
    logPrintln("  forward speed = %.1f cm/s", g_nav_track->params.forward_speed);
    logPrintln("  kp = %.2f", g_nav_track->params.kp);
    logPrintln("  ki = %.3f", g_nav_track->params.ki);
    logPrintln("  kd = %.2f", g_nav_track->params.kd);
    logPrintln("  max yaw speed = %.1f deg/s", g_nav_track->params.max_yaw_speed);
    logPrintln("  line polarity = %d", g_nav_track->params.line_polarity);
    logPrintln("  steering direction = %d", g_nav_track->params.steering_dir);
    logPrintln("  run time = %u ms", g_nav_track->params.run_time_ms);
}

ShellCommand NavTrackGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, start, ntrack_start_shell, start line following),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, stop, ntrack_stop_shell, stop line following),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, sta, ntrack_sta_shell, show status and params),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, view, ntrack_view_shell, debug view sensors and error),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, speed, ntrack_speed_shell, set forward speed cm/s),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, kp, ntrack_kp_shell, set P gain),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, ki, ntrack_ki_shell, set I gain),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, kd, ntrack_kd_shell, set D gain),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, yaw, ntrack_yaw_shell, set max yaw speed),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pol, ntrack_pol_shell, set line polarity),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, dir, ntrack_dir_shell, set steering direction),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, time, ntrack_time_shell, set run time ms),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, info, ntrack_info_shell, show info),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       ntrack, NavTrackGroup, Nav Track Tool Group);
