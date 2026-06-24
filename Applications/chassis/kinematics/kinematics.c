/**
 * @file kinematics.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 麦克纳姆轮运动学解算模块源文件
 */

#include "kinematics.h"
#include "chassis_config.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/**
 * @brief 正运动学解算
 * @param encoder_delta 编码器增量数据
 * @param pose_delta 位姿增量输出（dyaw单位为度）
 */
void Kinematics_Forward(WheelEncoderData_t *encoder_delta, PoseDelta_t *pose_delta) {
    float L = sqrtf(WHEEL_BASE_WIDTH * WHEEL_BASE_WIDTH + WHEEL_BASE_LENGTH * WHEEL_BASE_LENGTH) / 2.0f;
    float pulse_to_deg = 360.0f / MOTOR_PULSES_PER_REV;

    float delta_theta[4] = {
        (float)encoder_delta->front_left * pulse_to_deg,
        (float)encoder_delta->front_right * pulse_to_deg,
        (float)encoder_delta->rear_left * pulse_to_deg,
        (float)encoder_delta->rear_right * pulse_to_deg
    };

    float delta_s[4];
    for (int i = 0; i < 4; i++) {
        delta_s[i] = delta_theta[i] * WHEEL_RADIUS * M_PI / 180.0f;
    }

    pose_delta->dx = (delta_s[0] + delta_s[1] + delta_s[2] + delta_s[3]) / 4.0f;
    pose_delta->dy = (-delta_s[0] + delta_s[1] + delta_s[2] - delta_s[3]) / 4.0f;
    pose_delta->dyaw = (-delta_theta[0] + delta_theta[1] - delta_theta[2] + delta_theta[3]) / 4.0f;
}

/**
 * @brief 逆运动学解算
 * @param vx X方向速度 (cm/s)
 * @param vy Y方向速度 (cm/s)
 * @param w 角速度 (deg/s)
 * @param wheels 四个轮子的输出角速度 (rad/s)
 */
void Kinematics_Inverse(float vx, float vy, float w, Wheel_t *wheels) {
    float omega = w * M_PI / 180.0f;

    float L = WHEEL_BASE_LENGTH / 2.0f;
    float W = WHEEL_BASE_WIDTH / 2.0f;
    float R = WHEEL_RADIUS;

    wheels->fl = (vx - vy - (L + W) * omega) / R;  // 前左轮
	wheels->fr = (vx + vy + (L + W) * omega) / R;  // 前右轮
	wheels->rl = (vx + vy - (L + W) * omega) / R;  // 后左轮
	wheels->rr = (vx - vy + (L + W) * omega) / R;  // 后右轮
}
