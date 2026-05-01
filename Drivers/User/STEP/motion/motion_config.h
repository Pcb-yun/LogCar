/**
 * @file motion_config.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 运动控制配置文件
 */

#ifndef __MOTION_CONFIG_H__
#define __MOTION_CONFIG_H__


// 麦轮小车机械参数
#define WHEEL_RADIUS       5.0f     // 轮子半径 (cm)
#define WHEEL_BASE_WIDTH   25.0f    // 轮距宽度 (cm)
#define WHEEL_BASE_LENGTH  25.0f    // 轴距长度 (cm)

// 电机ID定义
#define MOTOR_FRONT_LEFT   1	// 前左轮
#define MOTOR_FRONT_RIGHT  2	// 前右轮
#define MOTOR_BACK_RIGHT   3	// 后右轮
#define MOTOR_BACK_LEFT    4	// 后左轮

// 电机脉冲参数
#define MOTOR_PULSES_PER_REV    3200	// 电机每转脉冲数 (200步 * 16细分)


#endif /* __MOTION_CONFIG_H__ */
