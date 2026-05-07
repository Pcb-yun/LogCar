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
#include "step_port.h"

#ifndef ZDT_V5_SEND_CMD
#error "ZDT_V5_SEND_CMD not defined"
#endif

#ifndef __IO
#define __IO volatile
#endif

/**
 * @brief 电机指令枚举
 * @note 后缀有(Y42)的为Y42新增指令，X42不支持
 */
typedef enum {
	S_VBUS  = 5,	// 读取总线电压

#if CURRENT_FIRMWARE == FIRMWARE_X
	S_CBUS  = 6,	// 读取总线电流
#endif

	S_CPHA  = 7,	// 读取相电流
	S_ENCO  = 8,	// 读取编码器原始值
	S_CLKC  = 9,	// 读取实时脉冲数
	S_ENCL  = 10,	// 读取经过线性化校准后的编码器值
	S_CLKI  = 11,	// 读取输入脉冲数
	S_TPOS  = 12,	// 读取电机目标位置
	S_SPOS  = 13,	// 读取电机实时设定的目标位置
	S_VEL   = 14,	// 读取电机实时转速
	S_CPOS  = 15,	// 读取电机实时位置
	S_PERR  = 16,	// 读取电机位置误差

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
	S_VBAT  = 17,	// 读取多圈编码器电池电压（Y42）
#endif

	S_TEMP  = 18,	// 读取电机实时温度（Y42）
	S_FLAG  = 19,	// 读取电机状态标志位
	S_OFLAG = 20,   // 读取回零状态标志位

	S_OAF   = 21,	// 读取电机状态标志位 + 回零状态标志位（Y42）
	S_PIN   = 22,	// 读取引脚状态（Y42）
}SysParams_t;

/******************** 触发动作指令 **********************/

void ZDT_V5_Trig_Encoder_Cal(uint8_t addr);
void ZDT_V5_Reset_Motor(uint8_t addr);
void ZDT_V5_Reset_CurPos_To_Zero(uint8_t addr);
void ZDT_V5_Reset_Clog_Pro(uint8_t addr);
void ZDT_V5_Restore_Motor(uint8_t addr);

/******************** 运动控制指令 **********************/

#if MOTOR_CMD_ENABLE
void ZDT_V5_En_Control(uint8_t addr, bool state, bool snF);
#endif
#if MOTOR_CMD_VELOCITY
void ZDT_V5_Vel_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF);
#endif
#if MOTOR_CMD_POSITION
void ZDT_V5_Pos_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t clk, bool raF, bool snF);
#endif
#if MOTOR_CMD_TORQUE
void ZDT_V5_Torque_Control(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, bool snF);
#endif
#if MOTOR_CMD_TORQUE && CURRENT_FIRMWARE == FIRMWARE_X
void ZDT_V5_Torque_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, uint16_t max_vel, bool snF);
#endif
#if MOTOR_CMD_STOP
void ZDT_V5_Stop_Now(uint8_t addr, bool snF);
#endif

void ZDT_V5_Synchronous_motion(uint8_t addr);

#if MOTOR_CMD_VELOCITY && CURRENT_FIRMWARE == FIRMWARE_X
void ZDT_V5_Vel_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF, uint16_t max_current);
#endif
#if MOTOR_CMD_POSITION && CURRENT_FIRMWARE == FIRMWARE_X
void ZDT_V5_Pos_Control_Direct(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF);
void ZDT_V5_Pos_Control_Direct_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current);
void ZDT_V5_Pos_Control_Trapezoidal_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current);
#endif

/******************** 快速位置模式(User) **********************/

#if MOTOR_CMD_FAST
void ZDT_V5_Fast_Set_Param(uint8_t addr, uint16_t vel, uint16_t acc, uint16_t dec, uint16_t max_current, uint8_t mode, uint8_t sync);
void ZDT_V5_Fast_Send_Pos(uint8_t addr, int32_t pos);
#endif

/******************** 回零点指令 **********************/

#if MOTOR_CMD_HOME
void ZDT_V5_Origin_Set_O(uint8_t addr, bool svF);
void ZDT_V5_Origin_Trigger_Return(uint8_t addr, uint8_t o_mode, bool snF);
void ZDT_V5_Origin_Interrupt(uint8_t addr);
void ZDT_V5_Origin_Read_Params(uint8_t addr);
void ZDT_V5_Origin_Modify_Params(uint8_t addr, bool svF, uint8_t o_mode, uint8_t o_dir, uint16_t o_vel, uint32_t o_tm, uint16_t sl_vel, uint16_t sl_ma, uint16_t sl_ms, bool potF);
#endif

/******************** 读取系统参数命令 **********************/

void ZDT_V5_Auto_Return_Sys_Params_Timed(uint8_t addr, SysParams_t s, uint16_t time_ms);
void ZDT_V5_Read_Sys_Params(uint8_t addr, SysParams_t s);
// void ZDT_V5_Read_Batch_Status(uint8_t addr);
void ZDT_V5_Read_Batch_Config(uint8_t addr);

#if MOTOR_SYSTEM
void ZDT_V5_Read_Option_Params(uint8_t addr);
void ZDT_V5_Read_Version_Info(uint8_t addr);
void ZDT_V5_Read_Phase_Params(uint8_t addr);
#endif

void ZDT_V5_Read_Motor_ID(uint8_t addr);

#if MOTOR_CONTROL
void ZDT_V5_Read_PID_Params(uint8_t addr);
void ZDT_V5_Read_Pos_Window(uint8_t addr);
void ZDT_V5_Read_Integral_Limit(uint8_t addr);
#endif
#if MOTOR_PROTECTION
void ZDT_V5_Read_Otocp(uint8_t addr);
void ZDT_V5_Read_Heart_Protect(uint8_t addr);
void ZDT_V5_Read_Collision_Angle(uint8_t addr);
#endif

/******************** 读写驱动参数命令 **********************/

#if MOTOR_DRIVER
void ZDT_V5_Modify_Motor_ID(uint8_t addr, bool svF, uint8_t id);
void ZDT_V5_Modify_MicroStep(uint8_t addr, bool svF, uint8_t mstep);
void ZDT_V5_Modify_PDFlag(uint8_t addr, bool pdf);
void ZDT_V5_Modify_Motor_Type(uint8_t addr, bool svF, bool mottype);
void ZDT_V5_Modify_Firmware_Type(uint8_t addr, bool svF, bool fwtype);
void ZDT_V5_Modify_Ctrl_Mode(uint8_t addr, bool svF, bool ctrl_mode);
void ZDT_V5_Modify_Motor_Dir(uint8_t addr, bool svF, bool dir);
void ZDT_V5_Modify_Lock_Btn(uint8_t addr, bool svF, bool lockbtn);
void ZDT_V5_Modify_S_Vel(uint8_t addr, bool svF, bool s_vel);
void ZDT_V5_Modify_Lock_Params(uint8_t addr, bool svF, uint8_t lock_level);
#endif
#if MOTOR_CURRENT
void ZDT_V5_Modify_OM_mA(uint8_t addr, bool svF, uint16_t om_ma);
void ZDT_V5_Modify_FOC_mA(uint8_t addr, bool svF, uint16_t foc_mA);
#endif
#if MOTOR_CONTROL
void ZDT_V5_Modify_PID_Params(uint8_t addr, bool svF, uint32_t kp, uint32_t ki, uint32_t kd);
void ZDT_V5_Modify_Integral_Limit(uint8_t addr, bool svF, uint32_t il);
void ZDT_V5_Modify_Pos_Window(uint8_t addr, bool svF, uint16_t prw);
#endif
#if MOTOR_PROTECTION
void ZDT_V5_Modify_Otocp(uint8_t addr, bool svF, uint16_t otp, uint16_t ocp, uint16_t time_ms);
void ZDT_V5_Modify_Heart_Protect(uint8_t addr, bool svF, uint32_t hp);
void ZDT_V5_Modify_Collision_Angle(uint8_t addr, bool svF, uint16_t angle);
#elif USE_HEARTBEAT
void ZDT_V5_Modify_Heart_Protect(uint8_t addr, bool svF, uint32_t hp);
#endif
#if MOTOR_COMM
void ZDT_V5_Modify_Comm_Params(uint8_t addr, bool svF, uint8_t uart_baudrate, uint8_t can_baudrate, uint8_t verify_mode, uint8_t response_mode);
#endif

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42 && MOTOR_DRIVER
void ZDT_V5_Read_DMX512_Params(uint8_t addr);
void ZDT_V5_Modify_DMX512_Params(uint8_t addr, bool svF, uint16_t tch, uint8_t nch, uint8_t mode, uint16_t vel, uint16_t acc, uint16_t vel_step, uint32_t pos_step);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ZDT_V5_DRIVER_H__ */
