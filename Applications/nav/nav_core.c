/**
 * @file nav_core.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航核心控制器源文件 - 两阶段导航控制实现
 */

#include "nav_core.h"
#include "nav_local.h"
#include "nav_math.h"
#include "nav_map.h"
#include "nav_config.h"
#include "motion_control.h"
#include "step_ttl.h"
#include "cmsis_os2.h"
#include "Events.h"
#include <math.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <chassis_config.h>
#include "log.h"

#if !MOTOR_VELOCITY_MODE
    #error "MOTOR_VELOCITY_MODE must be enabled for velocity-based navigation"
#endif


/**
 * @brief PID控制器结构体
 */
typedef struct {
    float kp, ki, kd;       // PID参数
    float integral;         // 积分项
    float last_error;       // 上一次的误差
    float output;           // 输出
    float output_limit;     // 输出限制
} PIDController_t;

/**
 * @brief 导航核心控制器结构体
 */
typedef struct {
    NavState_t state;               // 导航状态
    uint32_t task_start_tick;       // 任务开始时间戳
    TargetPoint_t *cached_target;   // 缓存的目标点
    float saved_linear_speed;       // 保存的线速度
    float saved_yaw_speed;          // 保存的yaw速度
    float saved_acc;                // 保存的加速度
    float saved_dec;                // 保存的减速度
    PIDController_t yaw_pid;        // yaw角度PID控制器
    float last_vx;                  // 上一次的vx速度
    float last_vy;                  // 上一次的y速度
    float last_yaw_speed;           // 上一次的yaw速度
    bool has_reached_distance;      // 是否到达目标距离
    bool has_reached_yaw;           // 是否到达目标角度
    uint32_t arrive_timeout_tick;   // 到达超时时间戳
} NavCore_t;

static NavCore_t *g_nav_core = NULL;
static bool is_init = false;

static bool get_current_pose(Pose2D_t *pose);
static void restore_motion_params(void);
static void pid_reset(PIDController_t *pid);
static float pid_update(PIDController_t *pid, float error, float dt);
static float calculate_distance(Pose2D_t a, Pose2D_t b);
static bool check_arrival(Pose2D_t current, TargetPoint_t *target);


/**
 * @brief 获取当前位姿
 * @param pose 输出的位姿结构体指针
 * @return 获取状态
 */
static bool get_current_pose(Pose2D_t *pose) {
    PoseTimestamp_t pt;
    if (!Loc_Get(&pt)) return false;
    pose->x = pt.pose.x;
    pose->y = pt.pose.y;
    pose->yaw = pt.pose.yaw;
    return true;
}

/**
 * @brief 恢复运动参数
 */
static void restore_motion_params(void) {
    MotionControl_SetMotionParams(g_nav_core->saved_linear_speed,
                                   g_nav_core->saved_yaw_speed,
                                   g_nav_core->saved_acc,
                                   g_nav_core->saved_dec);
}

/**
 * @brief 重置PID控制器
 * @param pid PID控制器指针
 */
static void pid_reset(PIDController_t *pid) {
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->output = 0.0f;
}

/**
 * @brief 更新PID控制器输出
 * @param pid PID控制器指针
 * @param error 误差值
 * @param dt 时间间隔
 * @return PID输出
 */
static float pid_update(PIDController_t *pid, float error, float dt) {
    float p_term = pid->kp * error;

    pid->integral += error * dt;
    pid->integral = clamp(pid->integral, -pid->output_limit / pid->ki, pid->output_limit / pid->ki);
    float i_term = pid->ki * pid->integral;

    float derivative = (error - pid->last_error) / dt;
    float d_term = pid->kd * derivative;

    pid->last_error = error;

    pid->output = p_term + i_term + d_term;
    pid->output = clamp(pid->output, -pid->output_limit, pid->output_limit);

    return pid->output;
}

/**
 * @brief 计算两个点之间的距离
 * @param a 第一个点
 * @param b 第二个点
 * @return 距离
 */
static float calculate_distance(Pose2D_t a, Pose2D_t b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

/**
 * @brief 检查是否到达目标点
 * @param current 当前位姿
 * @param target 目标点指针
 * @return 是否到达目标点
 */
static bool check_arrival(Pose2D_t current, TargetPoint_t *target) {
    bool distance_ok = false;
    bool yaw_ok = false;

    if (target->arrive.check_mode == ARRIVE_CHECK_DISTANCE ||
        target->arrive.check_mode == ARRIVE_CHECK_BOTH) {
        float dist = calculate_distance(current, target->pose);
        distance_ok = (dist <= target->arrive.distance_threshold);
    } else {
        distance_ok = true;
    }

    if (target->arrive.check_mode == ARRIVE_CHECK_YAW ||
        target->arrive.check_mode == ARRIVE_CHECK_BOTH) {
        float target_yaw_norm = normalize_angle(target->pose.yaw);
        float yaw_diff = fabsf(normalize_angle(target_yaw_norm - current.yaw));
        yaw_ok = (yaw_diff <= target->arrive.yaw_threshold);
    } else {
        yaw_ok = true;
    }

    return (distance_ok && yaw_ok);
}

/**
 * @brief 初始化导航核心控制器
 * @return 初始化状态
 */
bool Nav_Core_Init(void) {
    g_nav_core = pvPortMalloc(sizeof(NavCore_t));
    if (g_nav_core == NULL) {
        return false;
    }
    memset(g_nav_core, 0, sizeof(NavCore_t));

    g_nav_core->state = NAV_STATE_IDLE;
    g_nav_core->yaw_pid.kp = NAV_YAW_PID_KP;
    g_nav_core->yaw_pid.ki = NAV_YAW_PID_KI;
    g_nav_core->yaw_pid.kd = NAV_YAW_PID_KD;
    g_nav_core->yaw_pid.output_limit = NAV_YAW_PID_OUTPUT_LIMIT;

    is_init = true;
    return true;
}

/**
 * @brief 导航到目标点
 * @param target_id 目标点ID
 * @return 导航状态
 */
bool Nav_GoTo(uint8_t target_id) {
    if (!is_init) return false;
    TargetPoint_t *target = Map_GetPoint(target_id);
    if (target == NULL) return false;

    MotionControl_GetMotionParams(&g_nav_core->saved_linear_speed,
                                   &g_nav_core->saved_yaw_speed,
                                   &g_nav_core->saved_acc,
                                   &g_nav_core->saved_dec);

    MotionControl_SetMotionParams(target->motion.target_speed,
                                   target->motion.target_angular_speed,
                                   target->motion.acceleration,
                                   target->motion.deceleration);

    g_nav_core->cached_target = target;
    g_nav_core->state = NAV_STATE_RUNNING;
    g_nav_core->task_start_tick = osKernelGetTickCount();
    g_nav_core->arrive_timeout_tick = g_nav_core->task_start_tick + target->arrive.timeout_ms;
    g_nav_core->has_reached_distance = false;
    g_nav_core->has_reached_yaw = false;
    g_nav_core->last_vx = 0.0f;
    g_nav_core->last_vy = 0.0f;
    g_nav_core->last_yaw_speed = 0.0f;

    logInfo("Navigation to target: %s, x: %.2f, y: %.2f, yaw: %.2f\r\n",
        target->name, target->pose.x, target->pose.y, target->pose.yaw);
    pid_reset(&g_nav_core->yaw_pid);
    return true;
}

/**
 * @brief 停止导航
 */
void Nav_Stop(void) {
    if (!is_init) return;

    MotionControl_Stop();
    g_nav_core->state = NAV_STATE_IDLE;
    g_nav_core->cached_target = NULL;
    restore_motion_params();
}

/**
 * @brief 获取导航状态
 * @return 导航状态
 */
NavState_t Nav_GetState(void) {
    return g_nav_core->state;
}

/**
 * @brief 导航任务
 */
void Nav_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!is_init) vTaskDelete(NULL);

    Pose2D_t current_pose;
    uint32_t last_control_tick = 0;

    for (;;) {
        osDelay(1);

        if (g_nav_core->state == NAV_STATE_IDLE) {
            last_control_tick = osKernelGetTickCount();
            continue;
        }

        if (!get_current_pose(&current_pose)) {
            logError("Failed to get current pose");
            continue;
        }

        uint32_t current_tick = osKernelGetTickCount();

        float dt = (current_tick - last_control_tick) / 1000.0f;
        last_control_tick = current_tick;

        if (g_nav_core->state == NAV_STATE_RUNNING && g_nav_core->cached_target != NULL) {
            TargetPoint_t *target = g_nav_core->cached_target;
            float distance = calculate_distance(current_pose, target->pose);

            if (check_arrival(current_pose, target)) {
                MotionControl_Stop();
                g_nav_core->state = NAV_STATE_COMPLETE;
                g_nav_core->last_vx = 0.0f;
                g_nav_core->last_vy = 0.0f;
                restore_motion_params();
                logInfo("Navigation complete, target: %s", target->name);
                continue;
            }

            if (current_tick > g_nav_core->arrive_timeout_tick) {
                MotionControl_Stop();
                g_nav_core->state = NAV_STATE_ERROR;
                g_nav_core->last_vx = 0.0f;
                g_nav_core->last_vy = 0.0f;
                restore_motion_params();
                logError("Navigation timeout");
                continue;
            }

            float max_speed = target->motion.target_speed;
            float acceleration = target->motion.acceleration;
            float deceleration = target->motion.deceleration;
            float dt_sec = dt;

            float dx = target->pose.x - current_pose.x;
            float dy = target->pose.y - current_pose.y;

            float out_vx, out_vy, yaw_speed;

            if (distance > NAV_ALIGN_DIST) {
                float target_vx_world = 0.0f;
                float target_vy_world = 0.0f;
                if (distance > 0.5f) {
                    // 提前减速：扩大减速区域
                    float speed_factor = 1.0f;
                    float decel_start = NAV_ALIGN_DIST + 30.0f;
                    if (distance < decel_start) {
                        speed_factor = (distance - 0.5f) / (decel_start - 0.5f);
                        if (speed_factor < 0.05f) speed_factor = 0.05f;
                    }
                    target_vx_world = dx / distance * max_speed * speed_factor;
                    target_vy_world = dy / distance * max_speed * speed_factor;
                }

                float cos_yaw = cosf(current_pose.yaw * DEG_TO_RAD);
                float sin_yaw = sinf(current_pose.yaw * DEG_TO_RAD);

                float target_vx = target_vx_world * cos_yaw + target_vy_world * sin_yaw;
                float target_vy = -target_vx_world * sin_yaw + target_vy_world * cos_yaw;

                float max_accel_change = acceleration * dt_sec;
                float max_decel_change = deceleration * dt_sec;
                float dvx = target_vx - g_nav_core->last_vx;
                float dvy = target_vy - g_nav_core->last_vy;

                if (dvx > 0) { if (dvx > max_accel_change) dvx = max_accel_change; }
                else { if (dvx < -max_decel_change) dvx = -max_decel_change; }
                if (dvy > 0) { if (dvy > max_accel_change) dvy = max_accel_change; }
                else { if (dvy < -max_decel_change) dvy = -max_decel_change; }

                g_nav_core->last_vx += dvx;
                g_nav_core->last_vy += dvy;

                out_vx = clamp(g_nav_core->last_vx, -max_speed, max_speed);
                out_vy = clamp(g_nav_core->last_vy, -max_speed, max_speed);

                float target_yaw_deg = atan2f(dy, dx) * RAD_TO_DEG;
                float yaw_error = normalize_angle(target_yaw_deg - current_pose.yaw);

                if (fabsf(yaw_error) > 1.5f) {
                    yaw_speed = yaw_error * NAV_APPROACH_YAW_KP;
                    yaw_speed = clamp(yaw_speed,
                                     -NAV_APPROACH_YAW_MAX,
                                      NAV_APPROACH_YAW_MAX);
                } else {
                    yaw_speed = 0.0f;
                }

            } else {
                float cos_yaw = cosf(current_pose.yaw * DEG_TO_RAD);
                float sin_yaw = sinf(current_pose.yaw * DEG_TO_RAD);

                float body_dx = dx * cos_yaw + dy * sin_yaw;
                float body_dy = -dx * sin_yaw + dy * cos_yaw;

                float dist_factor = distance / NAV_ALIGN_DIST;
                float stage2_max_speed = max_speed * dist_factor;
                if (stage2_max_speed < NAV_MIN_SPEED) stage2_max_speed = NAV_MIN_SPEED;

                float target_yaw_norm = normalize_angle(target->pose.yaw);
                float yaw_error = normalize_angle(target_yaw_norm - current_pose.yaw);

                bool dist_ok = (distance <= target->arrive.distance_threshold);
                bool yaw_ok = (fabsf(yaw_error) <= target->arrive.yaw_threshold);

                if (!dist_ok && yaw_ok) {
                    if (fabsf(body_dx) < NAV_ALIGN_XY_DEADBAND) body_dx = 0.0f;
                    if (fabsf(body_dy) < NAV_ALIGN_XY_DEADBAND) body_dy = 0.0f;

                    out_vx = body_dx * NAV_ALIGN_XY_KP;
                    out_vy = body_dy * NAV_ALIGN_XY_KP;
                    out_vx = clamp(out_vx, -stage2_max_speed, stage2_max_speed);
                    out_vy = clamp(out_vy, -stage2_max_speed, stage2_max_speed);
                    yaw_speed = 0.0f;
                } else if (dist_ok && !yaw_ok) {
                    out_vx = 0.0f;
                    out_vy = 0.0f;

                    yaw_speed = pid_update(&g_nav_core->yaw_pid, yaw_error, dt_sec);
                    yaw_speed = clamp(yaw_speed, -NAV_ALIGN_YAW_MAX, NAV_ALIGN_YAW_MAX);
                    if (fabsf(yaw_speed) > 0.0f && fabsf(yaw_speed) < NAV_ALIGN_YAW_MIN) {
                        yaw_speed = (yaw_speed > 0) ? NAV_ALIGN_YAW_MIN : -NAV_ALIGN_YAW_MIN;
                    }
                } else {
                    if (fabsf(body_dx) < NAV_ALIGN_XY_DEADBAND) body_dx = 0.0f;
                    if (fabsf(body_dy) < NAV_ALIGN_XY_DEADBAND) body_dy = 0.0f;

                    out_vx = body_dx * NAV_ALIGN_XY_KP;
                    out_vy = body_dy * NAV_ALIGN_XY_KP;
                    out_vx = clamp(out_vx, -stage2_max_speed, stage2_max_speed);
                    out_vy = clamp(out_vy, -stage2_max_speed, stage2_max_speed);

                    yaw_speed = pid_update(&g_nav_core->yaw_pid, yaw_error, dt_sec);
                    yaw_speed = clamp(yaw_speed, -NAV_ALIGN_YAW_MAX, NAV_ALIGN_YAW_MAX);
                    if (fabsf(yaw_speed) > 0.0f && fabsf(yaw_speed) < NAV_ALIGN_YAW_MIN) {
                        yaw_speed = (yaw_speed > 0) ? NAV_ALIGN_YAW_MIN : -NAV_ALIGN_YAW_MIN;
                    }
                }
            }

            MotionControl_SetVelocity(out_vx, out_vy, yaw_speed);
        }
    }
}
