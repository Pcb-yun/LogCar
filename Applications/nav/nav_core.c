/**
 * @file nav_core.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航核心控制器源文件
 *
 * 导航任务采用分层控制架构：
 * 层1: 梯形速度规划 - 计算速度幅值
 * 层2: 前馈速度解算 - 世界系到车身系变换
 * 层3: 航向控制 - 自适应前馈+PID
 * 层4: 位置反馈修正 - PI补偿（远距离模式）
 * 层5: 输出融合与限幅 - 加减速限制+速度限幅
 *
 * 控制模式分为两种：
 * - 远距离模式: 距离 > near_threshold，使用PI反馈补偿
 * - 近目标模式: 距离 <= near_threshold，使用纯P反馈，消除积分过冲
 */

#include "nav_core.h"
#include "nav_local.h"
#include "nav_math.h"
#include "nav_config.h"
#include "motion_control.h"
#include "cmsis_os2.h"
#include "Events.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <chassis_config.h>
#include "log.h"
#include "zdt_v5_cfg.h"

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
    NavState_t state;               // 导航状态（IDLE/RUNNING/STOPPING/COMPLETE/ERROR）
    uint32_t task_start_tick;       // 任务开始时间戳
    uint32_t last_control_tick;     // 上次控制时间戳
    TargetPoint_t *cached_target;   // 缓存的目标点
    float saved_linear_speed;       // 保存的线速度（导航前状态）
    float saved_yaw_speed;          // 保存的yaw速度（导航前状态）
    float saved_acc;                // 保存的加速度（导航前状态）
    float saved_dec;                // 保存的减速度（导航前状态）
    PIDController_t yaw_pid;        // yaw角度PID控制器
    float last_vx;                  // 上一次的车身系X速度
    float last_vy;                  // 上一次的车身系Y速度
    float last_yaw_speed;           // 上一次的yaw角速度
    float integral_x;               // X方向位置积分项（远距离模式PI控制）
    float integral_y;               // Y方向位置积分项（远距离模式PI控制）
    float current_speed_magnitude;  // 当前速度幅值（用于梯形规划）
    float last_yaw_error;           // 上一次的yaw误差（用于零交叉检测）
    bool yaw_zero_cross_lock;       // yaw零交叉锁存标志
    uint32_t yaw_zero_cross_lock_start; // 零交叉锁存开始时间戳
    uint32_t arrive_timeout_tick;   // 到达超时时间戳
    uint8_t arrive_check_count;     // 连续到达检查计数器
    uint32_t stop_start_tick;       // 停止开始时间戳（用于等待车辆静止）
    bool last_near_target;          // 上一帧是否处于近目标模式
    uint8_t near_transition_count;  // 近目标过渡剩余帧数
    float trans_last_ff_vx;         // 过渡起点：远距离前馈+反馈合成的vx
    float trans_last_ff_vy;         // 过渡起点：远距离前馈+反馈合成的vy
} NavCore_t;

static NavCore_t *g_nav_core = NULL;
static bool is_init = false;

/* 静态辅助函数声明 */
static bool get_current_pose(Pose2D_t *pose);
static void restore_motion_params(void);
static void pid_reset(PIDController_t *pid);
static float pid_update(PIDController_t *pid, float error, float dt);
static float calculate_distance(Pose2D_t a, Pose2D_t b);
static bool check_arrival(Pose2D_t current, TargetPoint_t *target);
static float trapezoidal_speed_planner(float remaining_dist, float current_speed,
                                       float max_speed, float acceleration,
                                       float deceleration, float dt);
static void compute_feedforward_velocity(float dx, float dy, float current_yaw,
                                         float speed_magnitude,
                                         float *out_vx, float *out_vy);
static float compute_yaw_speed(float yaw_error, float distance, float dt);
static void compute_position_correction(float body_dx, float body_dy, float dt, float distance,
                                        float *out_vx, float *out_vy);
static void world_to_body(float dx_world, float dy_world, float yaw_deg,
                          float *body_dx, float *body_dy);
static void apply_accel_limit(float desired_vx, float desired_vy, float *out_vx, float *out_vy,
                              float acceleration, float deceleration, float dt);


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
 * @brief 检查是否到达目标点（连续多次确认）
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

    // 速度过大时不判定到达（防止高速穿堂而过直接停车），但允许近目标速度
    // 因为进入 STOPPING 状态后会先 MotionControl_Stop，再等 500ms 二次确认距离
    float current_speed = sqrtf(g_nav_core->last_vx * g_nav_core->last_vx +
                                g_nav_core->last_vy * g_nav_core->last_vy);
    bool speed_not_too_fast = (current_speed <= NAV_TRAJ_SPEED_CAP);

    if (target->arrive.check_mode == ARRIVE_CHECK_YAW ||
        target->arrive.check_mode == ARRIVE_CHECK_BOTH) {
        float target_yaw_norm = normalize_angle(target->pose.yaw);
        float yaw_diff = fabsf(normalize_angle(target_yaw_norm - current.yaw));
        yaw_ok = (yaw_diff <= target->arrive.yaw_threshold);
    } else {
        yaw_ok = true;
    }

    // 只要距离+角度满足，且速度未超过近目标上限，就计数
    // 低速补偿维持的速度不会阻碍到达判定，由 STOPPING 的二次确认兜底
    if (distance_ok && yaw_ok && speed_not_too_fast) {
        g_nav_core->arrive_check_count++;
        if (g_nav_core->arrive_check_count >= NAV_ARRIVE_CHECK_COUNT) {
            return true;
        }
    } else {
        g_nav_core->arrive_check_count = 0;
    }

    return false;
}

/**
 * @brief 梯形速度规划器
 * @param remaining_dist 剩余距离 (cm)
 * @param current_speed 当前速度幅值 (cm/s)
 * @param max_speed 最大速度 (cm/s)
 * @param acceleration 加速度 (cm/s²)
 * @param deceleration 减速度 (cm/s²)
 * @param dt 时间间隔 (s)
 * @return 规划后的速度幅值
 */
static float trapezoidal_speed_planner(float remaining_dist, float current_speed,
                                       float max_speed, float acceleration,
                                       float deceleration, float dt) {
    if (remaining_dist < 0.5f) {
        return 0.0f;
    }

    float min_speed = NAV_MIN_SPEED;

    float decel_dist = (current_speed * current_speed) / (2.0f * deceleration);

    if (remaining_dist < decel_dist) {
        float target_speed = sqrtf(2.0f * deceleration * remaining_dist);
        if (target_speed < min_speed) target_speed = min_speed;
        if (target_speed > NAV_TRAJ_SPEED_CAP) target_speed = NAV_TRAJ_SPEED_CAP;
        float speed_diff = target_speed - current_speed;
        float max_decel_change = deceleration * dt;
        if (speed_diff < -max_decel_change) speed_diff = -max_decel_change;
        return current_speed + speed_diff;
    } else {
        float target_speed = max_speed;
        float speed_diff = target_speed - current_speed;
        float max_accel_change = acceleration * dt;
        float max_decel_change = deceleration * dt;
        if (speed_diff > max_accel_change) speed_diff = max_accel_change;
        else if (speed_diff < -max_decel_change) speed_diff = -max_decel_change;
        float result = current_speed + speed_diff;
        if (result < min_speed) result = min_speed;
        return result;
    }
}

/**
 * @brief 计算前馈速度（世界系→车身系）
 * @param dx 世界系X方向误差 (cm)
 * @param dy 世界系Y方向误差 (cm)
 * @param current_yaw 当前航向角 (deg)
 * @param speed_magnitude 速度幅值 (cm/s)
 * @param out_vx 输出车身系X速度
 * @param out_vy 输出车身系Y速度
 */
static void compute_feedforward_velocity(float dx, float dy, float current_yaw,
                                         float speed_magnitude,
                                         float *out_vx, float *out_vy) {
    float cos_yaw = cosf(current_yaw * DEG_TO_RAD);
    float sin_yaw = sinf(current_yaw * DEG_TO_RAD);

    float target_vx_world = dx * speed_magnitude;
    float target_vy_world = dy * speed_magnitude;

    *out_vx = target_vx_world * cos_yaw + target_vy_world * sin_yaw;
    *out_vy = -target_vx_world * sin_yaw + target_vy_world * cos_yaw;
}

/**
 * @brief 计算航向角速度（自适应增益）
 * @param yaw_error 航向误差 (deg)
 * @param distance 剩余距离 (cm)
 * @param dt 时间间隔 (s)
 * @return 航向角速度 (deg/s)
 */
static float compute_yaw_speed(float yaw_error, float distance, float dt) {
    float gain_factor = (distance > NAV_ALIGN_DIST) ? 0.3f : 1.0f;

    float feedforward_yaw = 0.0f;
    if (fabsf(yaw_error) > NAV_YAW_FEEDFORWARD_THRESH) {
        feedforward_yaw = (yaw_error > 0) ? NAV_ALIGN_YAW_MAX : -NAV_ALIGN_YAW_MAX;
        feedforward_yaw *= gain_factor;
    }

    float feedback_yaw = pid_update(&g_nav_core->yaw_pid, yaw_error, dt);
    feedback_yaw *= gain_factor;

    float yaw_speed = feedforward_yaw * NAV_YAW_FEEDFORWARD_WEIGHT + feedback_yaw * NAV_YAW_FEEDBACK_WEIGHT;
    yaw_speed = clamp(yaw_speed, -NAV_ALIGN_YAW_MAX, NAV_ALIGN_YAW_MAX);

    if (fabsf(yaw_error) < NAV_YAW_DEADBAND) {
        yaw_speed = 0.0f;
    } else {
        float error_sign = (yaw_error > 0) ? 1.0f : -1.0f;

        if (fabsf(yaw_speed) < NAV_ALIGN_YAW_MIN) {
            yaw_speed = error_sign * NAV_ALIGN_YAW_MIN;
        } else if (fabsf(yaw_speed) >= NAV_ALIGN_YAW_MIN) {
            float speed_sign = (yaw_speed > 0) ? 1.0f : -1.0f;

            if (speed_sign != error_sign) {
                yaw_speed = error_sign * NAV_ALIGN_YAW_MIN;
            }
        }
    }

    float dyaw_speed = yaw_speed - g_nav_core->last_yaw_speed;
    float max_yaw_accel = NAV_YAW_ACCEL_LIMIT * dt;
    if (dyaw_speed > 0) { if (dyaw_speed > max_yaw_accel) dyaw_speed = max_yaw_accel; }
    else { if (dyaw_speed < -max_yaw_accel) dyaw_speed = -max_yaw_accel; }

    g_nav_core->last_yaw_speed += dyaw_speed;
    return g_nav_core->last_yaw_speed;
}

/**
 * @brief 计算位置反馈修正（PI补偿）
 * @param body_dx 车身系X方向误差 (cm)
 * @param body_dy 车身系Y方向误差 (cm)
 * @param dt 时间间隔 (s)
 * @param distance 剩余距离 (cm)
 * @param out_vx 输出X方向修正速度
 * @param out_vy 输出Y方向修正速度
 */
static void compute_position_correction(float body_dx, float body_dy, float dt, float distance,
                                        float *out_vx, float *out_vy) {
    float body_dist = sqrtf(body_dx * body_dx + body_dy * body_dy);

    if (body_dist < NAV_ALIGN_XY_HYSTERESIS) {
        *out_vx = 0.0f;
        *out_vy = 0.0f;
        g_nav_core->integral_x = 0.0f;
        g_nav_core->integral_y = 0.0f;
        return;
    }

    if (fabsf(body_dx) < NAV_ALIGN_XY_DEADBAND) body_dx = 0.0f;
    if (fabsf(body_dy) < NAV_ALIGN_XY_DEADBAND) body_dy = 0.0f;

    float dist_factor = (distance > NAV_ALIGN_DIST) ? (NAV_ALIGN_DIST / distance) : 1.0f;
    if (dist_factor < 0.1f) dist_factor = 0.1f;

    float speed_dir_x = g_nav_core->last_vx;
    float speed_dir_y = g_nav_core->last_vy;
    float speed_mag = sqrtf(speed_dir_x * speed_dir_x + speed_dir_y * speed_dir_y);

    float raw_vx = 0.0f, raw_vy = 0.0f;

    if (speed_mag > NAV_POSITION_CORRECTION_SPEED_THRESH) {
        speed_dir_x /= speed_mag;
        speed_dir_y /= speed_mag;

        float longitudinal_err = body_dx * speed_dir_x + body_dy * speed_dir_y;
        float lateral_err_x = body_dx - longitudinal_err * speed_dir_x;
        float lateral_err_y = body_dy - longitudinal_err * speed_dir_y;

        g_nav_core->integral_x += lateral_err_x * dt * dist_factor;
        g_nav_core->integral_y += lateral_err_y * dt * dist_factor;

        float integral_max = (NAV_ALIGN_XY_KP / NAV_ALIGN_XY_KI) * dist_factor;
        g_nav_core->integral_x = clamp(g_nav_core->integral_x, -integral_max, integral_max);
        g_nav_core->integral_y = clamp(g_nav_core->integral_y, -integral_max, integral_max);

        raw_vx = lateral_err_x * NAV_ALIGN_XY_KP * dist_factor + g_nav_core->integral_x * NAV_ALIGN_XY_KI;
        raw_vy = lateral_err_y * NAV_ALIGN_XY_KP * dist_factor + g_nav_core->integral_y * NAV_ALIGN_XY_KI;
    } else {
        g_nav_core->integral_x += body_dx * dt * dist_factor;
        g_nav_core->integral_y += body_dy * dt * dist_factor;

        float integral_max = (NAV_ALIGN_XY_KP / NAV_ALIGN_XY_KI) * dist_factor;
        g_nav_core->integral_x = clamp(g_nav_core->integral_x, -integral_max, integral_max);
        g_nav_core->integral_y = clamp(g_nav_core->integral_y, -integral_max, integral_max);

        raw_vx = body_dx * NAV_ALIGN_XY_KP * dist_factor + g_nav_core->integral_x * NAV_ALIGN_XY_KI;
        raw_vy = body_dy * NAV_ALIGN_XY_KP * dist_factor + g_nav_core->integral_y * NAV_ALIGN_XY_KI;
    }

    if (speed_mag < NAV_POSITION_CORRECTION_SPEED_THRESH) {
        if (fabsf(body_dx) > NAV_ALIGN_XY_DEADBAND) {
            float sign_x = (body_dx > 0) ? 1.0f : -1.0f;
            raw_vx += sign_x * NAV_LOW_SPEED_COMPENSATION;
        }
        if (fabsf(body_dy) > NAV_ALIGN_XY_DEADBAND) {
            float sign_y = (body_dy > 0) ? 1.0f : -1.0f;
            raw_vy += sign_y * NAV_LOW_SPEED_COMPENSATION;
        }
    }

    *out_vx = raw_vx;
    *out_vy = raw_vy;
}

/**
 * @brief 世界系到车身系坐标变换
 *
 * 将世界系下的误差向量转换为车身系下的误差向量。
 *
 * @param dx_world 世界系X方向误差 (cm)
 * @param dy_world 世界系Y方向误差 (cm)
 * @param yaw_deg 当前航向角 (deg)
 * @param body_dx 输出车身系X方向误差
 * @param body_dy 输出车身系Y方向误差
 */
static void world_to_body(float dx_world, float dy_world, float yaw_deg,
                          float *body_dx, float *body_dy) {
    float cos_yaw = cosf(yaw_deg * DEG_TO_RAD);
    float sin_yaw = sinf(yaw_deg * DEG_TO_RAD);
    *body_dx = dx_world * cos_yaw + dy_world * sin_yaw;
    *body_dy = -dx_world * sin_yaw + dy_world * cos_yaw;
}

/**
 * @brief 应用加减速限制
 *
 * 根据加速度和减速度参数，限制速度变化量。
 *
 * @param desired_vx 期望X速度
 * @param desired_vy 期望Y速度
 * @param out_vx 输出X速度（已应用限制）
 * @param out_vy 输出Y速度（已应用限制）
 * @param acceleration 加速度 (cm/s²)
 * @param deceleration 减速度 (cm/s²)
 * @param dt 时间间隔 (s)
 */
static void apply_accel_limit(float desired_vx, float desired_vy, float *out_vx, float *out_vy,
                              float acceleration, float deceleration, float dt) {
    float dvx = desired_vx - g_nav_core->last_vx;
    float dvy = desired_vy - g_nav_core->last_vy;
    float max_accel_change = acceleration * dt;
    float max_decel_change = deceleration * dt;

    if (dvx > 0) { if (dvx > max_accel_change) dvx = max_accel_change; }
    else { if (dvx < -max_decel_change) dvx = -max_decel_change; }
    if (dvy > 0) { if (dvy > max_accel_change) dvy = max_accel_change; }
    else { if (dvy < -max_decel_change) dvy = -max_decel_change; }

    g_nav_core->last_vx += dvx;
    g_nav_core->last_vy += dvy;
    *out_vx = g_nav_core->last_vx;
    *out_vy = g_nav_core->last_vy;
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
 * @brief 导航到指定ID点
 * @param target_id 目标点ID
 * @return 导航状态
 */
bool Nav_GoTo(uint8_t target_id) {
    if (!is_init) return false;
    TargetPoint_t *target = Map_GetPoint(target_id);
    if (target == NULL) return false;

    return Nav_GoToDirect(target);
}

/**
 * @brief 导航到指定名称点
 * @param name 目标点名称
 * @return 导航状态
 */
bool Nav_GoTo_fromName(const char *name) {
    if (!is_init) return false;
    TargetPoint_t *target = Map_GetPointByName(name);
    if (target == NULL) return false;

    return Nav_GoToDirect(target);
}

/**
 * @brief 导航到指定目标点
 * @param target 目标点指针
 * @return 导航状态
 */
bool Nav_GoToDirect(TargetPoint_t *target) {
    if (!is_init || target == NULL) return false;

    MotionControl_GetMotionParams(&g_nav_core->saved_linear_speed,
                                   &g_nav_core->saved_yaw_speed,
                                   &g_nav_core->saved_acc,
                                   &g_nav_core->saved_dec);

    MotionControl_SetMotionParams(target->motion.target_speed,
                                   target->motion.target_angular_speed,
                                   0.0f,
                                   0.0f);

    g_nav_core->cached_target = target;
    g_nav_core->state = NAV_STATE_RUNNING;
    g_nav_core->task_start_tick = osKernelGetTickCount();
    g_nav_core->last_control_tick = osKernelGetTickCount();
    g_nav_core->arrive_timeout_tick = g_nav_core->task_start_tick + target->arrive.timeout_ms;
    g_nav_core->last_vx = 0.0f;
    g_nav_core->last_vy = 0.0f;
    g_nav_core->last_yaw_speed = 0.0f;
    g_nav_core->integral_x = 0.0f;
    g_nav_core->integral_y = 0.0f;
    g_nav_core->current_speed_magnitude = 0.0f;
    g_nav_core->last_yaw_error = 0.0f;
    g_nav_core->yaw_zero_cross_lock = false;
    g_nav_core->yaw_zero_cross_lock_start = 0;
    g_nav_core->arrive_check_count = 0;

    logInfo("Direct navigation to: x: %.2f, y: %.2f, yaw: %.2f, name: %s",
        target->pose.x, target->pose.y, target->pose.yaw, target->name);
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
    g_nav_core->last_vx = 0.0f;
    g_nav_core->last_vy = 0.0f;
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
 *
 * 主循环周期：NAV_UPDATE_TIME (5ms)
 * 控制流程：
 * 1. 获取当前位姿
 * 2. 到达检测 / 超时检测
 * 3. 梯形速度规划（层1）
 * 4. 前馈速度解算（层2）
 * 5. 航向控制（层3）
 * 6. 位置反馈修正（层4/近目标纯P）
 * 7. 输出融合与限幅（层5）
 * 8. 执行速度指令
 */
void Nav_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    if (!is_init) vTaskDelete(NULL);

    Pose2D_t current_pose;
    float out_vx = 0.0f, out_vy = 0.0f, yaw_speed = 0.0f;

    for (;;) {
        osDelay(NAV_UPDATE_TIME);

        if (g_nav_core->state == NAV_STATE_IDLE) {
            g_nav_core->last_control_tick = osKernelGetTickCount();
            continue;
        }

        if (!get_current_pose(&current_pose)) {
            logError("Failed to get current pose");
            continue;
        }

        uint32_t current_tick = osKernelGetTickCount();
        float dt = (current_tick - g_nav_core->last_control_tick) / 1000.0f;
        g_nav_core->last_control_tick = current_tick;

        if (g_nav_core->state == NAV_STATE_RUNNING && g_nav_core->cached_target != NULL) {
            TargetPoint_t *target = g_nav_core->cached_target;
            float dx = target->pose.x - current_pose.x;
            float dy = target->pose.y - current_pose.y;
            float distance = calculate_distance(current_pose, target->pose);

            if (check_arrival(current_pose, target)) {
                MotionControl_Stop();
                g_nav_core->state = NAV_STATE_STOPPING;
                g_nav_core->stop_start_tick = current_tick;
                g_nav_core->last_vx = 0.0f;
                g_nav_core->last_vy = 0.0f;
                g_nav_core->current_speed_magnitude = 0.0f;
                continue;
            }

            if (current_tick > g_nav_core->arrive_timeout_tick) {
                MotionControl_Stop();
                g_nav_core->state = NAV_STATE_ERROR;
                g_nav_core->last_vx = 0.0f;
                g_nav_core->last_vy = 0.0f;
                g_nav_core->current_speed_magnitude = 0.0f;
                restore_motion_params();
                logError("Navigation timeout");
                continue;
            }

            float max_speed = target->motion.target_speed;
            float acceleration = target->motion.acceleration;
            float deceleration = target->motion.deceleration;

            float current_speed = sqrtf(g_nav_core->last_vx * g_nav_core->last_vx +
                                    g_nav_core->last_vy * g_nav_core->last_vy);

            g_nav_core->current_speed_magnitude = trapezoidal_speed_planner(
                distance, current_speed,
                max_speed, acceleration, deceleration, dt
            );

            float feedforward_vx = 0.0f, feedforward_vy = 0.0f;
            float unit_dx = (distance > 0.1f) ? dx / distance : 0.0f;
            float unit_dy = (distance > 0.1f) ? dy / distance : 0.0f;
            compute_feedforward_velocity(unit_dx, unit_dy, current_pose.yaw,
                                         g_nav_core->current_speed_magnitude,
                                         &feedforward_vx, &feedforward_vy);

            float target_yaw_norm = normalize_angle(target->pose.yaw);
            float yaw_error = normalize_angle(target_yaw_norm - current_pose.yaw);

            bool yaw_locked = false;
            if (g_nav_core->yaw_zero_cross_lock) {
                if (current_tick - g_nav_core->yaw_zero_cross_lock_start >= NAV_YAW_ZERO_CROSS_LOCK_MS) {
                    g_nav_core->yaw_zero_cross_lock = false;
                } else {
                    yaw_speed = 0.0f;
                    g_nav_core->last_yaw_error = yaw_error;
                    yaw_locked = true;
                }
            }

            if (!yaw_locked && g_nav_core->last_yaw_error != 0.0f && (yaw_error * g_nav_core->last_yaw_error) < 0) {
                g_nav_core->yaw_zero_cross_lock = true;
                g_nav_core->yaw_zero_cross_lock_start = current_tick;
                yaw_speed = 0.0f;
                g_nav_core->last_yaw_error = yaw_error;
                yaw_locked = true;
            }

            if (!yaw_locked) {
                yaw_speed = compute_yaw_speed(yaw_error, distance, dt);
                g_nav_core->last_yaw_error = yaw_error;
            }

            float near_threshold = target->arrive.distance_threshold * NAV_NEAR_TARGET_FACTOR;
            bool near_target = (distance <= near_threshold);

            if (near_target) {
                g_nav_core->integral_x = 0.0f;
                g_nav_core->integral_y = 0.0f;

                // 检测远→近切换，启动平滑过渡
                if (!g_nav_core->last_near_target) {
                    g_nav_core->near_transition_count = NAV_NEAR_TRANSITION_FRAMES;
                    // 用上一帧已经下发的速度作为过渡起点，保证连续性
                    g_nav_core->trans_last_ff_vx = g_nav_core->last_vx;
                    g_nav_core->trans_last_ff_vy = g_nav_core->last_vy;
                }

                float body_dx, body_dy;
                world_to_body(dx, dy, current_pose.yaw, &body_dx, &body_dy);

                float body_dist = sqrtf(body_dx * body_dx + body_dy * body_dy);

                float deadband = target->arrive.distance_threshold * NAV_NEAR_TARGET_DEADBAND_RATIO;
                if (deadband < NAV_ALIGN_XY_DEADBAND) deadband = NAV_ALIGN_XY_DEADBAND;
                if (fabsf(body_dx) < deadband) body_dx = 0.0f;
                if (fabsf(body_dy) < deadband) body_dy = 0.0f;

                float stop_threshold = target->arrive.distance_threshold * NAV_NEAR_TARGET_STOP_RATIO;
                if (stop_threshold < NAV_ALIGN_XY_HYSTERESIS * 3.0f) {
                    stop_threshold = NAV_ALIGN_XY_HYSTERESIS * 3.0f;
                }

                if (body_dist < stop_threshold) {
                    // 渐进趋近：保持小比例纯P，不直接硬清零，交给加减速限制缓慢收敛
                    float feedback_vx = body_dx * NAV_ALIGN_XY_KP * NAV_NEAR_TARGET_GAIN * 0.5f;
                    float feedback_vy = body_dy * NAV_ALIGN_XY_KP * NAV_NEAR_TARGET_GAIN * 0.5f;

                    float desired_vx = feedback_vx;
                    float desired_vy = feedback_vy;

                    float near_acceleration = acceleration * NAV_NEAR_ACCEL_MULTIPLIER;
                    float near_deceleration = deceleration * NAV_NEAR_ACCEL_MULTIPLIER;
                    apply_accel_limit(desired_vx, desired_vy, &out_vx, &out_vy,
                                     near_acceleration, near_deceleration, dt);

                    float speed = sqrtf(out_vx * out_vx + out_vy * out_vy);
                    // 低速时不做强制缩放，允许平滑趋近0；低于启动速度且无有效误差才钳位为0
                    if (body_dist < deadband && speed < NAV_STARTUP_SPEED) {
                        out_vx = 0.0f;
                        out_vy = 0.0f;
                        g_nav_core->last_vx = 0.0f;
                        g_nav_core->last_vy = 0.0f;
                    }

                    g_nav_core->current_speed_magnitude = sqrtf(out_vx * out_vx + out_vy * out_vy);
                } else {
                    float feedback_vx = body_dx * NAV_ALIGN_XY_KP * NAV_NEAR_TARGET_GAIN;
                    float feedback_vy = body_dy * NAV_ALIGN_XY_KP * NAV_NEAR_TARGET_GAIN;

                    float desired_vx = feedback_vx;
                    float desired_vy = feedback_vy;

                    // 远→近过渡阶段，在上一帧远距离速度与当前纯P输出之间线性混合
                    if (g_nav_core->near_transition_count > 0) {
                        float alpha = 1.0f - (float)g_nav_core->near_transition_count / (float)NAV_NEAR_TRANSITION_FRAMES;
                        // 若起点速度为0（刚起步就进入近目标），跳过过渡直接用纯P
                        float start_speed = sqrtf(g_nav_core->trans_last_ff_vx * g_nav_core->trans_last_ff_vx +
                                                  g_nav_core->trans_last_ff_vy * g_nav_core->trans_last_ff_vy);
                        if (start_speed > 0.5f) {
                            desired_vx = g_nav_core->trans_last_ff_vx * (1.0f - alpha) + feedback_vx * alpha;
                            desired_vy = g_nav_core->trans_last_ff_vy * (1.0f - alpha) + feedback_vy * alpha;
                        }
                        g_nav_core->near_transition_count--;
                    }

                    float near_acceleration = acceleration * NAV_NEAR_ACCEL_MULTIPLIER;
                    float near_deceleration = deceleration * NAV_NEAR_ACCEL_MULTIPLIER;
                    apply_accel_limit(desired_vx, desired_vy, &out_vx, &out_vy,
                                     near_acceleration, near_deceleration, dt);

                    float speed = sqrtf(out_vx * out_vx + out_vy * out_vy);

                    // 用低速补偿替代强制启动速度缩放：有误差才补一个固定偏移，避免反复跳变
                    if (speed > 0.0f && speed < NAV_STARTUP_SPEED && body_dist > deadband) {
                        float sign_x = (out_vx > 0) ? 1.0f : ((out_vx < 0) ? -1.0f : 0.0f);
                        float sign_y = (out_vy > 0) ? 1.0f : ((out_vy < 0) ? -1.0f : 0.0f);
                        if (sign_x != 0.0f || sign_y != 0.0f) {
                            float delta = NAV_STARTUP_SPEED - speed;
                            if (sign_x != 0.0f && sign_y != 0.0f) {
                                // 两个方向都有分量，按比例分配补偿量
                                float ratio_x = fabsf(out_vx) / speed;
                                float ratio_y = fabsf(out_vy) / speed;
                                out_vx += sign_x * delta * ratio_x;
                                out_vy += sign_y * delta * ratio_y;
                            } else {
                                out_vx += sign_x * delta;
                                out_vy += sign_y * delta;
                            }
                            g_nav_core->last_vx = out_vx;
                            g_nav_core->last_vy = out_vy;
                            speed = NAV_STARTUP_SPEED;
                        }
                    }

                    float max_near_speed = NAV_TRAJ_SPEED_CAP;
                    if (speed > max_near_speed) {
                        out_vx = out_vx / speed * max_near_speed;
                        out_vy = out_vy / speed * max_near_speed;
                        g_nav_core->last_vx = out_vx;
                        g_nav_core->last_vy = out_vy;
                    }

                    g_nav_core->current_speed_magnitude = sqrtf(out_vx * out_vx + out_vy * out_vy);
                }

                g_nav_core->last_near_target = true;
            } else {
                float body_dx, body_dy;
                world_to_body(dx, dy, current_pose.yaw, &body_dx, &body_dy);

                float feedback_vx = 0.0f, feedback_vy = 0.0f;
                compute_position_correction(body_dx, body_dy, dt, distance, &feedback_vx, &feedback_vy);

                out_vx = feedforward_vx + feedback_vx;
                out_vy = feedforward_vy + feedback_vy;

                apply_accel_limit(out_vx, out_vy, &out_vx, &out_vy,
                                 acceleration, deceleration, dt);

                float speed = sqrtf(out_vx * out_vx + out_vy * out_vy);
                if (speed > max_speed) {
                    out_vx = out_vx / speed * max_speed;
                    out_vy = out_vy / speed * max_speed;
                    g_nav_core->last_vx = out_vx;
                    g_nav_core->last_vy = out_vy;
                }

                // 离开近目标模式，复位过渡状态
                g_nav_core->last_near_target = false;
                g_nav_core->near_transition_count = 0;
            }

            if (g_nav_core->state != NAV_STATE_RUNNING) {
                out_vx = out_vy = yaw_speed = 0.0f;
                MotionControl_Stop();
            }

            // logDebug("err_x: %.2f, err_y: %.2f, dist: %.2f, err_yaw: %.2f;    vx: %.2f, vy: %.2f, yaw: %.2f",
            //          dx, dy, distance, yaw_error, out_vx, out_vy, yaw_speed);
            MotionControl_SetVelocity(out_vx, out_vy, yaw_speed);
        }

        if (g_nav_core->state == NAV_STATE_STOPPING && g_nav_core->cached_target != NULL) {
            uint32_t stop_wait_ms = 300;
            if (current_tick - g_nav_core->stop_start_tick >= stop_wait_ms) {
                float final_distance = calculate_distance(current_pose, g_nav_core->cached_target->pose);

                if (final_distance <= g_nav_core->cached_target->arrive.distance_threshold) {
                    g_nav_core->state = NAV_STATE_COMPLETE;
                    restore_motion_params();
                    logInfo("Navigation complete after stopping, distance: %.2f", final_distance);
                } else {
                    g_nav_core->state = NAV_STATE_RUNNING;
                    g_nav_core->current_speed_magnitude = 0.0f;
                    g_nav_core->arrive_check_count = 0;
                    // 恢复运行时重新分配一次超时窗口，防止多次启停累计触发超时
                    g_nav_core->arrive_timeout_tick = osKernelGetTickCount() +
                        g_nav_core->cached_target->arrive.timeout_ms;
                    logInfo("Resuming navigation, final distance: %.2f", final_distance);
                }
            }
        }
    }
}
