/**
 * @file ZDT_V5_Driver.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机驱动层头文件
 */

#ifndef __ZDT_V5_DRIVER_H__
#define __ZDT_V5_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "step_port_def.h"
#include "step_cfg.h"

#ifndef ZDT_V5_SEND_CMD
#error "ZDT_V5_SEND_CMD not defined"
#endif

#ifndef __IO
#define __IO volatile
#endif


/**
 * @brief 系统状态参数枚举
 */
typedef enum {
	S_NULL = 0, // 无指令
#if MOTOR_STATUS_READ_BATCH
	S_BATCH,	// 批量读取
#endif
#if MOTOR_STATUS_BUS_VOLTAGE
	S_VBUS,    // 读取总线电压
#endif
#if MOTOR_STATUS_PHASE_CURRENT
	S_CPHA,   // 读取相电流
#endif
#if MOTOR_STATUS_ENCODER_VALUE
	S_ENCL,  // 读取经过线性化校准后的编码器值
#endif
#if MOTOR_STATUS_TARGET_POS
	S_TPOS,   // 读取电机目标位置
#endif
#if MOTOR_STATUS_SET_POS
	S_SPOS,   // 读取电机实时设定的目标位置
#endif
#if MOTOR_STATUS_SPEED
	S_VEL,   // 读取电机实时转速
#endif
#if MOTOR_STATUS_REAL_POS
	S_CPOS,   // 读取电机实时位置
#endif
#if MOTOR_STATUS_POS_ERROR
	S_PERR,   // 读取电机位置误差
#endif
#if MOTOR_STATUS_MOTOR_FLAGS
	S_FLAG,   // 读取电机状态标志位
#endif
#if MOTOR_STATUS_HOME_FLAGS
	S_OFLAG,   // 读取回零状态标志位
#endif
#if MOTOR_STATUS_BUS_CURRENT
	S_CBUS,    // 读取总线电流
#endif
#if MOTOR_STATUS_TEMPERATURE
	S_TEMP,   // 读取电机实时温度
#endif
#if MOTOR_STATUS_FLAGS_COMBINED
	S_OAF,   // 读取电机状态标志位 + 回零状态标志位（组合）
#endif
#if MOTOR_STATUS_BATTERY_VOLTAGE
	S_VBAT,   // 读取多圈编码器电池电压
#endif
#if MOTOR_STATUS_INPUT_PULSES
	S_CLKI,   // 读取输入脉冲数
#endif
#if MOTOR_STATUS_PIN_STATUS
	S_PIN,   // 读取引脚IO电平状态
#endif
} SysParams_t;

void ZDT_V5_Read_Sys_Params(uint8_t addr, SysParams_t s);

#if MOTOR_PERIODIC_RETURN
void ZDT_V5_Auto_Return_Sys_Params_Timed(uint8_t addr, SysParams_t s, uint16_t time_ms);
#endif

#if MOTOR_STATUS_READ_BATCH
void ZDT_V5_Read_All_Sys_Params(uint8_t addr);
#endif

/**
 * @brief 驱动配置参数枚举
 */
typedef enum {
	D_NULL = 0, // 无指令
#if MOTOR_DRIVER_CONFIG_READ_BATCH
	D_BATCH,	// 批量读取
#endif
#if MOTOR_DRIVER_POS_WINDOW
	D_POS_WINDOW,     // 位置到达窗口
#endif
} DriverParams_t;

void ZDT_V5_Read_Driver_Params(uint8_t addr, DriverParams_t d);

#if MOTOR_DRIVER_CONFIG_READ_BATCH
void ZDT_V5_Read_Batch_Config(uint8_t addr);
#endif

/**
 * @brief 电机控制参数枚举
 */
typedef enum {
	C_NULL = 0, // 无指令
#if MOTOR_PID_READ
	C_PID,            // 读取PID参数
#endif
#if MOTOR_HOME_READ
	C_HOME,           // 读取回零参数
#endif
#if MOTOR_DRIVER_INTEGRAL_LIMIT
	C_INTEGRAL_LIMIT, // 读取积分限幅/刚性系数
#endif
#if MOTOR_PROTECT_THRESHOLD_READ
	C_PROTECT_THRESHOLD, // 读取过热过流保护阈值
#endif
#if MOTOR_COLLISION_ANGLE_READ
	C_COLLISION_ANGLE, // 读取碰撞回零返回角度
#endif
#if MOTOR_HEARTBEAT_READ
	C_HEARTBEAT,      // 读取心跳保护时间
#endif
} CtrlParams_t;

void ZDT_V5_Read_Ctrl_Params(uint8_t addr, CtrlParams_t c);

#if MOTOR_PID_WRITE
#if CURRENT_FIRMWARE == FIRMWARE_EMM
void ZDT_V5_Modify_PID_Params(uint8_t addr, bool svF, uint32_t kp, uint32_t ki, uint32_t kd);
#elif CURRENT_FIRMWARE == FIRMWARE_X
void ZDT_V5_Modify_PID_Params(uint8_t addr, bool svF, uint32_t trapezoidal_kp, uint32_t direct_kp, uint32_t vel_kp, uint32_t vel_ki);
#endif
#endif

#if MOTOR_INTEGRAL_LIMIT_WRITE
void ZDT_V5_Modify_Integral_Limit(uint8_t addr, bool svF, uint32_t il);
#endif

#if MOTOR_POS_WINDOW_WRITE
void ZDT_V5_Modify_Pos_Window(uint8_t addr, bool svF, uint16_t prw);
#endif

#if MOTOR_PROTECT_THRESHOLD_WRITE
void ZDT_V5_Modify_Otocp(uint8_t addr, bool svF, uint16_t otp, uint16_t ocp, uint16_t time_ms);
#endif

#if MOTOR_HEARTBEAT_WRITE
void ZDT_V5_Modify_Heart_Protect(uint8_t addr, bool svF, uint32_t hp);
#endif

#if MOTOR_COLLISION_ANGLE_WRITE
void ZDT_V5_Modify_Collision_Angle(uint8_t addr, bool svF, uint16_t angle);
#endif

/**
 * @brief 设备信息与特殊功能枚举
 */
typedef enum {
	I_NULL = 0, // 无指令
#if MOTOR_READ_VERSION
	I_VERSION,        // 读取固件版本和硬件版本
#endif
#if MOTOR_READ_PHASE_PARAMS
	I_PHASE_PARAMS,   // 读取相电阻和相电感
#endif
#if MOTOR_READ_OPTION_PARAMS
	I_OPTION,         // 读取选项参数状态
#endif
#if MOTOR_BROADCAST_READ_ID
	I_ID,             // 广播读取ID地址
#endif
#if MOTOR_DRIVER_DMX512
	I_DMX512,         // DMX512协议参数
#endif
} DeviceInfo_t;

void ZDT_V5_Read_Device_Info_Params(uint8_t addr, DeviceInfo_t i);

/******************** 运动控制命令 *********************/

#if MOTOR_CMD_ENABLE
void ZDT_V5_En_Control(uint8_t addr, bool state, bool snF);
#endif

#if MOTOR_VELOCITY_MODE
void ZDT_V5_Vel_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF);
#endif

#if MOTOR_VELOCITY_MODE_LIMIT
void ZDT_V5_Vel_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF, uint16_t max_current);
#endif

#if MOTOR_POS_MODE_DIRECT
void ZDT_V5_Pos_Control_Direct(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF);
#endif

#if MOTOR_POS_MODE_DIRECT_LIMIT
void ZDT_V5_Pos_Control_Direct_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current);
#endif

#if MOTOR_POS_MODE_TRAPEZOIDAL
void ZDT_V5_Pos_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t clk, bool raF, bool snF);
#endif

#if MOTOR_POS_MODE_TRAPEZOIDAL_LIMIT
void ZDT_V5_Pos_Control_Trapezoidal_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current);
#endif

#if MOTOR_TORQUE_MODE
void ZDT_V5_Torque_Control(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, bool snF);
#endif

#if MOTOR_TORQUE_MODE_LIMIT
void ZDT_V5_Torque_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, uint16_t max_vel, bool snF);
#endif

#if MOTOR_CMD_STOP
void ZDT_V5_Stop(uint8_t addr, bool snF);
#endif

#if MOTOR_SYNC_TRIGGER
void ZDT_V5_Synchronous_Motion(uint8_t addr);
#endif

#if MOTOR_MULTI_CMD
void ZDT_V5_Multi_Motor_Cmd(uint8_t addr, uint8_t len, uint8_t *cmd_data);
#endif

#if MOTOR_POS_MODE_FAST
void ZDT_V5_Fast_Set_Param(uint8_t addr, uint16_t vel, uint16_t acc, uint16_t dec, uint16_t max_current, uint8_t mode, uint8_t sync);
void ZDT_V5_Fast_Send_Pos(uint8_t addr, int32_t pos);
#endif

/******************** 触发动作命令 *********************/

#if MOTOR_TRIGGER_ENCODER_CALIB
void ZDT_V5_Trig_Encoder_Cal(uint8_t addr);
#endif

#if MOTOR_TRIGGER_RESTART
void ZDT_V5_Reset_Motor(uint8_t addr);
#endif

#if MOTOR_TRIGGER_RESET_POS
void ZDT_V5_Reset_CurPos_To_Zero(uint8_t addr);
#endif

#if MOTOR_TRIGGER_CLEAR_PROTECT
void ZDT_V5_Reset_Clog_Pro(uint8_t addr);
#endif

#if MOTOR_TRIGGER_FACTORY_RESET
void ZDT_V5_Restore_Motor(uint8_t addr);
#endif

/******************** 原点回零命令 *********************/

#if MOTOR_HOME_SET_ZERO
void ZDT_V5_Origin_Set_Zero(uint8_t addr, bool svF);
#endif

#if MOTOR_HOME_TRIGGER
void ZDT_V5_Origin_Trigger_Return(uint8_t addr, uint8_t o_mode, bool snF);
#endif

#if MOTOR_HOME_INTERRUPT
void ZDT_V5_Origin_Interrupt(uint8_t addr);
#endif

#if MOTOR_HOME_WRITE
void ZDT_V5_Origin_Modify_Params(uint8_t addr, bool svF, uint8_t o_mode, uint8_t o_dir, uint16_t o_vel, uint32_t o_tm, uint16_t sl_vel, uint16_t sl_ma, uint16_t sl_ms, bool potF);
#endif

/******************** 电机参数设置 *********************/

#if MOTOR_SET_MOTOR_ID
void ZDT_V5_Modify_Motor_ID(uint8_t addr, bool svF, uint8_t id);
#endif

#if MOTOR_SET_MICRO_STEP
void ZDT_V5_Modify_MicroStep(uint8_t addr, bool svF, uint8_t mstep);
#endif

#if MOTOR_SET_POWER_FLAG
void ZDT_V5_Modify_PDFlag(uint8_t addr, bool pdf);
#endif

#if MOTOR_SET_MOTOR_TYPE
void ZDT_V5_Modify_Motor_Type(uint8_t addr, bool svF, bool mottype);
#endif

#if MOTOR_SET_FIRMWARE_TYPE
void ZDT_V5_Modify_Firmware_Type(uint8_t addr, bool svF, bool fwtype);
#endif

#if MOTOR_SET_OPENLOOP_CURRENT
void ZDT_V5_Modify_OM_mA(uint8_t addr, bool svF, uint16_t om_ma);
#endif

#if MOTOR_SET_CLOSEDLOOP_CURRENT
void ZDT_V5_Modify_FOC_mA(uint8_t addr, bool svF, uint16_t foc_mA);
#endif

#if MOTOR_SET_CONTROL_MODE
void ZDT_V5_Modify_Ctrl_Mode(uint8_t addr, bool svF, bool ctrl_mode);
#endif

#if MOTOR_SET_DIRECTION
void ZDT_V5_Modify_Motor_Dir(uint8_t addr, bool svF, bool dir);
#endif

#if MOTOR_SET_LOCK_KEY
void ZDT_V5_Modify_Lock_Btn(uint8_t addr, bool svF, bool lockbtn);
#endif

#if MOTOR_SET_SCALE_INPUT
void ZDT_V5_Modify_S_Vel(uint8_t addr, bool svF, bool s_vel);
#endif

#if MOTOR_SET_DRIVER_CONFIG_ALL
void ZDT_V5_Modify_Batch_Config(uint8_t addr, bool svF, uint8_t *params);
#endif

#if MOTOR_SET_LOCK_PARAMS
void ZDT_V5_Modify_Lock_Params(uint8_t addr, bool svF, uint8_t lock_level);
#endif

#if MOTOR_DMX512_WRITE
void ZDT_V5_Modify_DMX512_Params(uint8_t addr, bool svF, uint16_t tch, uint8_t nch, uint8_t mode, uint16_t vel, uint16_t acc, uint16_t vel_step, uint32_t pos_step);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ZDT_V5_DRIVER_H__ */
