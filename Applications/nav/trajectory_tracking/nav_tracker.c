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
    TargetPoint_t *cached_target; // 缓存的目标点
    Pose2D_t start_pose;        // 当前阶段的起始位姿
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
 * @brief 进入新的跟踪阶段
 */
static void enter_phase(TrackPhase_t new_phase) {
    g_tracker.phase = new_phase;
    g_tracker.cmd_sent = false;
    g_tracker.phase_start_tick = osKernelGetTickCount();
    get_current_pose(&g_tracker.start_pose);
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

    g_tracker.target_id = target_id;
    g_tracker.cached_target = target;
    g_tracker.state = TRACK_STATE_RUNNING;
    enter_phase(TRACK_PHASE_ROTATE_TO_TARGET);
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
    enter_phase(g_tracker.phase);
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
            g_tracker.state = TRACK_STATE_ERROR;
            osDelay(10);
            continue;
        }

        // 超时检查
        uint32_t elapsed = osKernelGetTickCount() - g_tracker.phase_start_tick;
        if (elapsed > g_tracker.cached_target->arrive.timeout_ms) {
            g_tracker.state = TRACK_STATE_ERROR;
            MotionControl_Stop();
            osDelay(10);
            continue;
        }

        // 阶段处理
        switch (g_tracker.phase) {
            case TRACK_PHASE_ROTATE_TO_TARGET: {
                if (!g_tracker.cmd_sent) {
                    if (!get_current_pose(&current_pose)) break;
                    float dx = g_tracker.cached_target->pose.x - current_pose.x;
                    float dy = g_tracker.cached_target->pose.y - current_pose.y;
                    float angle_to_target = atan2f(dy, dx);
                    float yaw_error = normalize_angle(angle_to_target - current_pose.yaw);

                    if (fabsf(yaw_error) < g_tracker.cached_target->arrive.yaw_threshold) {
                        enter_phase(TRACK_PHASE_TRANSLATE);
                    } else {
                        int32_t yaw_deg = (int32_t)(yaw_error * RAD_TO_DEG);
                        MotionControl_SetPosition(0, 0, yaw_deg);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位
                    if (motors_reached_target(g_tracker.cached_target->arrive.yaw_threshold * 100.0f)) {
                        enter_phase(TRACK_PHASE_TRANSLATE);
                    }
                }
                break;
            }

            case TRACK_PHASE_TRANSLATE: {
                if (!g_tracker.cmd_sent) {
                    if (!get_current_pose(&current_pose)) break;
                    float dx = g_tracker.cached_target->pose.x - current_pose.x;
                    float dy = g_tracker.cached_target->pose.y - current_pose.y;
                    float dist = sqrtf(dx * dx + dy * dy);

                    if (dist < g_tracker.cached_target->arrive.distance_threshold) {
                        if (g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_BOTH ||
                            g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_YAW) {
                            enter_phase(TRACK_PHASE_ADJUST_YAW);
                        } else {
                            g_tracker.state = TRACK_STATE_COMPLETE;
                            g_tracker.phase = TRACK_PHASE_IDLE;
                        }
                    } else {
                        float cos_yaw = cosf(current_pose.yaw);
                        float sin_yaw = sinf(current_pose.yaw);
                        int32_t x_offset = (int32_t)(dx * cos_yaw + dy * sin_yaw);
                        int32_t y_offset = (int32_t)(-dx * sin_yaw + dy * cos_yaw);

                        MotionControl_SetPosition(x_offset, y_offset, 0);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位，然后用定位数据验证
                    if (motors_reached_target(100)) {
                        if (!get_current_pose(&current_pose)) break;
                        float dx = g_tracker.cached_target->pose.x - current_pose.x;
                        float dy = g_tracker.cached_target->pose.y - current_pose.y;
                        float dist = sqrtf(dx * dx + dy * dy);

                        if (dist < g_tracker.cached_target->arrive.distance_threshold) {
                            if (g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_BOTH ||
                                g_tracker.cached_target->arrive.check_mode == ARRIVE_CHECK_YAW) {
                                enter_phase(TRACK_PHASE_ADJUST_YAW);
                            } else {
                                g_tracker.state = TRACK_STATE_COMPLETE;
                                g_tracker.phase = TRACK_PHASE_IDLE;
                            }
                        } else {
                            enter_phase(TRACK_PHASE_TRANSLATE); // 有偏差，重新发指令修正
                        }
                    }
                }
                break;
            }

            case TRACK_PHASE_ADJUST_YAW: {
                if (!g_tracker.cmd_sent) {
                    if (!get_current_pose(&current_pose)) break;
                    float final_yaw_error = normalize_angle(g_tracker.cached_target->pose.yaw - current_pose.yaw);

                    if (fabsf(final_yaw_error) < g_tracker.cached_target->arrive.yaw_threshold) {
                        g_tracker.state = TRACK_STATE_COMPLETE;
                        g_tracker.phase = TRACK_PHASE_IDLE;
                    } else {
                        int32_t yaw_deg = (int32_t)(final_yaw_error * RAD_TO_DEG);
                        MotionControl_SetPosition(0, 0, yaw_deg);
                        g_tracker.cmd_sent = true;
                    }
                } else {
                    // 等待电机到位，然后用定位数据验证
                    if (motors_reached_target(g_tracker.cached_target->arrive.yaw_threshold * 100.0f)) {
                        if (!get_current_pose(&current_pose)) break;
                        float final_yaw_error = normalize_angle(g_tracker.cached_target->pose.yaw - current_pose.yaw);
                        if (fabsf(final_yaw_error) < g_tracker.cached_target->arrive.yaw_threshold) {
                            g_tracker.state = TRACK_STATE_COMPLETE;
                            g_tracker.phase = TRACK_PHASE_IDLE;
                        } else {
                            enter_phase(TRACK_PHASE_ADJUST_YAW); // 有偏差，重新修正
                        }
                    }
                }
                break;
            }

            default:
                enter_phase(TRACK_PHASE_ROTATE_TO_TARGET);
                break;
        }

        osDelay(10);
    }
}
