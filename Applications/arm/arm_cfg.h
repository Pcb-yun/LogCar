#ifndef __ARM_CFG_H__
#define __ARM_CFG_H__

#define ARM_ACTION_INTERVAL_MS 2000

/**
 * @brief 机械臂模块提升舵机的参数
 */
#define ARM_LIFT_SERVO_ID 3             // 提升舵机ID

#define ARM_LIFT_ACC 0.0f              // 提升舵机加速度
#define ARM_LIFT_DEC 0.0f              // 提升舵机减速度

#define ARM_LIFT_INIT_ANGLE 136.0f       // 初始角度

#define ARM_LIFT_MIN_ANGLE 227.4f         // 最低角度
#define ARM_LIFT_MAX_ANGLE 62.1f       // 最高角度


/**
 * @brief 机械臂模块翻转舵机的参数
 */
#define ARM_FLIP_SERVO_ID 2             // 翻转舵机ID

#define ARM_FLIP_ACC 0.0f              // 翻转舵机加速度
#define ARM_FLIP_DEC 0.0f              // 翻转舵机减速度

#define ARM_FLIP_INIT_ANGLE -82.0f        // 初始角度

#define ARM_FLIP_MIN_ANGLE -82.0f         // 收回角度
#define ARM_FLIP_MAX_ANGLE -169.0f       // 水平角度

#endif
