/**
 * @file kinematics.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 麦克纳姆轮运动学解算模块头文件
 */

#ifndef __KINEMATICS_H__
#define __KINEMATICS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "chassis.h"

/**
 * @brief 轮子编码器数据
 */
typedef struct {
    int32_t front_left;       // 前左轮编码器值
    int32_t front_right;      // 前右轮编码器值
    int32_t rear_left;        // 后左轮编码器值
    int32_t rear_right;       // 后右轮编码器值
    uint32_t timestamp;       // 时间戳
} WheelEncoderData_t;

/**
 * @brief 位姿增量
 */
typedef struct {
    float dx;                 // X方向位移增量 (cm)
    float dy;                 // Y方向位移增量 (cm)
    float dyaw;               // 航向角增量 (deg)
} PoseDelta_t;

/**
 * @brief 正运动学解算
 * @param encoder_delta 编码器增量数据
 * @param pose_delta 位姿增量输出（dyaw单位为度）
 */
void Kinematics_Forward(WheelEncoderData_t *encoder_delta, PoseDelta_t *pose_delta);

/**
 * @brief 逆运动学解算
 * @param vx X方向速度/位移 (cm/s)
 * @param vy Y方向速度/位移 (cm/s)
 * @param w 角速度/角度 (deg/s)
 * @param wheels 四个轮子的输出角速度/角度 (rad/s/rad)
 */
void Kinematics_Inverse(float vx, float vy, float w, Wheel_t *wheels);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KINEMATICS_H__ */

