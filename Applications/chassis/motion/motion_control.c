/**
 * @file motion_control.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 运动控制模块源文件
 */

#include "motion_control.h"
#include "chassis_config.h"
#include "step_port.h"
#include "step_ttl.h"
#include "nav_math.h"
#include "kinematics.h"
#include "cmsis_os2.h"
#include "log.h"
#include <math.h>
#include <string.h>


/**
 * @brief 运动控制结构体
 */
typedef struct {
    uint16_t linear_speed;   // 线速度（厘米/秒）
    uint16_t yaw_speed;      // 偏摆速度（度/秒）
    uint16_t car_acc;        // 加速度（厘米/秒²）
    uint16_t car_dec;        // 减速度（厘米/秒²）
} MotionControl_t;

static MotionControl_t *g_motion = NULL;
static WheelEncoderData_t *g_last_enc = NULL;
static Pose_t *g_enc_pose = NULL;
static bool is_init = false;

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

/**
 * @brief 初始化运动控制模块
 * @return 初始化结果
 */
bool MotionControl_Init(void) {
    g_motion = pvPortMalloc(sizeof(MotionControl_t));
    if (g_motion == NULL) {
        return false;
    }

    g_enc_pose = pvPortMalloc(sizeof(Pose_t));
    if (g_enc_pose == NULL) {
        vPortFree(g_motion);
        return false;
    }

    g_last_enc = pvPortMalloc(sizeof(WheelEncoderData_t));
    if (g_last_enc == NULL) {
        vPortFree(g_motion);
        vPortFree(g_enc_pose);
        return false;
    }

    memset(g_enc_pose, 0, sizeof(Pose_t));
    memset(g_last_enc, 0, sizeof(WheelEncoderData_t));

    // 初始化编码器初始值
    extern MotorStatusShared_t *g_motor_status;
    MotorStatus_t *motor;

    for (int i = 0; i < 4; i++) {
        motor = &g_motor_status->motors[i];
		switch(motor->motor_id) {
			case MOTOR_FRONT_LEFT: g_last_enc->front_left = motor->encoder_linear; break;
			case MOTOR_FRONT_RIGHT: g_last_enc->front_right = motor->encoder_linear; break;
			case MOTOR_BACK_LEFT: g_last_enc->rear_left = motor->encoder_linear; break;
			case MOTOR_BACK_RIGHT: g_last_enc->rear_right = motor->encoder_linear; break;
		}
    }
    g_last_enc->timestamp = osKernelGetTickCount();

    g_motion->linear_speed = 100;
    g_motion->yaw_speed = 90;
    g_motion->car_acc = acc_car_to_motor(100.0f);
    g_motion->car_dec = acc_car_to_motor(100.0f);

    is_init = true;
    return true;
}

/**
 * @brief 里程计更新
 * @param pose 输出参数，用于存储更新后的定位数据
 * @return 更新结果
 */
bool MotionControl_OdomUpdate(Pose_t *pose) {
    extern MotorStatusShared_t *g_motor_status;
	extern osMutexId_t Pose_MutexHandle;
    MotorStatus_t *motor;
    WheelEncoderData_t enc;

	if (!is_init) return false;
	if (osMutexAcquire(Pose_MutexHandle, osWaitForever) == osOK) {
		for (int i = 0; i < 4; i++) {
			motor = &g_motor_status->motors[i];
			switch(motor->motor_id) {
				case MOTOR_FRONT_LEFT: enc.front_left = motor->encoder_linear; break;
				case MOTOR_FRONT_RIGHT: enc.front_right = motor->encoder_linear; break;
				case MOTOR_BACK_LEFT: enc.rear_left = motor->encoder_linear; break;
				case MOTOR_BACK_RIGHT: enc.rear_right = motor->encoder_linear; break;
			}
		}
		enc.timestamp = osKernelGetTickCount();
		osMutexRelease(Pose_MutexHandle);
	}

    // 计算编码器脉冲增量
    WheelEncoderData_t enc_delta = {
        .front_left  = (int32_t)(enc.front_left - g_last_enc->front_left),
        .front_right = (int32_t)(enc.front_right - g_last_enc->front_right),
        .rear_left   = (int32_t)(enc.rear_left - g_last_enc->rear_left),
        .rear_right  = (int32_t)(enc.rear_right - g_last_enc->rear_right),
        .timestamp   = enc.timestamp
    };

    // 正运动学解算 - 得到车体坐标系下的位姿增量
    PoseDelta_t delta;
    Kinematics_Forward(&enc_delta, &delta);

    // 将车体坐标系增量旋转到世界坐标系并累加
    float cos_yaw = cosf(g_enc_pose->yaw);
    float sin_yaw = sinf(g_enc_pose->yaw);
    g_enc_pose->x += delta.dx * cos_yaw - delta.dy * sin_yaw;
    g_enc_pose->y += delta.dx * sin_yaw + delta.dy * cos_yaw;
    g_enc_pose->yaw = normalize_angle(g_enc_pose->yaw + delta.dyaw);
    g_enc_pose->timestamp = enc.timestamp;

    // 更新上一次的编码器值
    memcpy(g_last_enc, &enc, sizeof(WheelEncoderData_t));

    // 更新导航定位
    *pose = *g_enc_pose;
    return true;
}

#if MOTOR_CMD_VELOCITY
/**
 * @brief 发送轮子速度命令
 * @param wheels 四个轮子的速度 (rad/s)
 */
static void send_wheel_velocity_commands(Wheel_t *wheels) {
	MotorCmd_t cmd;
	cmd.op_type = OP_CONTROL;
	cmd.type.ctrl.type = CMD_VELOCITY;
	cmd.type.ctrl.p.vel.acc = g_motion->car_acc;
	cmd.type.ctrl.p.vel.sync = true;

	uint16_t rpm = (uint16_t)(fabs(wheels->fl) * 60.0f / (2.0f * M_PI));
	cmd.motor_id = MOTOR_FRONT_LEFT;
	cmd.type.ctrl.p.vel.dir = (wheels->fl >= 0) ? 0 : 1;
	cmd.type.ctrl.p.vel.vel = rpm;
	Motor_Send_Cmd(&cmd);

	rpm = (uint16_t)(fabs(wheels->fr) * 60.0f / (2.0f * M_PI));
	cmd.motor_id = MOTOR_FRONT_RIGHT;
	cmd.type.ctrl.p.vel.dir = (wheels->fr >= 0) ? 1 : 0;
	cmd.type.ctrl.p.vel.vel = rpm;
	Motor_Send_Cmd(&cmd);

	rpm = (uint16_t)(fabs(wheels->rl) * 60.0f / (2.0f * M_PI));
	cmd.motor_id = MOTOR_BACK_LEFT;
	cmd.type.ctrl.p.vel.dir = (wheels->rl >= 0) ? 0 : 1;
	cmd.type.ctrl.p.vel.vel = rpm;
	Motor_Send_Cmd(&cmd);

	rpm = (uint16_t)(fabs(wheels->rr) * 60.0f / (2.0f * M_PI));
	cmd.motor_id = MOTOR_BACK_RIGHT;
	cmd.type.ctrl.p.vel.dir = (wheels->rr >= 0) ? 1 : 0;
	cmd.type.ctrl.p.vel.vel = rpm;
	Motor_Send_Cmd(&cmd);

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

	float vx = x_ratio * (float)g_motion->linear_speed;
	float vy = y_ratio * (float)g_motion->linear_speed;
	float w = yaw_ratio * (float)g_motion->yaw_speed;

    Wheel_t wheels;
    Kinematics_Inverse(vx, vy, w, &wheels);
    send_wheel_velocity_commands(&wheels);
}
#endif /* MOTOR_CMD_VELOCITY */

#if MOTOR_CMD_POSITION
/**
 * @brief 发送轮子位置命令
 * @param wheels 四个轮子的角位移 (rad)
 */
static void send_wheel_position_commands(Wheel_t *wheels) {
	float wheel_rpm = g_motion->linear_speed * 60.0f / (2.0f * M_PI * WHEEL_RADIUS);

	MotorCmd_t cmd;
	cmd.op_type = OP_CONTROL;
	cmd.type.ctrl.type = CMD_POSITION;
	cmd.type.ctrl.p.pos.acc = g_motion->car_acc;
	cmd.type.ctrl.p.pos.mode = 0;
#if CURRENT_FIRMWARE == FIRMWARE_X
	cmd.type.ctrl.p.pos.dec = g_motion->car_dec;
#endif
	cmd.type.ctrl.p.pos.sync = true;

	int32_t pulses = (int32_t)(wheels->fl * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
	uint32_t clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
	cmd.motor_id = MOTOR_FRONT_LEFT;
	cmd.type.ctrl.p.pos.dir = (pulses >= 0) ? 0 : 1;
	cmd.type.ctrl.p.pos.vel = (uint16_t)wheel_rpm;
	cmd.type.ctrl.p.pos.target = (int32_t)clk;
	Motor_Send_Cmd(&cmd);

	pulses = (int32_t)(wheels->fr * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
	clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
	cmd.motor_id = MOTOR_FRONT_RIGHT;
	cmd.type.ctrl.p.pos.dir = (pulses >= 0) ? 1 : 0;
	cmd.type.ctrl.p.pos.vel = (uint16_t)wheel_rpm;
	cmd.type.ctrl.p.pos.target = (int32_t)clk;
	Motor_Send_Cmd(&cmd);

	pulses = (int32_t)(wheels->rl * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
	clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
	cmd.motor_id = MOTOR_BACK_LEFT;
	cmd.type.ctrl.p.pos.dir = (pulses >= 0) ? 0 : 1;
	cmd.type.ctrl.p.pos.vel = (uint16_t)wheel_rpm;
	cmd.type.ctrl.p.pos.target = (int32_t)clk;
	Motor_Send_Cmd(&cmd);

	pulses = (int32_t)(wheels->rr * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
	clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
	cmd.motor_id = MOTOR_BACK_RIGHT;
	cmd.type.ctrl.p.pos.dir = (pulses >= 0) ? 1 : 0;
	cmd.type.ctrl.p.pos.vel = (uint16_t)wheel_rpm;
	cmd.type.ctrl.p.pos.target = (int32_t)clk;
	Motor_Send_Cmd(&cmd);

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
    Wheel_t wheels;
    Kinematics_Inverse((float)x_offset, (float)y_offset, (float)yaw_offset, &wheels);
    send_wheel_position_commands(&wheels);
}
#endif /* MOTOR_CMD_POSITION */

/**
 * @brief 设置运动参数
 * @param linear_speed 最大线速度（厘米/秒）
 * @param yaw_speed 最大偏摆速度（度/秒）
 * @param acc 加速度（厘米/秒²）
 * @param dec 减速度（厘米/秒²）
 */
void MotionControl_SetMotionParams(uint16_t linear_speed, uint16_t yaw_speed, uint16_t acc, uint16_t dec) {
    g_motion->linear_speed = linear_speed;
    g_motion->yaw_speed = yaw_speed;
    g_motion->car_acc = acc_car_to_motor((float)acc);
    g_motion->car_dec = acc_car_to_motor((float)dec);
}

/**
 * @brief 获取运动参数
 * @param linear_speed 最大线速度（厘米/秒）
 * @param yaw_speed 最大偏摆速度（度/秒）
 * @param acc 加速度（厘米/秒²）
 * @param dec 减速度（厘米/秒²）
 */
void MotionControl_GetMotionParams(uint16_t *linear_speed, uint16_t *yaw_speed, uint16_t *acc, uint16_t *dec) {
    *linear_speed = g_motion->linear_speed;
    *yaw_speed = g_motion->yaw_speed;
    *acc = acc_motor_to_car((float)g_motion->car_acc);
    *dec = acc_motor_to_car((float)g_motion->car_dec);
}

/**
 * @brief 设置当前位姿
 * @param pose 位姿指针
 */
void MotionControl_SetPose(Pose_t *pose) {
	*g_enc_pose = *pose;
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
