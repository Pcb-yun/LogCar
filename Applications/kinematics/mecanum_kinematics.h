/**
 * @file mecanum_kinematics.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 麦克纳姆轮运动学算法接口
 * @details 提供四麦轮小车的正运动学和逆运动学解算
 */

#ifndef __MECANUM_KINEMATICS_H__
#define __MECANUM_KINEMATICS_H__

#include "kinematics_common.h"

/**
 * @brief 麦克纳姆轮小车配置参数
 */
typedef struct {
    float wheel_radius;           // 轮子半径 (cm)
    float wheel_base_width;       // 轮距宽度 (cm) - 左右轮中心间距
    float wheel_base_length;      // 轮距长度 (cm) - 前后轮中心间距
    float encoder_resolution;     // 编码器分辨率 (脉冲/转)
    float max_wheel_speed;        // 轮子最大速度 (rad/s)
    float max_body_speed;         // 车体最大速度 (cm/s)
    float max_angular_speed;      // 最大角速度 (rad/s)
} MecanumConfig_t;

/**
 * @brief 初始化麦克纳姆轮运动学
 * @param config 配置参数指针
 * @return 是否成功
 */
bool Mecanum_Kinematics_Init(MecanumConfig_t *config);

/**
 * @brief 设置配置参数
 * @param config 配置参数指针
 */
void Mecanum_Kinematics_SetConfig(MecanumConfig_t *config);

/**
 * @brief 获取当前配置
 * @return 当前配置指针
 */
MecanumConfig_t *Mecanum_Kinematics_GetConfig(void);

/**
 * @brief 正运动学解算
 * @details 从四个轮子的角度增量计算车体位姿变化
 * @param encoder_delta 编码器增量数据
 * @param pose_delta 输出的位姿增量
 * @return 是否成功
 */
bool Mecanum_ForwardKinematics(WheelEncoderData_t *encoder_delta, PoseDelta_t *pose_delta);

/**
 * @brief 逆运动学解算
 * @details 从车体期望速度计算四个轮子的目标速度
 * @param motion 车体期望运动状态
 * @param wheel_speed 输出的轮子速度 (rad/s)
 * @return 是否成功
 */
bool Mecanum_InverseKinematics(MotionState_t *motion, WheelData_t *wheel_speed);

/**
 * @brief 轮子速度限制
 * @param wheel_speed 轮子速度数据
 */
void Mecanum_LimitWheelSpeed(WheelData_t *wheel_speed);

/**
 * @brief 车体速度限制
 * @param motion 车体运动状态
 */
void Mecanum_LimitMotionSpeed(MotionState_t *motion);

/**
 * @brief 计算轮子到车体中心的距离
 * @return 距离 (cm)
 */
float Mecanum_GetWheelDistance(void);

#endif /* __MECANUM_KINEMATICS_H__ */
