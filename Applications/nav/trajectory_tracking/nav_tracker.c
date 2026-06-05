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
    uint32_t phase_start_tick;  // 阶段开始时间戳
    bool cmd_sent;              // 指令是否已发送
} NavTracker_t;


static NavTracker_t g_tracker = {0};
static bool is_init = false;

extern MotorStatusShared_t *g_motor_status;


/**
 * @brief 检查四轮电机是否全部到位
 * @param threshold 位置误差阈值(原始值)
 * @return true-全部到位, false-未到位
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
 * @brief 计算到目标点的距离和航向
 */
static void calc_target_info(Pose2D_t current, TargetPose_t target,
                              float *dist, float *angle_to_target, float *yaw_error) {
    float dx = target.x - current.x;
    float dy = target.y - current.y;
    *dist = sqrtf(dx * dx + dy * dy);
    *angle_to_target = atan2f(dy, dx);
    *yaw_error = normalize_angle(*angle_to_target - current.yaw);
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
    if (Map_GetPoint(target_id) == NULL) return false;

    g_tracker.target_id = target_id;
    g_tracker.state = TRACK_STATE_RUNNING;
    g_tracker.phase = TRACK_PHASE_ROTATE_TO_TARGET;
    g_tracker.cmd_sent = false;
    g_tracker.phase_start_tick = osKernelGetTickCount();
    return true;
}

/**
 * @brief 暂停导航
 */
bool Nav_Track_Pause(void) {
    if (g_tracker.state != TRACK_STATE_RUNNING) return false;
    g_tracker.state = TRACK_STATE_PAUSED;
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
    g_tracker.cmd_sent = false; // 恢复后重新发指令
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
    TargetPoint_t *target = NULL;

    for (;;) {
        if (g_tracker.state != TRACK_STATE_RUNNING) {
            osDelay(10);
            continue;
        }

        target = Map_GetPoint(g_tracker.target_id);
        if (target == NULL) {
            g_tracker.state = TRACK_STATE_ERROR;
            osDelay(10);
            continue;
        }

        if (!get_current_pose(&current_pose)) {
            osDelay(10);
            continue;
        }

        float dist, angle_to_target, yaw_error;
        calc_target_info(current_pose, target->pose, &dist, &angle_to_target, &yaw_error);

        // 超时检查
        uint32_t elapsed = osKernelGetTickCount() - g_tracker.phase_start_tick;
        if (elapsed > target->arrive.timeout_ms) {
            g_tracker.state = TRACK_STATE_ERROR;
            MotionControl_Stop();
            osDelay(10);
            continue;
        }

        // 阶段处理
        switch (g_tracker.phase) {
            case TRACK_PHASE_ROTATE_TO_TARGET: {
                if (!g_tracker.cmd_sent) {
                    if (fabsf(yaw_error) < target->arrive.yaw_threshold) {
                        // 已到位，直接进入平移阶段
                        g_tracker.phase = TRACK_PHASE_TRANSLATE;
                        g_tracker.cmd_sent = false;
                        g_tracker.phase_start_tick = osKernelGetTickCount();
                    } else {
                        // 发一次旋转指令，然后等待到位
                        int32_t yaw_deg = (int32_t)(yaw_error * RAD_TO_DEG);
                        MotionControl_SetPosition(0, 0, yaw_deg);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位
                    if (motors_reached_target(target->arrive.yaw_threshold * 100.0f)) {
                        g_tracker.phase = TRACK_PHASE_TRANSLATE;
                        g_tracker.cmd_sent = false;
                        g_tracker.phase_start_tick = osKernelGetTickCount();
                    }
                }
                break;
            }

            case TRACK_PHASE_TRANSLATE: {
                if (!g_tracker.cmd_sent) {
                    float dx = target->pose.x - current_pose.x;
                    float dy = target->pose.y - current_pose.y;
                    dist = sqrtf(dx * dx + dy * dy);

                    if (dist < target->arrive.distance_threshold) {
                        // 已到位，检查是否需要调整航向
                        if (target->arrive.check_mode == ARRIVE_CHECK_BOTH ||
                            target->arrive.check_mode == ARRIVE_CHECK_YAW) {
                            g_tracker.phase = TRACK_PHASE_ADJUST_YAW;
                        } else {
                            g_tracker.state = TRACK_STATE_COMPLETE;
                            g_tracker.phase = TRACK_PHASE_IDLE;
                        }
                        g_tracker.cmd_sent = false;
                        g_tracker.phase_start_tick = osKernelGetTickCount();
                    } else {
                        // 发一次平移指令，然后等待到位
                        float cos_yaw = cosf(current_pose.yaw);
                        float sin_yaw = sinf(current_pose.yaw);
                        int32_t x_offset = (int32_t)(dx * cos_yaw + dy * sin_yaw);
                        int32_t y_offset = (int32_t)(-dx * sin_yaw + dy * cos_yaw);

                        MotionControl_SetPosition(x_offset, y_offset, 0);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位
                    if (motors_reached_target(100)) { // 100原始值约等于0.3°误差
                        g_tracker.phase = TRACK_PHASE_TRANSLATE;
                        g_tracker.cmd_sent = false;
                        g_tracker.phase_start_tick = osKernelGetTickCount();
                    }
                }
                break;
            }

            case TRACK_PHASE_ADJUST_YAW: {
                if (!g_tracker.cmd_sent) {
                    float final_yaw_error = normalize_angle(target->pose.yaw - current_pose.yaw);

                    if (fabsf(final_yaw_error) < target->arrive.yaw_threshold) {
                        g_tracker.state = TRACK_STATE_COMPLETE;
                        g_tracker.phase = TRACK_PHASE_IDLE;
                    } else {
                        int32_t yaw_deg = (int32_t)(final_yaw_error * RAD_TO_DEG);
                        MotionControl_SetPosition(0, 0, yaw_deg);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位
                    if (motors_reached_target(target->arrive.yaw_threshold * 100.0f)) {
                        g_tracker.state = TRACK_STATE_COMPLETE;
                        g_tracker.phase = TRACK_PHASE_IDLE;
                    }
                }
                break;
            }

            default:
                g_tracker.phase = TRACK_PHASE_ROTATE_TO_TARGET;
                g_tracker.cmd_sent = false;
                break;
        }

        osDelay(10);
    }
}
