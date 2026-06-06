/**
 * @file nav_tracker.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航轨迹跟踪源文件
 */

#include "nav_tracker.h"
#include "nav_local.h"
#include "nav_math.h"
#include "nav_map.h"
#include "motion_control.h"
#include "step_ttl.h"
#include "cmsis_os2.h"
#include "Events.h"
#include <math.h>
#include <string.h>


/**
 * @brief 导航跟踪状态
 */
typedef struct {
    TrackState_t state;         // 跟踪状态
    TrackPhase_t phase;         // 当前阶段
    uint8_t target_id;          // 目标目标点ID
    uint32_t task_start_tick;   // 任务开始时间戳（总超时用）
    uint32_t task_elapsed_ms;   // 暂停前已用的任务时间（毫秒）
    uint32_t phase_start_tick;  // 阶段开始时间戳
    bool cmd_sent;              // 指令是否已发送
    TargetPoint_t *cached_target; // 缓存的目标点
    Pose2D_t start_pose;        // 当前阶段的起始位姿
    float saved_linear_speed;     // 保存的原始线速度
    float saved_yaw_speed;        // 保存的原始角速度
    float saved_acc;             // 保存的原始加速度
    float saved_dec;             // 保存的原始减速度
} NavTracker_t;


static NavTracker_t g_tracker = {0};
static bool is_init = false;

extern MotorStatusShared_t *g_motor_status;


/**
 * @brief 检查四轮电机是否全部到位
 * @param threshold 位置误差阈值（原始值）
 * @return true-全部到位，false-未到位
 */
static bool motors_reached_target(uint16_t threshold) {
    for (uint8_t i = 0; i < 4; i++) {
        int32_t err = g_motor_status->motors[i].pos_error;
        if (err < 0) err = -err;
        if ((uint32_t)err > threshold) return false;
    }
    return true;
}

/**
 * @brief 获取当前位置误差阈值（原始值）
 * @return 位置误差阈值
 */
static uint16_t get_pos_error_threshold(void) {
    // 电机位置误差阈值：yaw_threshold (deg) × 100 = 原始值
    return (uint16_t)(g_tracker.cached_target->arrive.yaw_threshold * 100.0f);
}

/**
 * @brief 获取当前位姿
 */
static bool get_current_pose(Pose2D_t *pose) {
    Pose_t p;
    if (!Loc_Get(&p)) return false;
    pose->x = p.x;
    pose->y = p.y;
    pose->yaw = p.yaw;
    return true;
}

/**
 * @brief 进入新的跟踪阶段
 */
static void enter_phase(TrackPhase_t new_phase) {
    g_tracker.phase = new_phase;
    g_tracker.cmd_sent = false;
    g_tracker.phase_start_tick = osKernelGetTickCount();
    get_current_pose(&g_tracker.start_pose);
}

/**
 * @brief 停止并报错
 */
static void nav_error(void) {
    MotionControl_Stop();
    restore_motion_params();
    g_tracker.state = TRACK_STATE_ERROR;
    g_tracker.phase = TRACK_PHASE_IDLE;
}

/**
 * @brief 导航跟踪初始化
 */
static bool Nav_Track_Init(void) {
    memset(&g_tracker, 0, sizeof(NavTracker_t));
    g_tracker.state = TRACK_STATE_IDLE;
    g_tracker.phase = TRACK_PHASE_IDLE;
    is_init = true;
    return true;
}

/**
 * @brief 导航到指定目标点
 */
bool Nav_Track_GoTo(uint8_t target_id) {
    if (!is_init) return false;
    TargetPoint_t *target = Map_GetPoint(target_id);
    if (target == NULL) return false;

    // 保存原始运动参数
    MotionControl_GetMotionParams(&g_tracker.saved_linear_speed, 
                                   &g_tracker.saved_yaw_speed, 
                                   &g_tracker.saved_acc, 
                                   &g_tracker.saved_dec);
    
    // 应用目标点的运动参数
    MotionControl_SetMotionParams(target->motion.target_speed, 
                                   target->motion.target_angular_speed, 
                                   target->motion.acceleration, 
                                   target->motion.deceleration);

    g_tracker.target_id = target_id;
    g_tracker.cached_target = target;
    g_tracker.state = TRACK_STATE_RUNNING;
    g_tracker.task_start_tick = osKernelGetTickCount();
    g_tracker.task_elapsed_ms = 0;
    enter_phase(TRACK_PHASE_ROTATE_TO_TARGET);
    return true;
}

/**
 * @brief 暂停导航
 */
bool Nav_Track_Pause(void) {
    if (g_tracker.state != TRACK_STATE_RUNNING) return false;
    g_tracker.state = TRACK_STATE_PAUSED;
    g_tracker.task_elapsed_ms = osKernelGetTickCount() - g_tracker.task_start_tick;
    MotionControl_Stop();
    g_tracker.cmd_sent = false;
    return true;
}

/**
 * @brief 恢复导航
 */
bool Nav_Track_Resume(void) {
    if (g_tracker.state != TRACK_STATE_PAUSED) return false;
    g_tracker.state = TRACK_STATE_RUNNING;
    // 恢复时更新 task_start_tick，避免暂停期间的时间计入超时
    g_tracker.task_start_tick = osKernelGetTickCount() - g_tracker.task_elapsed_ms;
    enter_phase(g_tracker.phase);
    return true;
}

/**
 * @brief 恢复原始运动参数
 */
static void restore_motion_params(void) {
    MotionControl_SetMotionParams(g_tracker.saved_linear_speed, 
                                   g_tracker.saved_yaw_speed, 
                                   g_tracker.saved_acc, 
                                   g_tracker.saved_dec);
}

/**
 * @brief 停止导航
 */
bool Nav_Track_Stop(void) {
    if (g_tracker.state == TRACK_STATE_IDLE) return false;
    MotionControl_Stop();
    restore_motion_params();
    g_tracker.state = TRACK_STATE_IDLE;
    g_tracker.phase = TRACK_PHASE_IDLE;
    g_tracker.cmd_sent = false;
    return true;
}

/**
 * @brief 获取当前状态
 */
TrackState_t Nav_Track_GetState(void) {
    return g_tracker.state;
}

/**
 * @brief 导航轨迹跟踪任务
 */
void Nav_Track_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!Nav_Track_Init()) return;

    Pose2D_t current_pose;

    for (;;) {
        if (g_tracker.state != TRACK_STATE_RUNNING) {
            osDelay(10);
            continue;
        }

        if (g_tracker.cached_target == NULL) {
            nav_error();
            osDelay(10);
            continue;
        }

        // 获取当前位姿，失败则报错退出
        if (!get_current_pose(&current_pose)) {
            nav_error();
            osDelay(10);
            continue;
        }

        // 超时检查（从任务开始计时）
        uint32_t elapsed = osKernelGetTickCount() - g_tracker.task_start_tick;
        if (elapsed > g_tracker.cached_target->arrive.timeout_ms) {
            nav_error();
            osDelay(10);
            continue;
        }

        // 阶段处理
        switch (g_tracker.phase) {
            case TRACK_PHASE_ROTATE_TO_TARGET: {
                if (!g_tracker.cmd_sent) {
                    float dx = g_tracker.cached_target->pose.x - current_pose.x;
                    float dy = g_tracker.cached_target->pose.y - current_pose.y;
                    float angle_to_target = atan2f(dy, dx) * RAD_TO_DEG;
                    float yaw_error = normalize_angle(angle_to_target - current_pose.yaw);

                    if (fabsf(yaw_error) < g_tracker.cached_target->arrive.yaw_threshold) {
                        enter_phase(TRACK_PHASE_TRANSLATE);
                    } else {
                        MotionControl_SetPosition(0.0f, 0.0f, yaw_error);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位
                    if (motors_reached_target(get_pos_error_threshold())) {
                        enter_phase(TRACK_PHASE_TRANSLATE);
                    }
                }
                break;
            }

            case TRACK_PHASE_TRANSLATE: {
                if (!g_tracker.cmd_sent) {
                    float dx = g_tracker.cached_target->pose.x - current_pose.x;
                    float dy = g_tracker.cached_target->pose.y - current_pose.y;
                    float dist = sqrtf(dx * dx + dy * dy);

                    if (dist < g_tracker.cached_target->arrive.distance_threshold) {
                        if (g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_BOTH ||
                            g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_YAW) {
                            enter_phase(TRACK_PHASE_ADJUST_YAW);
                        } else {
                            restore_motion_params();
                            g_tracker.state = TRACK_STATE_COMPLETE;
                            g_tracker.phase = TRACK_PHASE_IDLE;
                        }
                    } else {
                        float cos_yaw = cosf(current_pose.yaw * DEG_TO_RAD);
                        float sin_yaw = sinf(current_pose.yaw * DEG_TO_RAD);
                        float x_offset = dx * cos_yaw + dy * sin_yaw;
                        float y_offset = -dx * sin_yaw + dy * cos_yaw;

                        MotionControl_SetPosition(x_offset, y_offset, 0.0f);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位，然后用定位数据验证
                    if (motors_reached_target(get_pos_error_threshold())) {
                        float dx = g_tracker.cached_target->pose.x - current_pose.x;
                        float dy = g_tracker.cached_target->pose.y - current_pose.y;
                        float dist = sqrtf(dx * dx + dy * dy);

                        if (dist < g_tracker.cached_target->arrive.distance_threshold) {
                            if (g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_BOTH ||
                                g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_YAW) {
                                enter_phase(TRACK_PHASE_ADJUST_YAW);
                            } else {
                                restore_motion_params();
                                g_tracker.state = TRACK_STATE_COMPLETE;
                                g_tracker.phase = TRACK_PHASE_IDLE;
                            }
                        } else {
                            enter_phase(TRACK_PHASE_TRANSLATE);
                        }
                    }
                }
                break;
            }

            case TRACK_PHASE_ADJUST_YAW: {
                if (!g_tracker.cmd_sent) {
                    float final_yaw_error = normalize_angle(g_tracker.cached_target->pose.yaw - current_pose.yaw);

                    if (fabsf(final_yaw_error) < g_tracker.cached_target->arrive.yaw_threshold) {
                        restore_motion_params();
                        g_tracker.state = TRACK_STATE_COMPLETE;
                        g_tracker.phase = TRACK_PHASE_IDLE;
                    } else {
                        MotionControl_SetPosition(0.0f, 0.0f, final_yaw_error);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位，然后用定位数据验证
                    if (motors_reached_target(get_pos_error_threshold())) {
                        float final_yaw_error = normalize_angle(g_tracker.cached_target->pose.yaw - current_pose.yaw);
                        if (fabsf(final_yaw_error) < g_tracker.cached_target->arrive.yaw_threshold) {
                            restore_motion_params();
                            g_tracker.state = TRACK_STATE_COMPLETE;
                            g_tracker.phase = TRACK_PHASE_IDLE;
                        } else {
                            enter_phase(TRACK_PHASE_ADJUST_YAW);
                        }
                    }
                }
                break;
            }

            default:
                nav_error();
                break;
        }

        osDelay(10);
    }
}

