/**
 * @file motion_control.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 运动控制模块头文件
 */

#ifndef __MOTION_CONTROL_H__
#define __MOTION_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "chassis.h"


bool MotionControl_Init(void);
void MotionControl_GetMotionParams(float *linear_speed, float *yaw_speed, float *acc, float *dec);
void MotionControl_SetMotionParams(float linear_speed, float yaw_speed, float acc, float dec);
void MotionControl_SetPosition(float x_offset, float y_offset, float yaw_offset);
void MotionControl_SetVelocity(float x_component, float y_component, float yaw_component);
void MotionControl_Stop(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MOTION_CONTROL_H__ */
