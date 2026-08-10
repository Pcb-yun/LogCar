#ifndef __ARM_CFG_H__
#define __ARM_CFG_H__

/**
 * @brief 机械臂模块提升舵机的参数
 */
#define ARM_LIFT_SERVO_ID 2             // 提升舵机ID
#define ARM_LIFT_INIT_ANGLE 0.0f        // 初始角度
#define ARM_LIFT_MIN_ANGLE 0.0f         // 最小角度
#define ARM_LIFT_MAX_ANGLE 180.0f       // 最大角度

/**
 * @brief 机械臂模块翻转舵机的参数
 */
#define ARM_FLIP_SERVO_ID 3             // 翻转舵机ID
#define ARM_FLIP_INIT_ANGLE 0.0f        // 初始角度
#define ARM_FLIP_MIN_ANGLE 0.0f         // 最小角度
#define ARM_FLIP_MAX_ANGLE 180.0f       // 最大角度

#endif
