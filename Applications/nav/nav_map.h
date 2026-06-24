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
#include "nav_common.h"

/**
 * @brief 目标点类型
 */
typedef enum {
    TARGET_POINT_NORMAL = 0,      // 普通目标点
    TARGET_POINT_PICKUP,          // 取货点
    TARGET_POINT_DELIVERY,        // 放货点
    TARGET_POINT_PAUSE,           // 暂停点
    TARGET_POINT_WAIT             // 等待点
} TargetPointType_t;

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
 * @brief 目标点位姿
 */
typedef struct {
    float x;                     // X坐标 (cm)
    float y;                     // Y坐标 (cm)
    float yaw;                   // 期望航向角 (deg)
} TargetPose_t;

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
    uint8_t id;                  // 目标点ID
    char name[16];               // 目标点名称
    TargetPointType_t type;      // 目标点类型
    TargetPose_t pose;           // 目标位姿
    MotionParams_t motion;       // 运动参数
    ArriveParams_t arrive;       // 到达检测参数
    bool enable;                 // 是否启用
} TargetPoint_t;

/**
 * @brief 地图数据
 */
typedef struct {
    uint8_t max_points;      // 地图最大目标点数
    uint8_t point_count;     // 当前目标点数量
} NavMapInfo_t;



NavMapInfo_t *Map_GetInfo(void);
bool Map_AddPoint(TargetPoint_t *point);
bool Map_RemovePoint(uint8_t id);
bool Map_UpdatePoint(uint8_t id, TargetPoint_t *point);
TargetPoint_t *Map_GetPoint(uint8_t id);
bool Map_LoadPoints(TargetPoint_t *points, uint8_t count);

TargetPoint_t *Map_GetDataPoints(void);
uint8_t Map_GetDataPointCount(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAV_MAP_H__ */
