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
	float r = L + W;

	wheels->fl = (vx - vy - r * omega) / R;  // 前左轮
	wheels->fr = (vx + vy + r * omega) / R;  // 前右轮
	wheels->rl = (vx + vy - r * omega) / R;  // 后左轮
	wheels->rr = (vx - vy + r * omega) / R;  // 后右轮
}
