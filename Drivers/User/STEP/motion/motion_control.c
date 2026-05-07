/**
 * @file motion_control.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 运动控制模块源文件
 */

#include "motion_control.h"
#include "motion_config.h"
#include "step_port.h"
#include "log.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/**
 * @brief 运动控制状态结构体
 */
typedef struct {
	uint16_t linear_speed;	// 线速度（厘米/秒）
	uint16_t yaw_speed;		// 偏摆速度（度/秒）
	uint16_t car_acc;		// 加速度（厘米/秒²）
	uint16_t car_dec;		// 减速度（厘米/秒²）
} MotionControlState_t;

MotionControlState_t g_motion_control;

#if MOTOR_CMD_VELOCITY || MOTOR_CMD_POSITION
/**
 * @brief 运动学逆解
 * @param vx_cm X方向值 (cm/s 或 cm)
 * @param vy_cm Y方向值 (cm/s 或 cm)
 * @param w_deg 角速度/角度 (deg/s 或 deg)
 * @param wheel_outputs 四个轮子的输出值 (rad/s 或 rad)
 */
static void kinematics_inverse(float vx_cm, float vy_cm, float w_deg, float wheel_outputs[4]) {
	float vx = vx_cm;
	float vy = vy_cm;
	float w = w_deg * M_PI / 180.0f;

	float L = WHEEL_BASE_LENGTH / 2.0f;
	float W = WHEEL_BASE_WIDTH / 2.0f;
	float R = WHEEL_RADIUS;

	wheel_outputs[0] = (vx - vy - (L + W) * w) / R;	// 前左轮
	wheel_outputs[1] = (vx + vy + (L + W) * w) / R;	// 前右轮
	wheel_outputs[2] = (vx + vy - (L + W) * w) / R;	// 后左轮
	wheel_outputs[3] = (vx - vy + (L + W) * w) / R;	// 后右轮
}
#endif /* MOTOR_CMD_VELOCITY || MOTOR_CMD_POSITION */

/**
 * @brief 小车线加速度(cm/s²) 转 电机加速度(RPM/S)
 * @param car_acc 小车线加速度（厘米/秒²）
 * @return 电机加速度（RPM/S）
 */
static uint16_t acc_car_to_motor(float car_acc) {
	float rpm_s = car_acc * 60.0f / (2.0f * M_PI * WHEEL_RADIUS);
	if (rpm_s > 65535.0f) return 65535;
	return (uint16_t)(rpm_s + 0.5f);
}

/**
 * @brief 电机加速度(RPM/S) 转 小车线加速度(cm/s²)
 * @param rpm_s 电机加速度（RPM/S）
 * @return 小车线加速度（厘米/秒²）
 */
static uint16_t acc_motor_to_car(float rpm_s) {
	float car_acc = rpm_s * (2.0f * M_PI * WHEEL_RADIUS) / 60.0f;
	if (car_acc > 65535.0f) return 65535;
	return (uint16_t)(car_acc + 0.5f);
}

#if MOTOR_CMD_VELOCITY
/**
 * @brief 发送轮子速度命令
 * @param wheel_speeds 四个轮子的速度 (rad/s)
 */
static void send_wheel_velocity_commands(float wheel_speeds[4]) {
	uint8_t motor_ids[] = {MOTOR_FRONT_LEFT, MOTOR_FRONT_RIGHT, MOTOR_BACK_LEFT, MOTOR_BACK_RIGHT};
	MotorCmd_t cmd;
	cmd.op_type = OP_CONTROL;
	cmd.type.ctrl.type = CMD_VELOCITY;
	cmd.type.ctrl.p.vel.acc = g_motion_control.car_acc;
	cmd.type.ctrl.p.vel.sync = true;

	for (uint8_t i = 0; i < 4; i++) {
		float speed = wheel_speeds[i];
		uint8_t dir;
		if (motor_ids[i] == MOTOR_FRONT_LEFT || motor_ids[i] == MOTOR_BACK_LEFT) {
			dir = (speed >= 0) ? 0 : 1;
		} else {
			dir = (speed >= 0) ? 1 : 0;
		}
		uint16_t rpm = (uint16_t)(fabs(speed) * 60.0f / (2.0f * M_PI));

		cmd.motor_id = motor_ids[i];
		cmd.type.ctrl.p.vel.dir = dir;
		cmd.type.ctrl.p.vel.vel = rpm;
		Motor_Send_Cmd(&cmd);
	}

	cmd.motor_id = 0;
	cmd.type.ctrl.type = CMD_SYNC;
	Motor_Send_Cmd(&cmd);
}

/**
 * @brief 速度控制
 * @param x_component X分量，前正后负
 * @param y_component Y分量，左正右负
 * @param yaw_component Yaw分量，顺正逆负
 */
void MotionControl_SetVelocity(int8_t x_component, int8_t y_component, int8_t yaw_component) {
	float x_ratio = (float)x_component / 127.0f;
	float y_ratio = (float)y_component / 127.0f;
	float yaw_ratio = (float)yaw_component / 127.0f;

	float vx = x_ratio * (float)g_motion_control.linear_speed;
	float vy = y_ratio * (float)g_motion_control.linear_speed;
	float w = yaw_ratio * (float)g_motion_control.yaw_speed;

	float wheel_speeds[4];
	kinematics_inverse(vx, vy, w, wheel_speeds);
	send_wheel_velocity_commands(wheel_speeds);
}
#endif /* MOTOR_CMD_VELOCITY */

#if MOTOR_CMD_POSITION
/**
 * @brief 发送轮子位置命令
 * @param wheel_rad 四个轮子的角位移 (rad)
 */
static void send_wheel_position_commands(float wheel_rad[4]) {
	uint8_t motor_ids[] = {MOTOR_FRONT_LEFT, MOTOR_FRONT_RIGHT, MOTOR_BACK_LEFT, MOTOR_BACK_RIGHT};
	float wheel_rpm = g_motion_control.linear_speed * 60.0f / (2.0f * M_PI * WHEEL_RADIUS);

	MotorCmd_t cmd;
	cmd.op_type = OP_CONTROL;
	cmd.type.ctrl.type = CMD_POSITION;
	cmd.type.ctrl.p.pos.acc = g_motion_control.car_acc;
	cmd.type.ctrl.p.pos.mode = 0;
#if CURRENT_FIRMWARE == FIRMWARE_X
	cmd.type.ctrl.p.pos.dec = g_motion_control.car_dec;
#endif
	cmd.type.ctrl.p.pos.sync = true;

	for (uint8_t i = 0; i < 4; i++) {
		int32_t pulses = (int32_t)(wheel_rad[i] * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
		uint8_t dir;
		if (motor_ids[i] == MOTOR_FRONT_LEFT || motor_ids[i] == MOTOR_BACK_LEFT) {
			dir = (pulses >= 0) ? 0 : 1;
		} else {
			dir = (pulses >= 0) ? 1 : 0;
		}
		uint32_t clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);

		cmd.motor_id = motor_ids[i];
		cmd.type.ctrl.p.pos.dir = dir;
		cmd.type.ctrl.p.pos.vel = (uint16_t)wheel_rpm;
		cmd.type.ctrl.p.pos.target = (int32_t)clk;
		Motor_Send_Cmd(&cmd);
	}

	cmd.motor_id = 0;
	cmd.type.ctrl.type = CMD_SYNC;
	Motor_Send_Cmd(&cmd);
}

/**
 * @brief 位置控制
 * @param x_offset X偏移（厘米），前正后负
 * @param y_offset Y偏移（厘米），左正右负
 * @param yaw_offset Yaw偏移（度），顺正逆负
 */
void MotionControl_SetPosition(int32_t x_offset, int32_t y_offset, int32_t yaw_offset) {
	float wheel_rad[4];
	kinematics_inverse((float)x_offset, (float)y_offset, (float)yaw_offset, wheel_rad);
	send_wheel_position_commands(wheel_rad);
}
#endif /* MOTOR_CMD_POSITION */

/**
 * @brief 初始化运动控制模块
 */
void MotionControl_Init(void) {
	g_motion_control.linear_speed = 100;
	g_motion_control.yaw_speed = 90;
	g_motion_control.car_acc = acc_car_to_motor(100.0f);
	g_motion_control.car_dec = acc_car_to_motor(100.0f);
}

/**
 * @brief 设置运动参数
 * @param linear_speed 最大线速度（厘米/秒）
 * @param yaw_speed 最大偏摆速度（度/秒）
 * @param acc 加速度（厘米/秒²）
 * @param dec 减速度（厘米/秒²）
 */
void MotionControl_SetMotionParams(uint16_t linear_speed, uint16_t yaw_speed, uint16_t acc, uint16_t dec) {
	g_motion_control.linear_speed = linear_speed;
	g_motion_control.yaw_speed = yaw_speed;
	g_motion_control.car_acc = acc_car_to_motor((float)acc);
	g_motion_control.car_dec = acc_car_to_motor((float)dec);
}

/**
 * @brief 获取运动参数
 * @param linear_speed 最大线速度（厘米/秒）
 * @param yaw_speed 最大偏摆速度（度/秒）
 * @param acc 加速度（厘米/秒²）
 * @param dec 减速度（厘米/秒²）
 */
void MotionControl_GetMotionParams(uint16_t *linear_speed, uint16_t *yaw_speed, uint16_t *acc, uint16_t *dec) {
	*linear_speed = g_motion_control.linear_speed;
	*yaw_speed = g_motion_control.yaw_speed;
	*acc = acc_motor_to_car((float)g_motion_control.car_acc);
	*dec = acc_motor_to_car((float)g_motion_control.car_dec);
}

#if MOTOR_CMD_STOP
/**
 * @brief 停止运动
 */
void MotionControl_Stop(void) {
	uint8_t motor_ids[] = {MOTOR_FRONT_LEFT, MOTOR_FRONT_RIGHT, MOTOR_BACK_LEFT, MOTOR_BACK_RIGHT};
	MotorCmd_t cmd;
	cmd.op_type = OP_CONTROL;
	cmd.type.ctrl.type = CMD_STOP;
	cmd.type.ctrl.p.stop.sync = true;

	for (uint8_t i = 0; i < 4; i++) {
		cmd.motor_id = motor_ids[i];
		Motor_Send_Cmd(&cmd);
	}

	cmd.motor_id = 0;
	cmd.type.ctrl.type = CMD_SYNC;
	Motor_Send_Cmd(&cmd);
}
#endif /* MOTOR_CMD_STOP */
