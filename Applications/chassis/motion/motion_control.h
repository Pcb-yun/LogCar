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
bool MotionControl_OdomUpdate(Pose_t *pose);
void MotionControl_GetMotionParams(uint16_t *linear_speed, uint16_t *yaw_speed, uint16_t *acc, uint16_t *dec);
void MotionControl_SetMotionParams(uint16_t linear_speed, uint16_t yaw_speed, uint16_t acc, uint16_t dec);
void MotionControl_SetPosition(int32_t x_offset, int32_t y_offset, int32_t yaw_offset);
void MotionControl_SetVelocity(int8_t x_component, int8_t y_component, int8_t yaw_component);
void MotionControl_Stop(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MOTION_CONTROL_H__ */
