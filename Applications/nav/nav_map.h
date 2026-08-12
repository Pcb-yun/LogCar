/**
 * @file nav_map.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航地图头文件
 */

#ifndef __NAV_MAP_H__
#define __NAV_MAP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdint.h"
#include "stdbool.h"
#include "nav_math.h"


/**
 * @brief 到达检测方式
 */
typedef enum {
    ARRIVE_CHECK_DISTANCE = 0,  // 基于距离检测
    ARRIVE_CHECK_YAW,           // 基于角度检测
    ARRIVE_CHECK_BOTH           // 距离+角度检测
} ArriveCheckMode_t;

/**
 * @brief 目标点运动参数
 */
typedef struct {
    float target_speed;             // 目标线速度 (cm/s)
    float target_angular_speed;     // 目标角速度 (deg/s)
    float acceleration;             // 加速度 (cm/s²)
    float deceleration;             // 减速度 (cm/s²)
    float angular_acceleration;     // 角加速度 (deg/s²)
} MotionParams_t;

/**
 * @brief 到达检测参数
 */
typedef struct {
    ArriveCheckMode_t check_mode;   // 检测模式
    float distance_threshold;       // 距离阈值 (cm)
    float yaw_threshold;            // 角度阈值 (deg)
    uint16_t timeout_ms;            // 超时时间 (ms)
} ArriveParams_t;

/**
 * @brief 目标点完整结构
 */
typedef struct {
    uint8_t id;                 // 目标点ID
    char name[16];              // 目标点名称
    Pose2D_t pose;              // 目标位姿
    MotionParams_t motion;      // 运动参数
    ArriveParams_t arrive;      // 到达检测参数
} TargetPoint_t;

/**
 * @brief 地图数据
 */
typedef struct {
    uint8_t max_points;      // 地图最大目标点数
    uint8_t point_count;     // 当前目标点数量
} NavMapInfo_t;



NavMapInfo_t *Map_GetInfo(void);
bool Map_AddPoint(const TargetPoint_t *point);
bool Map_RemovePoint(uint8_t id);
bool Map_UpdatePoint(uint8_t id, TargetPoint_t *point);
TargetPoint_t *Map_GetPoint(uint8_t id);
TargetPoint_t *Map_GetPointByName(const char *name);
bool Map_LoadPoints(const TargetPoint_t *points, uint8_t count);

const TargetPoint_t *Map_GetDataPoints(void);
uint8_t Map_GetDataPointCount(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAV_MAP_H__ */
