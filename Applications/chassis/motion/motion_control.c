/**
 * @file motion_control.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 运动控制模块源文件
 */

#include "motion_control.h"
#include "chassis_config.h"
#include "zdt_v5_cfg.h"
#include "zdt_v5_cmd.h"
#include "zdt_v5_port.h"
#include "zdt_v5_engine.h"
#include "nav_math.h"
#include "kinematics.h"
#include "cmsis_os2.h"
#include "log.h"
#include <string.h>


/**
 * @brief 运动控制结构体
 */
typedef struct {
    float linear_speed;   // 线速度 (cm/s)
    float yaw_speed;      // 偏摆速度 (deg/s)
    float car_acc;        // 加速度 (RPM/s)
    float car_dec;        // 减速度 (RPM/s)
} MotionControl_t;

static MotionControl_t *g_motion = NULL;
static bool is_init = false;
extern osMutexId_t Motor_MutexHandle;

/**
 * @brief 小车线加速度 (cm/s²) 转 电机加速度 (RPM/s)
 * @param car_acc 小车线加速度 (cm/s²)
 * @return 电机加速度 (RPM/s)
 */
static float acc_car_to_motor(float car_acc) {
    return car_acc * 60.0f / (2.0f * M_PI * WHEEL_RADIUS);
}

/**
 * @brief 电机加速度 (RPM/s) 转 小车线加速度 (cm/s²)
 * @param rpm_s 电机加速度 (RPM/s)
 * @return 小车线加速度 (cm/s²)
 */
static float acc_motor_to_car(float rpm_s) {
    return rpm_s * (2.0f * M_PI * WHEEL_RADIUS) / 60.0f;
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
    memset(g_motion, 0, sizeof(MotionControl_t));

    g_motion->linear_speed = 80.0f;
    g_motion->yaw_speed = 140.0f;
    g_motion->car_acc = acc_car_to_motor(70.0f);
    g_motion->car_dec = acc_car_to_motor(100.0f);

    is_init = true;
    return true;
}

#if MOTOR_VELOCITY_MODE
/**
 * @brief 发送轮子速度命令
 * @param wheels 四个轮子的角位移 (rad)
 */
static void send_wheel_velocity_commands(Wheel_t *wheels) {
    if (!is_init) return;
    MotorCmd_t cmd;

#if MOTOR_MULTI_CMD
    ZDT_V5_Multi_Cmd_t multi_cmd = {
        .buf_size = MOTOR_MULTI_BUF_SIZE
    };
    MotorMulti_t multi;

    ZDT_V5_Multi_Reset(&multi_cmd);

    multi.type = MULTI_VEL;
    multi.p.vel.acc = (uint16_t)g_motion->car_acc;
    multi.p.vel.snF = false;

    uint16_t wheel_rpm = (uint16_t)(fabsf(wheels->fl) * 60.0f / (2.0f * M_PI));
    multi.p.vel.dir = (wheels->fl >= 0) ? 0 : 1;
    multi.p.vel.vel = wheel_rpm;
    ZDT_V5_Process_Multi_Cmd(MOTOR_FRONT_LEFT, &multi, &multi_cmd);

    wheel_rpm = (uint16_t)(fabsf(wheels->fr) * 60.0f / (2.0f * M_PI));
    multi.p.vel.dir = (wheels->fr >= 0) ? 1 : 0;
    multi.p.vel.vel = wheel_rpm;
    ZDT_V5_Process_Multi_Cmd(MOTOR_FRONT_RIGHT, &multi, &multi_cmd);

    wheel_rpm = (uint16_t)(fabsf(wheels->rl) * 60.0f / (2.0f * M_PI));
    multi.p.vel.dir = (wheels->rl >= 0) ? 0 : 1;
    multi.p.vel.vel = wheel_rpm;
    ZDT_V5_Process_Multi_Cmd(MOTOR_BACK_LEFT, &multi, &multi_cmd);

    wheel_rpm = (uint16_t)(fabsf(wheels->rr) * 60.0f / (2.0f * M_PI));
    multi.p.vel.dir = (wheels->rr >= 0) ? 1 : 0;
    multi.p.vel.vel = wheel_rpm;
    ZDT_V5_Process_Multi_Cmd(MOTOR_BACK_RIGHT, &multi, &multi_cmd);

    cmd.motor_id = 0;
    cmd.op_type = OP_CONTROL;
    cmd.type.ctrl.type = CTRL_MULTI;
    cmd.type.ctrl.p.multi.cmd = multi_cmd;
    Motor_Send_Cmd(&cmd);
#else
    MotorCtrl_t ctrl;
    cmd.op_type = OP_CONTROL;
    ctrl.type = CTRL_VEL;
    ctrl.p.vel.acc = (uint16_t)g_motion->car_acc;
    ctrl.p.vel.sync = true;

    uint16_t wheel_rpm = (uint16_t)(fabsf(wheels->fl) * 60.0f / (2.0f * M_PI));
    cmd.motor_id = MOTOR_FRONT_LEFT;
    ctrl.p.vel.dir = (wheels->fl >= 0) ? 0 : 1;
    ctrl.p.vel.vel = wheel_rpm;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    wheel_rpm = (uint16_t)(fabsf(wheels->fr) * 60.0f / (2.0f * M_PI));
    cmd.motor_id = MOTOR_FRONT_RIGHT;
    ctrl.p.vel.dir = (wheels->fr >= 0) ? 1 : 0;
    ctrl.p.vel.vel = wheel_rpm;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    wheel_rpm = (uint16_t)(fabsf(wheels->rl) * 60.0f / (2.0f * M_PI));
    cmd.motor_id = MOTOR_BACK_LEFT;
    ctrl.p.vel.dir = (wheels->rl >= 0) ? 0 : 1;
    ctrl.p.vel.vel = wheel_rpm;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    wheel_rpm = (uint16_t)(fabsf(wheels->rr) * 60.0f / (2.0f * M_PI));
    cmd.motor_id = MOTOR_BACK_RIGHT;
    ctrl.p.vel.dir = (wheels->rr >= 0) ? 1 : 0;
    ctrl.p.vel.vel = wheel_rpm;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    cmd.motor_id = 0;
    ctrl.type = CTRL_SYNC;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);
#endif
}

/**
 * @brief 速度控制
 * @param x X分量
 * @param y Y分量
 * @param yaw Yaw分量
 */
void MotionControl_SetVelocity(float x, float y, float yaw) {
    Wheel_t wheels;
    Kinematics_Inverse(x, y, yaw, &wheels);
    send_wheel_velocity_commands(&wheels);
}
#endif /* MOTOR_VELOCITY_MODE */

#if MOTOR_POS_MODE
/**
 * @brief 发送轮子位置命令
 * @param wheels 四个轮子的角位移 (rad)
 */
static void send_wheel_position_commands(Wheel_t *wheels) {
    if (!is_init) return;
    float wheel_rpm = g_motion->linear_speed * 60.0f / (2.0f * M_PI * WHEEL_RADIUS);
    MotorCmd_t cmd;

#if MOTOR_MULTI_CMD
    ZDT_V5_Multi_Cmd_t multi_cmd = {
        .buf_size = MOTOR_MULTI_BUF_SIZE
    };
    MotorMulti_t multi;

    ZDT_V5_Multi_Reset(&multi_cmd);

    multi.type = MULTI_POS;
    multi.p.pos.acc = (uint16_t)g_motion->car_acc;
    multi.p.pos.mode = 0;
    multi.p.pos.rsp = false;
#if CURRENT_FIRMWARE == FIRMWARE_X
    multi.p.pos.dec = (uint16_t)g_motion->car_dec;
#endif

    int32_t pulses = (int32_t)(wheels->fl * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    uint32_t clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    multi.p.pos.dir = (pulses >= 0) ? 0 : 1;
    multi.p.pos.vel = (uint16_t)wheel_rpm;
    multi.p.pos.target = (int32_t)clk;
    ZDT_V5_Process_Multi_Cmd(MOTOR_FRONT_LEFT, &multi, &multi_cmd);

    pulses = (int32_t)(wheels->fr * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    multi.p.pos.dir = (pulses >= 0) ? 1 : 0;
    multi.p.pos.target = (int32_t)clk;
    ZDT_V5_Process_Multi_Cmd(MOTOR_FRONT_RIGHT, &multi, &multi_cmd);

    pulses = (int32_t)(wheels->rl * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    multi.p.pos.dir = (pulses >= 0) ? 0 : 1;
    multi.p.pos.target = (int32_t)clk;
    ZDT_V5_Process_Multi_Cmd(MOTOR_BACK_LEFT, &multi, &multi_cmd);

    pulses = (int32_t)(wheels->rr * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    multi.p.pos.dir = (pulses >= 0) ? 1 : 0;
    multi.p.pos.target = (int32_t)clk;
    ZDT_V5_Process_Multi_Cmd(MOTOR_BACK_RIGHT, &multi, &multi_cmd);

    cmd.motor_id = 0;
    cmd.op_type = OP_CONTROL;
    cmd.type.ctrl.type = CTRL_MULTI;
    cmd.type.ctrl.p.multi.cmd = multi_cmd;
    Motor_Send_Cmd(&cmd);
#else
    MotorCtrl_t ctrl;
    cmd.op_type = OP_CONTROL;
    ctrl.type = CTRL_POS;
    ctrl.p.pos.acc = (uint16_t)g_motion->car_acc;
    ctrl.p.pos.mode = 0;
    ctrl.p.pos.sync = true;
#if CURRENT_FIRMWARE == FIRMWARE_X
    ctrl.p.pos.dec = (uint16_t)g_motion->car_dec;
#endif

    int32_t pulses = (int32_t)(wheels->fl * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    uint32_t clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    cmd.motor_id = MOTOR_FRONT_LEFT;
    ctrl.p.pos.dir = (pulses >= 0) ? 0 : 1;
    ctrl.p.pos.vel = (uint16_t)wheel_rpm;
    ctrl.p.pos.target = (int32_t)clk;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    pulses = (int32_t)(wheels->fr * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    cmd.motor_id = MOTOR_FRONT_RIGHT;
    ctrl.p.pos.dir = (pulses >= 0) ? 1 : 0;
    ctrl.p.pos.vel = (uint16_t)wheel_rpm;
    ctrl.p.pos.target = (int32_t)clk;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    pulses = (int32_t)(wheels->rl * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    cmd.motor_id = MOTOR_BACK_LEFT;
    ctrl.p.pos.dir = (pulses >= 0) ? 0 : 1;
    ctrl.p.pos.vel = (uint16_t)wheel_rpm;
    ctrl.p.pos.target = (int32_t)clk;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    pulses = (int32_t)(wheels->rr * MOTOR_PULSES_PER_REV / (2.0f * M_PI));
    clk = (uint32_t)(pulses >= 0 ? pulses : -pulses);
    cmd.motor_id = MOTOR_BACK_RIGHT;
    ctrl.p.pos.dir = (pulses >= 0) ? 1 : 0;
    ctrl.p.pos.vel = (uint16_t)wheel_rpm;
    ctrl.p.pos.target = (int32_t)clk;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);

    cmd.motor_id = 0;
    ctrl.type = CTRL_SYNC;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);
#endif
}

/**
 * @brief 位置控制
 * @param x_offset X偏移 (cm)
 * @param y_offset Y偏移 (cm)
 * @param yaw_offset Yaw偏移 (deg)
 */
void MotionControl_SetPosition(float x_offset, float y_offset, float yaw_offset) {
    Wheel_t wheels;
    Kinematics_Inverse(x_offset, y_offset, yaw_offset, &wheels);
    send_wheel_position_commands(&wheels);
}
#endif /* MOTOR_POS_MODE */

/**
 * @brief 设置运动参数
 * @param linear_speed 最大线速度 (cm/s)
 * @param yaw_speed 最大偏摆速度 (deg/s)
 * @param acc 加速度 (cm/s²)
 * @param dec 减速度 (cm/s²)
 */
void MotionControl_SetMotionParams(float linear_speed, float yaw_speed, float acc, float dec) {
    if (!is_init) return;
    g_motion->linear_speed = linear_speed;
    g_motion->yaw_speed = yaw_speed;
    g_motion->car_acc = acc_car_to_motor(acc);
    g_motion->car_dec = acc_car_to_motor(dec);
}

/**
 * @brief 获取运动参数
 * @param linear_speed 最大线速度 (cm/s)
 * @param yaw_speed 最大偏摆速度 (deg/s)
 * @param acc 加速度 (cm/s²)
 * @param dec 减速度 (cm/s²)
 */
void MotionControl_GetMotionParams(float *linear_speed, float *yaw_speed, float *acc, float *dec) {
    if (!is_init) return;
    *linear_speed = g_motion->linear_speed;
    *yaw_speed = g_motion->yaw_speed;
    *acc = acc_motor_to_car(g_motion->car_acc);
    *dec = acc_motor_to_car(g_motion->car_dec);
}

#if MOTOR_CMD_STOP
/**
 * @brief 停止运动
 */
void MotionControl_Stop(void) {
    MotorCmd_t cmd;
    MotorCtrl_t ctrl;
    cmd.op_type = OP_CONTROL;
    ctrl.type = CTRL_STOP;
    ctrl.p.stop.sync = false;
    cmd.motor_id = 0;
    cmd.type.ctrl = ctrl;
    Motor_Send_Cmd(&cmd);
}
#endif /* MOTOR_CMD_STOP */
