/**
 * @file kinematics_common.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 运动学公共类型定义
 */

#ifndef __KINEMATICS_COMMON_H__
#define __KINEMATICS_COMMON_H__

#include "stdint.h"
#include "stdbool.h"

/**
 * @brief 轮子编号枚举
 */
typedef enum {
    WHEEL_FRONT_LEFT = 0,    // 左前轮
    WHEEL_FRONT_RIGHT,       // 右前轮
    WHEEL_REAR_LEFT,         // 左后轮
    WHEEL_REAR_RIGHT         // 右后轮
} WheelIndex_t;

/**
 * @brief 四个轮子的速度/角度数据
 */
typedef struct {
    float front_left;        // 左前轮
    float front_right;       // 右前轮
    float rear_left;         // 左后轮
    float rear_right;        // 右后轮
} WheelData_t;

/**
 * @brief 四个轮子的编码器数据
 */
typedef struct {
    int32_t front_left;      // 左前轮编码器计数
    int32_t front_right;     // 右前轮编码器计数
    int32_t rear_left;       // 左后轮编码器计数
    int32_t rear_right;      // 右后轮编码器计数
    uint32_t timestamp;      // 时间戳 (ms)
} WheelEncoderData_t;

/**
 * @brief 车体运动状态
 */
typedef struct {
    float vx;                // X方向速度 (cm/s)
    float vy;                // Y方向速度 (cm/s)
    float omega;             // 角速度 (rad/s)
} MotionState_t;

/**
 * @brief 车体位姿增量
 */
typedef struct {
    float dx;                // X方向位移 (cm)
    float dy;                // Y方向位移 (cm)
    float dyaw;              // 航向角变化 (rad)
    float dt;                // 时间间隔 (s)
} PoseDelta_t;

#endif /* __KINEMATICS_COMMON_H__ */
