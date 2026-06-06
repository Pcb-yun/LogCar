/**
 * @file nav_common.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航类型公共头文件
 */

#ifndef __NAV_COMMON_H__
#define __NAV_COMMON_H__

#include "stdint.h"


/**
 * @brief 导航轨迹数据结构体
 * */
typedef struct {
    float x;              // X坐标 (cm)
    float y;              // Y坐标 (cm)
    float yaw;            // 航向角 (deg)
} TRAJData_t;

/**
 * @brief 导航轨迹状态枚举
 * */
typedef enum {
    TRACK_STATE_IDLE = 0,    // 空闲
    TRACK_STATE_RUNNING,     // 跟踪中
    TRACK_STATE_PAUSED,      // 暂停
    TRACK_STATE_COMPLETE,    // 完成
    TRACK_STATE_ERROR        // 错误
} TrackState_t;

#endif /* __NAV_COMMON_H__ */
