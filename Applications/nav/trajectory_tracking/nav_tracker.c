/**
 * @file nav_tracker.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航轨迹跟踪源文件
 */

#include "nav_tracker.h"
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

#if !MOTOR_POS_MODE_TRAPEZOIDAL
    #error "MOTOR_POS_MODE_TRAPEZOIDAL must be enabled"
#endif
#if !MOTOR_VELOCITY_MODE
    #error "MOTOR_VELOCITY_MODE must be enabled"
#endif

/**
 * @brief 导航跟踪状态
 */
typedef struct {
    TrackState_t state;             // 跟踪状态
    TrackPhase_t phase;             // 当前阶段
    uint32_t task_start_tick;       // 任务开始时间戳
    bool cmd_sent;                  // 指令是否已发送
    TargetPoint_t *cached_target;   // 缓存的目标点
    float saved_linear_speed;       // 保存的原始线速度
    float saved_yaw_speed;          // 保存的原始角速度
    float saved_acc;                // 保存的原始加速度
    float saved_dec;                // 保存的原始减速度
} NavTracker_t;


static NavTracker_t *g_tracker = NULL;
static bool is_init = false;
extern MotorStatusShared_t *g_motor_status;


/**
 * @brief 检查四轮电机是否全部到位
 * @return true-全部到位，false-未到位
 */
static bool motors_reached_target() {
    extern osMutexId_t Motor_MutexHandle;

    if (osMutexAcquire(Motor_MutexHandle, osWaitForever) == osOK) {
        for (uint8_t i = 0; i < 4; i++) {
            if (!g_motor_status->motors[i].prf) {
                logInfo("Motor %d not reached target", i);
                return false;
            }
        }
        osMutexRelease(Motor_MutexHandle);
    }

    return true;
}

/**
 * @brief 获取当前位姿
 * @param pose 当前位姿结构体指针
 * @return true-成功，false-失败
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
 * @param new_phase 新的跟踪阶段
 */
static void enter_phase(TrackPhase_t new_phase) {
    g_tracker->phase = new_phase;
    g_tracker->cmd_sent = false;
}

/**
 * @brief 恢复原始运动参数
 */
static void restore_motion_params(void) {
    MotionControl_SetMotionParams(g_tracker->saved_linear_speed,
                                   g_tracker->saved_yaw_speed,
                                   g_tracker->saved_acc,
                                   g_tracker->saved_dec);
}

/**
 * @brief 停止并报错
 */
static void nav_error(void) {
    MotionControl_Stop();
    restore_motion_params();
    g_tracker->state = TRACK_STATE_ERROR;
    g_tracker->phase = TRACK_PHASE_IDLE;
}

/**
 * @brief 导航跟踪初始化
 * @return 初始化状态
 */
bool Nav_Track_Init(void) {
    g_tracker = pvPortMalloc(sizeof(NavTracker_t));
    if (g_tracker == NULL) {
        return false;
    }
    memset(g_tracker, 0, sizeof(NavTracker_t));

    g_tracker->state = TRACK_STATE_IDLE;
    g_tracker->phase = TRACK_PHASE_IDLE;
    is_init = true;

    return true;
}

/**
 * @brief 导航到指定目标点
 * @param target_id 目标点ID
 * @return 导航状态
 */
bool Nav_Track_GoTo(uint8_t target_id) {
    if (!is_init) return false;
    TargetPoint_t *target = Map_GetPoint(target_id);
    if (target == NULL) return false;

    MotionControl_GetMotionParams(&g_tracker->saved_linear_speed,
                                   &g_tracker->saved_yaw_speed,
                                   &g_tracker->saved_acc,
                                   &g_tracker->saved_dec);

    MotionControl_SetMotionParams(target->motion.target_speed,
                                   target->motion.target_angular_speed,
                                   target->motion.acceleration,
                                   target->motion.deceleration);

    g_tracker->cached_target = target;
    g_tracker->state = TRACK_STATE_RUNNING;
    g_tracker->task_start_tick = osKernelGetTickCount();
    enter_phase(TRACK_PHASE_ROTATE_TO_TARGET);
    return true;
}

/**
 * @brief 停止导航
 */
void Nav_Track_Stop(void) {
    MotionControl_Stop();
    restore_motion_params();
    g_tracker->state = TRACK_STATE_IDLE;
    g_tracker->phase = TRACK_PHASE_IDLE;
    g_tracker->cmd_sent = false;
}

/**
 * @brief 获取当前状态
 * @return 当前导航状态枚举值
 */
TrackState_t Nav_Track_GetState(void) {
    return g_tracker->state;
}

/**
 * @brief 检查并处理平移到位
 * @return true-已处理(完成或进入下一阶段), false-继续等待
 */
static bool check_translate_arrival(Pose2D_t *current_pose) {
    float dx = g_tracker->cached_target->pose.x - current_pose->x;
    float dy = g_tracker->cached_target->pose.y - current_pose->y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (dist < g_tracker->cached_target->arrive.distance_threshold) {
        if (g_tracker->cached_target->arrive.check_mode == ARRIVE_CHECK_BOTH ||
            g_tracker->cached_target->arrive.check_mode == ARRIVE_CHECK_YAW) {
            enter_phase(TRACK_PHASE_ADJUST_YAW);
        } else {
            restore_motion_params();
            g_tracker->state = TRACK_STATE_COMPLETE;
            g_tracker->phase = TRACK_PHASE_IDLE;
        }
        return true;
    }
    return false;
}

/**
 * @brief 检查并处理航向到位
 * @return true-已处理(完成或重新进入), false-继续等待
 */
static bool check_yaw_arrival(Pose2D_t *current_pose) {
    float final_yaw_error = normalize_angle(g_tracker->cached_target->pose.yaw - current_pose->yaw);

    if (fabsf(final_yaw_error) < g_tracker->cached_target->arrive.yaw_threshold) {
        restore_motion_params();
        g_tracker->state = TRACK_STATE_COMPLETE;
        g_tracker->phase = TRACK_PHASE_IDLE;
        return true;
    }
    return false;
}

/**
 * @brief 偏差修正函数
 * @param current_pose 当前位姿
 * @return true-已修正并重新发布指令，false-无需修正
 */
static bool correct_deviation(Pose2D_t *current_pose) {
    float dx = g_tracker->cached_target->pose.x - current_pose->x;
    float dy = g_tracker->cached_target->pose.y - current_pose->y;
    float yaw_error = normalize_angle(g_tracker->cached_target->pose.yaw - current_pose->yaw);

    if (fabsf(yaw_error) < g_tracker->cached_target->arrive.yaw_threshold &&
        sqrtf(dx * dx + dy * dy) < g_tracker->cached_target->arrive.distance_threshold) {
        logInfo("Deviation pass");
        return false;
    }

    float cos_yaw = cosf(current_pose->yaw * DEG_TO_RAD);
    float sin_yaw = sinf(current_pose->yaw * DEG_TO_RAD);
    float x_offset = dx * cos_yaw + dy * sin_yaw;
    float y_offset = -dx * sin_yaw + dy * cos_yaw;

    logInfo("Correcting deviation: x_offset = %f, y_offset = %f, yaw_offset = %f", x_offset, y_offset, yaw_error);
    MotionControl_SetPosition(x_offset, y_offset, yaw_error);
    g_tracker->cmd_sent = true;
    return true;
}

/**
 * @brief 导航轨迹跟踪任务
 */
void Nav_Track_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!is_init) vTaskDelete(NULL);

    Pose2D_t current_pose;

    for (;;) {
        if (g_tracker->state != TRACK_STATE_RUNNING) {
            osDelay(10); continue;
        }

        if (g_tracker->cached_target == NULL) {
            nav_error(); continue;
        }

        if (!get_current_pose(&current_pose)) {
            nav_error(); continue;
        }

        uint32_t elapsed = osKernelGetTickCount() - g_tracker->task_start_tick;
        if (elapsed > g_tracker->cached_target->arrive.timeout_ms) {
            nav_error(); continue;
        }

        switch (g_tracker->phase) {
            case TRACK_PHASE_ROTATE_TO_TARGET: {
                if (!g_tracker->cmd_sent) {
                    logInfo("1. Rotating to target");
                    float dx = g_tracker->cached_target->pose.x - current_pose.x;
                    float dy = g_tracker->cached_target->pose.y - current_pose.y;
                    float angle_to_target = atan2f(dy, dx) * RAD_TO_DEG;
                    float yaw_error = normalize_angle(angle_to_target - current_pose.yaw);

                    if (fabsf(yaw_error) < g_tracker->cached_target->arrive.yaw_threshold) {
                        logInfo("Yaw error: %f, pass", yaw_error);
                        logInfo("Enter phase: TRACK_PHASE_TRANSLATE");
                        enter_phase(TRACK_PHASE_TRANSLATE);
                    } else {
                        logInfo("Yaw error: %f", yaw_error);
                        MotionControl_SetPosition(0.0f, 0.0f, yaw_error);
                        g_tracker->cmd_sent = true;
                    }
                } else if (motors_reached_target()) {
                    if (correct_deviation(&current_pose)) break;
                    enter_phase(TRACK_PHASE_TRANSLATE);
                    logInfo("Enter phase: TRACK_PHASE_TRANSLATE");
                }
                break;
            }

//             case TRACK_PHASE_TRANSLATE: {
//                 if (!g_tracker->cmd_sent) {
//                     logInfo("2. Translating to target");
//                     if (check_translate_arrival(&current_pose)) {
//                         break;
//                     }
//
//                     float dx = g_tracker->cached_target->pose.x - current_pose.x;
//                     float dy = g_tracker->cached_target->pose.y - current_pose.y;
//                     float cos_yaw = cosf(current_pose.yaw * DEG_TO_RAD);
//                     float sin_yaw = sinf(current_pose.yaw * DEG_TO_RAD);
//                     float x_offset = dx * cos_yaw + dy * sin_yaw;
//                     float y_offset = -dx * sin_yaw + dy * cos_yaw;
//
//                     logInfo("X offset: %f, Y offset: %f", x_offset, y_offset);
//                     MotionControl_SetPosition(x_offset, y_offset, 0.0f);
//                     g_tracker->cmd_sent = true;
//                 } else if (motors_reached_target()) {
//                     if (check_translate_arrival(&current_pose)) {
//                         break;
//                     }
//                     if (correct_deviation(&current_pose)) break;
//                     enter_phase(TRACK_PHASE_ADJUST_YAW);
//                 }
//                 break;
//             }
//
//             case TRACK_PHASE_ADJUST_YAW: {
//                 if (!g_tracker->cmd_sent) {
//                     logInfo("3. Adjusting yaw");
//                     if (check_yaw_arrival(&current_pose)) {
//                         break;
//                     }
//
//                     float final_yaw_error = normalize_angle(g_tracker->cached_target->pose.yaw - current_pose.yaw);
//                     MotionControl_SetPosition(0.0f, 0.0f, final_yaw_error);
//                     g_tracker->cmd_sent = true;
//                 } else if (motors_reached_target()) {
//                     if (check_yaw_arrival(&current_pose)) {
//                         break;
//                     }
//                     if (correct_deviation(&current_pose)) break;
//                     logInfo("deviation corrected");
//                 }
//                 break;
//             }
        }
        osDelay(1);
    }
}
