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

#include "chassis.h"

/**
 * @brief 位姿增量
 */
typedef struct {
    float dx;                 // X方向位移增量 (cm)
    float dy;                 // Y方向位移增量 (cm)
    float dyaw;               // 航向角增量 (deg)
} PoseDelta_t;

void Kinematics_Inverse(float vx, float vy, float w, Wheel_t *wheels);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KINEMATICS_H__ */
