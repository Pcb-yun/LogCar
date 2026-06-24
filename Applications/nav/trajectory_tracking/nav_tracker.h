/**
 * @file nav_tracker.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航轨迹跟踪头文件
 */

#ifndef __NAV_TRACKER_H__
#define __NAV_TRACKER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdbool.h"
#include "nav_common.h"


/**
 * @brief 导航跟踪阶段
 */
typedef enum {
    TRACK_PHASE_IDLE = 0,           // 空闲
    TRACK_PHASE_ROTATE_TO_TARGET,   // 旋转朝向目标方向
    TRACK_PHASE_TRANSLATE,          // 平移移动到目标
    TRACK_PHASE_ADJUST_YAW,         // 调整最终航向
} TrackPhase_t;


bool Nav_Track_GoTo(uint8_t target_id); // 导航到指定目标点
void Nav_Track_Stop(void);               // 停止导航
TrackState_t Nav_Track_GetState(void);   // 获取当前状态



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAV_TRACKER_H__ */
