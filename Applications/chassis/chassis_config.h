/**
 * @file chassis_config.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 麦轮小车机械参数配置
 */

#ifndef __CHASSIS_CONFIG_H__
#define __CHASSIS_CONFIG_H__

// 麦轮小车机械参数
#define WHEEL_RADIUS       3.65f    // 轮子半径 (cm)
#define WHEEL_BASE_WIDTH   17.0f    // 轮距宽度 (cm)
#define WHEEL_BASE_LENGTH  17.85f   // 轴距长度 (cm)

// 电机ID定义
#define MOTOR_FRONT_LEFT   1	// 前左轮
#define MOTOR_FRONT_RIGHT  2	// 前右轮
#define MOTOR_BACK_RIGHT   3	// 后右轮
#define MOTOR_BACK_LEFT    4	// 后左轮

// 电机脉冲参数
#define MOTOR_PULSES_PER_REV    3200	// 电机每转脉冲数


#endif /* __CHASSIS_CONFIG_H__ */
