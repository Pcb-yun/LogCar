/**
 * @file chassis.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 车体控制模块
 */

#ifndef __CHASSIS_H__
#define __CHASSIS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/**
 * @brief 车体位姿结构体
 */
typedef struct {
    float x;                // X坐标 (cm)
    float y;                // Y坐标 (cm)
    float yaw;              // 航向角 (deg)
    uint32_t timestamp;     // 时间戳 (ms)
} Pose_t;

/**
 * @brief 轮子控制结构体
 */
typedef struct {
    float fl;       // 前左轮
    float fr;       // 前右轮
    float rr;       // 后右轮
    float rl;       // 后左轮
} Wheel_t;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CHASSIS_H__ */
