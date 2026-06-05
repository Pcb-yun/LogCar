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
} NavTracker_t;


static NavTracker_t g_tracker = {0};
static bool is_init = false;


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
    return true;
}

/**
 * @brief 恢复导航
 */
bool Nav_Track_Resume(void) {
    if (g_tracker.state != TRACK_STATE_PAUSED) return false;
    g_tracker.state = TRACK_STATE_RUNNING;
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
                // 旋转朝向目标方向
                if (fabsf(yaw_error) < target->arrive.yaw_threshold) {
                    MotionControl_Stop();
                    g_tracker.phase = TRACK_PHASE_TRANSLATE;
                    g_tracker.phase_start_tick = osKernelGetTickCount();
                } else {
                    int32_t yaw_deg = (int32_t)(yaw_error * RAD_TO_DEG);
                    int32_t yaw_clamp = yaw_deg;
                    // 限制最大偏摆角度避免过冲
                    if (yaw_clamp > (int32_t)(target->motion.target_angular_speed * 0.2f)) {
                        yaw_clamp = (int32_t)(target->motion.target_angular_speed * 0.2f);
                    } else if (yaw_clamp < -(int32_t)(target->motion.target_angular_speed * 0.2f)) {
                        yaw_clamp = -(int32_t)(target->motion.target_angular_speed * 0.2f);
                    }
                    MotionControl_SetPosition(0, 0, yaw_clamp);
                }
                break;
            }

            case TRACK_PHASE_TRANSLATE: {
                // 平移移动到目标位置
                float decel_dist = 10.0f; // 减速距离(cm)
                float speed_ratio = fminf(dist / decel_dist, 1.0f);

                if (dist < target->arrive.distance_threshold) {
                    MotionControl_Stop();
                    // 检查是否需要调整最终航向
                    if (target->arrive.check_mode == ARRIVE_CHECK_BOTH ||
                        target->arrive.check_mode == ARRIVE_CHECK_YAW) {
                        g_tracker.phase = TRACK_PHASE_ADJUST_YAW;
                        g_tracker.phase_start_tick = osKernelGetTickCount();
                    } else {
                        // 到达目标点
                        g_tracker.state = TRACK_STATE_COMPLETE;
                        g_tracker.phase = TRACK_PHASE_IDLE;
                    }
                } else {
                    // 计算世界坐标系下的运动偏移
                    float dx = target->pose.x - current_pose.x;
                    float dy = target->pose.y - current_pose.y;

                    // 将运动方向投影到车体坐标系
                    float cos_yaw = cosf(current_pose.yaw);
                    float sin_yaw = sinf(current_pose.yaw);
                    int32_t x_offset = (int32_t)((dx * cos_yaw + dy * sin_yaw) * speed_ratio);
                    int32_t y_offset = (int32_t)((-dx * sin_yaw + dy * cos_yaw) * speed_ratio);

                    MotionControl_SetPosition(x_offset, y_offset, 0);
                }
                break;
            }

            case TRACK_PHASE_ADJUST_YAW: {
                // 调整最终航向
                float final_yaw_error = normalize_angle(target->pose.yaw - current_pose.yaw);

                if (fabsf(final_yaw_error) < target->arrive.yaw_threshold) {
                    MotionControl_Stop();
                    // 到达目标点
                    g_tracker.state = TRACK_STATE_COMPLETE;
                    g_tracker.phase = TRACK_PHASE_IDLE;
                } else {
                    int32_t yaw_deg = (int32_t)(final_yaw_error * RAD_TO_DEG);
                    int32_t yaw_clamp = yaw_deg;
                    if (yaw_clamp > 90) yaw_clamp = 90;
                    if (yaw_clamp < -90) yaw_clamp = -90;
                    MotionControl_SetPosition(0, 0, yaw_clamp);
                }
                break;
            }

            default:
                g_tracker.phase = TRACK_PHASE_ROTATE_TO_TARGET;
                break;
        }

        osDelay(10);
    }
}
