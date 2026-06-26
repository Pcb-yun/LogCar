/**
 * @file ZDT_V5_Driver.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机驱动层源文件
 */

#include "ZDT_V5_Driver.h"

/******************** 系统状态参数 *********************/

/**
 * @brief 读取系统参数
 * @param addr 电机地址
 * @param s 系统参数类型
 */
void ZDT_V5_Read_Sys_Params(uint8_t addr, SysParams_t s) {
	uint8_t i = 0; uint8_t cmd[16] = {0};
	cmd[i] = addr; ++i;
	switch(s) {
#if MOTOR_STATUS_READ_BATCH
	case S_BATCH: cmd[i] = CMD_READ_SYSTEM_STATUS; ++i; cmd[i] = AUX_CODE_SYSTEM_STATUS; ++i; break;
#endif
#if MOTOR_STATUS_BUS_VOLTAGE
	case S_VBUS : cmd[i] = CMD_READ_BUS_VOLTAGE; ++i; break;
#endif
#if MOTOR_STATUS_BUS_CURRENT
	case S_CBUS : cmd[i] = CMD_READ_BUS_CURRENT; ++i; break;
#endif
#if MOTOR_STATUS_PHASE_CURRENT
	case S_CPHA : cmd[i] = CMD_READ_PHASE_CURRENT; ++i; break;
#endif
#if MOTOR_STATUS_ENCODER_VALUE
	case S_ENCL : cmd[i] = CMD_READ_ENCODER_VALUE; ++i; break;
#endif
#if MOTOR_STATUS_INPUT_PULSES
	case S_CLKI : cmd[i] = CMD_READ_INPUT_PULSES; ++i; break;
#endif
#if MOTOR_STATUS_TARGET_POS
	case S_TPOS : cmd[i] = CMD_READ_TARGET_POSITION; ++i; break;
#endif
#if MOTOR_STATUS_SET_POS
	case S_SPOS : cmd[i] = CMD_READ_SET_POSITION; ++i; break;
#endif
#if MOTOR_STATUS_SPEED
	case S_VEL  : cmd[i] = CMD_READ_SPEED; ++i; break;
#endif
#if MOTOR_STATUS_REAL_POS
	case S_CPOS : cmd[i] = CMD_READ_POSITION; ++i; break;
#endif
#if MOTOR_STATUS_POS_ERROR
	case S_PERR : cmd[i] = CMD_READ_POSITION_ERROR; ++i; break;
#endif
#if MOTOR_STATUS_BATTERY_VOLTAGE
	case S_VBAT : cmd[i] = CMD_READ_BATTERY_VOLTAGE; ++i; break;
#endif
#if MOTOR_STATUS_TEMPERATURE
	case S_TEMP : cmd[i] = CMD_READ_TEMPERATURE; ++i; break;
#endif
#if MOTOR_STATUS_MOTOR_FLAGS
	case S_FLAG : cmd[i] = CMD_READ_MOTOR_STATUS; ++i; break;
#endif
#if MOTOR_STATUS_HOME_FLAGS
	case S_OFLAG: cmd[i] = CMD_READ_HOME_STATUS; ++i; break;
#endif
#if MOTOR_STATUS_FLAGS_COMBINED
	case S_OAF  : cmd[i] = CMD_READ_STATUS_FLAGS; ++i; break;
#endif
#if MOTOR_STATUS_PIN_STATUS
	case S_PIN  : cmd[i] = CMD_READ_PIN_STATUS; ++i; break;
#endif
	default: break;
	}
	cmd[i] = END_CODE; ++i;
	ZDT_V5_SEND_CMD(cmd, i);
}

#if MOTOR_PERIODIC_RETURN
/**
 * @brief 定时返回信息命令（X42S/Y42）
 * @param addr 电机地址
 * @param s 系统参数类型
 * @param time_ms 定时时间
 */
void ZDT_V5_Auto_Return_Sys_Params_Timed(uint8_t addr, SysParams_t s, uint16_t time_ms) {
	uint8_t i = 0; uint8_t cmd[16] = {0};
	cmd[i] = addr; ++i;
	cmd[i] = CMD_SET_TIMER_REPORT; ++i;
	cmd[i] = AUX_CODE_TIMER_REPORT; ++i;
	switch(s) {
#if MOTOR_STATUS_BUS_VOLTAGE
		case S_VBUS : cmd[i] = CMD_READ_BUS_VOLTAGE; ++i; break;
#endif
#if MOTOR_STATUS_BUS_CURRENT
		case S_CBUS : cmd[i] = CMD_READ_BUS_CURRENT; ++i; break;
#endif
#if MOTOR_STATUS_PHASE_CURRENT
		case S_CPHA : cmd[i] = CMD_READ_PHASE_CURRENT; ++i; break;
#endif
#if MOTOR_STATUS_ENCODER_VALUE
		case S_ENCL : cmd[i] = CMD_READ_ENCODER_VALUE; ++i; break;
#endif
#if MOTOR_STATUS_INPUT_PULSES
		case S_CLKI : cmd[i] = CMD_READ_INPUT_PULSES; ++i; break;
#endif
#if MOTOR_STATUS_TARGET_POS
		case S_TPOS : cmd[i] = CMD_READ_TARGET_POSITION; ++i; break;
#endif
#if MOTOR_STATUS_SET_POS
		case S_SPOS : cmd[i] = CMD_READ_SET_POSITION; ++i; break;
#endif
#if MOTOR_STATUS_SPEED
		case S_VEL  : cmd[i] = CMD_READ_SPEED; ++i; break;
#endif
#if MOTOR_STATUS_REAL_POS
		case S_CPOS : cmd[i] = CMD_READ_POSITION; ++i; break;
#endif
#if MOTOR_STATUS_POS_ERROR
		case S_PERR : cmd[i] = CMD_READ_POSITION_ERROR; ++i; break;
#endif
#if MOTOR_STATUS_BATTERY_VOLTAGE
		case S_VBAT : cmd[i] = CMD_READ_BATTERY_VOLTAGE; ++i; break;
#endif
#if MOTOR_STATUS_TEMPERATURE
		case S_TEMP : cmd[i] = CMD_READ_TEMPERATURE; ++i; break;
#endif
#if MOTOR_STATUS_MOTOR_FLAGS
		case S_FLAG : cmd[i] = CMD_READ_MOTOR_STATUS; ++i; break;
#endif
#if MOTOR_STATUS_HOME_FLAGS
		case S_OFLAG: cmd[i] = CMD_READ_HOME_STATUS; ++i; break;
#endif
#if MOTOR_STATUS_FLAGS_COMBINED
		case S_OAF  : cmd[i] = CMD_READ_STATUS_FLAGS; ++i; break;
#endif
#if MOTOR_STATUS_PIN_STATUS
		case S_PIN  : cmd[i] = CMD_READ_PIN_STATUS; ++i; break;
#endif
	default: break;
	}
	cmd[i] = (uint8_t)(time_ms >> 8); ++i;
	cmd[i] = (uint8_t)(time_ms >> 0); ++i;
	cmd[i] = END_CODE; ++i;
	ZDT_V5_SEND_CMD(cmd, i);
}
#endif

/******************** 驱动配置参数 *********************/

/**
 * @brief 读取驱动参数
 * @param addr 电机地址
 * @param d 驱动参数类型
 */
void ZDT_V5_Read_Driver_Params(uint8_t addr, DriverParams_t d) {
	uint8_t i = 0; uint8_t cmd[16] = {0};
	cmd[i] = addr; ++i;
	switch(d) {
#if MOTOR_DRIVER_READ_BATCH
		case D_BATCH: cmd[i] = CMD_READ_DRIVER_CONFIG; ++i; cmd[i] = AUX_CODE_DRIVER_CONFIG; ++i; break;
#endif
#if MOTOR_DRIVER_POS_WINDOW
		case D_POS_WINDOW: cmd[i] = CMD_READ_POSITION_WINDOW; ++i; break;
#endif
#if MOTOR_DRIVER_HOME
		case D_HOME: cmd[i] = CMD_READ_HOME_PARAMS; ++i; break;
#endif
	default: break;
	}
	cmd[i] = END_CODE; ++i;
	ZDT_V5_SEND_CMD(cmd, i);
}

/******************** 控制参数 *********************/

/**
 * @brief 读取控制参数
 * @param addr 电机地址
 * @param c 控制参数类型
 */
void ZDT_V5_Read_Ctrl_Params(uint8_t addr, CtrlParams_t c) {
	uint8_t i = 0; uint8_t cmd[16] = {0};
	cmd[i] = addr; ++i;
	switch(c) {
#if MOTOR_PID_READ
	case C_PID : cmd[i] = CMD_READ_PID_PARAMS; ++i; break;
#endif
#if MOTOR_INTEGRAL_LIMIT_READ
	case C_INTEGRAL_LIMIT : cmd[i] = CMD_READ_INTEGRAL_LIMIT; ++i; break;
#endif
#if MOTOR_PROTECT_THRESHOLD_READ
	case C_PROTECT_THRESHOLD : cmd[i] = CMD_READ_PROTECT_THRESHOLD; ++i; break;
#endif
#if MOTOR_COLLISION_ANGLE_READ
	case C_COLLISION_ANGLE : cmd[i] = CMD_READ_COLLISION_ANGLE; ++i; break;
#endif
#if MOTOR_HEARTBEAT_READ
	case C_HEARTBEAT : cmd[i] = CMD_READ_HEARTBEAT_TIME; ++i; break;
#endif
	default: break;
	}
	cmd[i] = END_CODE; ++i;
	ZDT_V5_SEND_CMD(cmd, i);
}

#if MOTOR_PID_WRITE
#if CURRENT_FIRMWARE == FIRMWARE_EMM
/**
 * @brief 修改PID参数（Emm固件）
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param kp 比例系数Kp
 * @param ki 积分系数Ki
 * @param kd 微分系数Kd
 */
void ZDT_V5_Modify_PID_Params(uint8_t addr, bool svF, uint32_t kp, uint32_t ki, uint32_t kd) {
	uint8_t cmd[24] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_PID_PARAMS; cmd[2] = AUX_CODE_SET_PID_PARAMS; cmd[3] = svF;
	cmd[4] = (uint8_t)(kp >> 24); cmd[5] = (uint8_t)(kp >> 16); cmd[6] = (uint8_t)(kp >> 8); cmd[7] = (uint8_t)(kp >> 0);
	cmd[8] = (uint8_t)(ki >> 24); cmd[9] = (uint8_t)(ki >> 16); cmd[10] = (uint8_t)(ki >> 8); cmd[11] = (uint8_t)(ki >> 0);
	cmd[12] = (uint8_t)(kd >> 24); cmd[13] = (uint8_t)(kd >> 16); cmd[14] = (uint8_t)(kd >> 8); cmd[15] = (uint8_t)(kd >> 0);
	cmd[16] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 17);
}
#elif CURRENT_FIRMWARE == FIRMWARE_X
/**
 * @brief 修改PID参数（X固件）
 * @param addr 电机地址
 * @param svF 是否存储标志
 * @param trapezoidal_kp 梯形曲线位置环Kp
 * @param direct_kp 直通限速位置环Kp
 * @param vel_kp 速度环Kp
 * @param vel_ki 速度环Ki
 */
void ZDT_V5_Modify_PID_Params(uint8_t addr, bool svF, uint32_t trapezoidal_kp, uint32_t direct_kp, uint32_t vel_kp, uint32_t vel_ki) {
	uint8_t cmd[24] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_PID_PARAMS; cmd[2] = AUX_CODE_SET_PID_PARAMS; cmd[3] = svF;
	cmd[4] = (uint8_t)(trapezoidal_kp >> 24); cmd[5] = (uint8_t)(trapezoidal_kp >> 16); cmd[6] = (uint8_t)(trapezoidal_kp >> 8); cmd[7] = (uint8_t)(trapezoidal_kp >> 0);
	cmd[8] = (uint8_t)(direct_kp >> 24); cmd[9] = (uint8_t)(direct_kp >> 16); cmd[10] = (uint8_t)(direct_kp >> 8); cmd[11] = (uint8_t)(direct_kp >> 0);
	cmd[12] = (uint8_t)(vel_kp >> 24); cmd[13] = (uint8_t)(vel_kp >> 16); cmd[14] = (uint8_t)(vel_kp >> 8); cmd[15] = (uint8_t)(vel_kp >> 0);
	cmd[16] = (uint8_t)(vel_ki >> 24); cmd[17] = (uint8_t)(vel_ki >> 16); cmd[18] = (uint8_t)(vel_ki >> 8); cmd[19] = (uint8_t)(vel_ki >> 0);
	cmd[20] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 21);
}
#endif
#endif

#if MOTOR_INTEGRAL_LIMIT_WRITE
/**
 * @brief 修改积分限幅/刚性系数
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param il 积分限幅值
 */
void ZDT_V5_Modify_Integral_Limit(uint8_t addr, bool svF, uint32_t il) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_INTEGRAL_LIMIT; cmd[2] = AUX_CODE_SET_INTEGRAL_LIMIT; cmd[3] = svF;
	cmd[4] = (uint8_t)(il >> 24); cmd[5] = (uint8_t)(il >> 16); cmd[6] = (uint8_t)(il >> 8); cmd[7] = (uint8_t)(il >> 0);
	cmd[8] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 9);
}
#endif

#if MOTOR_POS_WINDOW_WRITE
/**
 * @brief 修改位置到达窗口
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param prw 位置到达窗口值
 */
void ZDT_V5_Modify_Pos_Window(uint8_t addr, bool svF, uint16_t prw) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_POS_WINDOW; cmd[2] = AUX_CODE_SET_POS_WINDOW; cmd[3] = svF;
	cmd[4] = (uint8_t)(prw >> 8); cmd[5] = (uint8_t)(prw >> 0);
	cmd[6] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 7);
}
#endif

#if MOTOR_PROTECT_THRESHOLD_WRITE
/**
 * @brief 修改过热过流保护阈值
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param otp 温度保护阈值(℃)
 * @param ocp 电流保护阈值(mA)
 * @param time_ms 保护触发时间(ms)
 */
void ZDT_V5_Modify_Otocp(uint8_t addr, bool svF, uint16_t otp, uint16_t ocp, uint16_t time_ms) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_PROTECT_THRESHOLD; cmd[2] = AUX_CODE_SET_PROTECT_THRESHOLD; cmd[3] = svF;
	cmd[4] = (uint8_t)(otp >> 8); cmd[5] = (uint8_t)(otp >> 0);
	cmd[6] = (uint8_t)(ocp >> 8); cmd[7] = (uint8_t)(ocp >> 0);
	cmd[8] = (uint8_t)(time_ms >> 8); cmd[9] = (uint8_t)(time_ms >> 0);
	cmd[10] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 11);
}
#endif

#if MOTOR_HEARTBEAT_WRITE
/**
 * @brief 修改心跳保护时间
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param hp 心跳保护时间(ms)
 */
void ZDT_V5_Modify_Heart_Protect(uint8_t addr, bool svF, uint32_t hp) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_HEARTBEAT_TIME; cmd[2] = AUX_CODE_SET_HEARTBEAT_TIME; cmd[3] = svF;
	cmd[4] = (uint8_t)(hp >> 24); cmd[5] = (uint8_t)(hp >> 16); cmd[6] = (uint8_t)(hp >> 8); cmd[7] = (uint8_t)(hp >> 0);
	cmd[8] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 9);
}
#endif

#if MOTOR_COLLISION_ANGLE_WRITE
/**
 * @brief 修改碰撞回零返回角度
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param angle 碰撞回零返回角度
 */
void ZDT_V5_Modify_Collision_Angle(uint8_t addr, bool svF, uint16_t angle) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_COLLISION_ANGLE; cmd[2] = AUX_CODE_SET_COLLISION_ANGLE; cmd[3] = svF;
	cmd[4] = (uint8_t)(angle >> 8); cmd[5] = (uint8_t)(angle >> 0);
	cmd[6] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 7);
}
#endif

/******************** 设备信息与特殊功能 *********************/

/**
 * @brief 读取单个设备信息参数
 * @param addr 电机地址
 * @param i 设备信息参数类型
 */
void ZDT_V5_Read_Device_Info_Params(uint8_t addr, DeviceInfo_t i) {
	uint8_t j = 0; uint8_t cmd[16] = {0};
	cmd[j] = addr; ++j;
	switch(i) {
#if MOTOR_READ_VERSION
	case I_VERSION : cmd[j] = CMD_READ_VERSION_INFO; ++j; break;
#endif
#if MOTOR_READ_PHASE_PARAMS
	case I_PHASE_PARAMS : cmd[j] = CMD_READ_PHASE_PARAMS; ++j; break;
#endif
#if MOTOR_READ_OPTION_PARAMS
	case I_OPTION : cmd[j] = CMD_READ_OPTION_PARAMS; ++j; break;
#endif
#if MOTOR_BROADCAST_READ_ID
	case I_ID : cmd[j] = CMD_BROADCAST_READ_ID; ++j; break;
#endif
#if MOTOR_DRIVER_DMX512
	case I_DMX512 : cmd[j] = CMD_READ_DMX512_PARAMS; ++j; cmd[j] = AUX_CODE_READ_DMX512; ++j; break;
#endif
	default: break;
	}
	cmd[j] = END_CODE; ++j;
	ZDT_V5_SEND_CMD(cmd, j);
}

/******************** 运动控制命令 *********************/

#if MOTOR_CMD_ENABLE
/**
 * @brief 使能信号控制
 * @param addr 电机地址
 * @param state 使能状态,true为使能电机,false为关闭电机
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_En_Control(uint8_t addr, bool state, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_MOTOR_ENABLE; cmd[2] = AUX_CODE_MOTOR_ENABLE; cmd[3] = (uint8_t)state; cmd[4] = snF; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_VELOCITY_MODE
/**
 * @brief 速度模式
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param vel 速度(RPM) 0-5000
 * @param acc 加速度(RPM/S或档位)
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_Vel_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_VELOCITY_MODE;
#if CURRENT_FIRMWARE == FIRMWARE_X
	cmd[2] = dir;
	cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
	cmd[5] = (uint8_t)(vel >> 8); cmd[6] = (uint8_t)(vel >> 0);
	cmd[7] = snF; cmd[8] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 9);
#else
	cmd[2] = dir; cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
	cmd[5] = (uint8_t)acc; cmd[6] = snF; cmd[7] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 8);
#endif
}
#endif

#if MOTOR_VELOCITY_MODE_LIMIT
/**
 * @brief 速度模式限电流控制（X固件增强版）
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param vel 速度(RPM) 0-3000.0
 * @param acc 加速度(RPM/S) 0-65535
 * @param snF 多机同步标志,false为不启用,true为启用
 * @param max_current 最大电流(mA) 0-5000
 */
void ZDT_V5_Vel_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, bool snF, uint16_t max_current) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_VELOCITY_MODE_LIMIT; cmd[2] = dir;
	cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
	cmd[5] = (uint8_t)(vel >> 8); cmd[6] = (uint8_t)(vel >> 0);
	cmd[7] = snF;
	cmd[8] = (uint8_t)(max_current >> 8); cmd[9] = (uint8_t)(max_current >> 0);
	cmd[10] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 11);
}
#endif

#if MOTOR_POS_MODE_DIRECT
/**
 * @brief 直通限速位置模式控制（X固件）
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param vel 速度(RPM) 0-3000.0
 * @param pos 位置角度(0.1°) 0- (2^32 - 1)
 * @param mode 运动模式:0-相对上一目标,1-绝对位置,2-相对当前位置
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_Pos_Control_Direct(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_POS_MODE_DIRECT; cmd[2] = dir;
	cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
	cmd[5] = (uint8_t)(pos >> 24); cmd[6] = (uint8_t)(pos >> 16); cmd[7] = (uint8_t)(pos >> 8); cmd[8] = (uint8_t)(pos >> 0);
	cmd[9] = mode; cmd[10] = snF; cmd[11] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 12);
}
#endif

#if MOTOR_POS_MODE_DIRECT_LIMIT
/**
 * @brief 直通限速位置模式限电流控制（X固件增强版）
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param vel 速度(RPM) 0-3000.0
 * @param pos 位置角度(0.1°) 0- (2^32 - 1)
 * @param mode 运动模式:0-相对上一目标,1-绝对位置,2-相对当前位置
 * @param snF 多机同步标志,false为不启用,true为启用
 * @param max_current 最大电流(mA) 0-5000
 */
void ZDT_V5_Pos_Control_Direct_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_POS_MODE_DIRECT_LIMIT; cmd[2] = dir;
	cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
	cmd[5] = (uint8_t)(pos >> 24); cmd[6] = (uint8_t)(pos >> 16); cmd[7] = (uint8_t)(pos >> 8); cmd[8] = (uint8_t)(pos >> 0);
	cmd[9] = mode; cmd[10] = snF;
	cmd[11] = (uint8_t)(max_current >> 8); cmd[12] = (uint8_t)(max_current >> 0);
	cmd[13] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 14);
}
#endif

#if MOTOR_POS_MODE_TRAPEZOIDAL
/**
 * @brief 位置模式
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param vel 速度(RPM) 0-5000
 * @param acc 加速度(RPM/S或档位)
 * @param dec 减速度(仅X固件)
 * @param clk 脉冲数/角度 0- (2^32 - 1)
 * @param raF 相位/绝对标志,false为相对运动,true为绝对值运动
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_Pos_Control(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t clk, bool raF, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_POS_MODE_TRAPEZOIDAL;
#if CURRENT_FIRMWARE == FIRMWARE_X
	cmd[2] = dir;
	cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
	cmd[5] = (uint8_t)(dec >> 8); cmd[6] = (uint8_t)(dec >> 0);
	cmd[7] = (uint8_t)(vel >> 8); cmd[8] = (uint8_t)(vel >> 0);
	cmd[9] = (uint8_t)(clk >> 24); cmd[10] = (uint8_t)(clk >> 16); cmd[11] = (uint8_t)(clk >> 8); cmd[12] = (uint8_t)(clk >> 0);
	cmd[13] = (uint8_t)raF; cmd[14] = snF; cmd[15] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 16);
#else
	cmd[2] = dir; cmd[3] = (uint8_t)(vel >> 8); cmd[4] = (uint8_t)(vel >> 0);
	cmd[5] = (uint8_t)acc;
	cmd[6] = (uint8_t)(clk >> 24); cmd[7] = (uint8_t)(clk >> 16); cmd[8] = (uint8_t)(clk >> 8); cmd[9] = (uint8_t)(clk >> 0);
	cmd[10] = raF; cmd[11] = snF; cmd[12] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 13);
#endif
}
#endif

#if MOTOR_POS_MODE_TRAPEZOIDAL_LIMIT
/**
 * @brief 梯形曲线位置模式限电流控制（X固件增强版）
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param vel 速度(RPM) 0-3000.0
 * @param acc 加速度(RPM/S) 0-65535
 * @param dec 减速度(RPM/S) 0-65535
 * @param pos 位置角度(0.1°) 0- (2^32 - 1)
 * @param mode 运动模式:0-相对上一目标,1-绝对位置,2-相对当前位置
 * @param snF 多机同步标志,false为不启用,true为启用
 * @param max_current 最大电流(mA) 0-5000
 */
void ZDT_V5_Pos_Control_Trapezoidal_With_Limit(uint8_t addr, uint8_t dir, uint16_t vel, uint16_t acc, uint16_t dec, uint32_t pos, uint8_t mode, bool snF, uint16_t max_current) {
	uint8_t cmd[24] = {0};
	cmd[0] = addr; cmd[1] = CMD_POS_MODE_TRAPEZOIDAL_LIMIT; cmd[2] = dir;
	cmd[3] = (uint8_t)(acc >> 8); cmd[4] = (uint8_t)(acc >> 0);
	cmd[5] = (uint8_t)(dec >> 8); cmd[6] = (uint8_t)(dec >> 0);
	cmd[7] = (uint8_t)(vel >> 8); cmd[8] = (uint8_t)(vel >> 0);
	cmd[9] = (uint8_t)(pos >> 24); cmd[10] = (uint8_t)(pos >> 16); cmd[11] = (uint8_t)(pos >> 8); cmd[12] = (uint8_t)(pos >> 0);
	cmd[13] = mode; cmd[14] = snF;
	cmd[15] = (uint8_t)(max_current >> 8); cmd[16] = (uint8_t)(max_current >> 0);
	cmd[17] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 18);
}
#endif

#if MOTOR_TORQUE_MODE
/**
 * @brief 力矩模式控制
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param slope 电流斜率(mA/S) 0-65535
 * @param current 目标电流(mA) 0-5000
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_Torque_Control(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_TORQUE_MODE; cmd[2] = dir;
	cmd[3] = (uint8_t)(slope >> 8); cmd[4] = (uint8_t)(slope >> 0);
	cmd[5] = (uint8_t)(current >> 8); cmd[6] = (uint8_t)(current >> 0);
	cmd[7] = snF; cmd[8] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 9);
}
#endif

#if MOTOR_TORQUE_MODE_LIMIT
/**
 * @brief 力矩模式限速控制（增强版）
 * @param addr 电机地址
 * @param dir 方向,0为CW，其余值为CCW
 * @param slope 电流斜率(mA/S) 0-65535
 * @param current 目标电流(mA) 0-5000
 * @param max_vel 最大速度(0.1RPM) 0-30000
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_Torque_Control_With_Limit(uint8_t addr, uint8_t dir, uint16_t slope, uint16_t current, uint16_t max_vel, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_TORQUE_MODE_LIMIT; cmd[2] = dir;
	cmd[3] = (uint8_t)(slope >> 8); cmd[4] = (uint8_t)(slope >> 0);
	cmd[5] = (uint8_t)(current >> 8); cmd[6] = (uint8_t)(current >> 0);
	cmd[7] = snF;
	cmd[8] = (uint8_t)(max_vel >> 8); cmd[9] = (uint8_t)(max_vel >> 0);
	cmd[10] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 11);
}
#endif

#if MOTOR_CMD_STOP
/**
 * @brief 立即停止
 * @param addr 电机地址
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_Stop(uint8_t addr, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_STOP_NOW; cmd[2] = AUX_CODE_STOP_NOW; cmd[3] = snF; cmd[4] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 5);
}
#endif

#if MOTOR_SYNC_TRIGGER
/**
 * @brief 多机同步运动
 * @param addr 电机地址
 */
void ZDT_V5_Synchronous_Motion(uint8_t addr) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SYNC_MOTION; cmd[2] = AUX_CODE_SYNC_MOTION; cmd[3] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 4);
}
#endif

#if MOTOR_MULTI_CMD
/**
 * @brief 多电机命令（X42S/Y42）
 * @param addr 电机地址
 * @param len 总字节数
 * @param cmd_data 命令数据
 */
void ZDT_V5_Multi_Motor_Cmd(uint8_t addr, uint8_t len, uint8_t *cmd_data) {
	uint8_t cmd[64] = {0};
	uint8_t i = 0;
	cmd[i] = addr; ++i;
	cmd[i] = CMD_MULTI_MOTOR; ++i;
	cmd[i] = len; ++i;
	for(uint8_t n = 0; n < len; n++) {
		cmd[i] = cmd_data[n]; ++i;
	}
	cmd[i] = END_CODE; ++i;
	ZDT_V5_SEND_CMD(cmd, i);
}
#endif

#if MOTOR_POS_MODE_FAST
/**
 * @brief 快速位置模式 - 设定参数
 * @param addr 电机地址
 * @param vel 速度（RPM）
 * @param acc 加速度（Emm：档位，X：加速加速度）
 * @param dec 减速度（仅X固件）
 * @param max_current 最大电流（仅X固件）
 * @param mode 运动模式：0-相对上一目标，1-绝对位置，2-相对当前位置
 * @param sync 同步标志：0-立即执行，1-等待同步触发
 */
void ZDT_V5_Fast_Set_Param(uint8_t addr, uint16_t vel, uint16_t acc, uint16_t dec, uint16_t max_current, uint8_t mode, uint8_t sync) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_POS_MODE_FAST_SET;
#if CURRENT_FIRMWARE == FIRMWARE_EMM
	cmd[2] = (uint8_t)(vel >> 8); cmd[3] = (uint8_t)(vel >> 0);
	cmd[4] = (uint8_t)acc;
	cmd[5] = mode;
	cmd[6] = sync;
	cmd[7] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 8);
#else
	cmd[2] = (uint8_t)(acc >> 8); cmd[3] = (uint8_t)(acc >> 0);
	cmd[4] = (uint8_t)(dec >> 8); cmd[5] = (uint8_t)(dec >> 0);
	cmd[6] = (uint8_t)(vel >> 8); cmd[7] = (uint8_t)(vel >> 0);
	cmd[8] = mode;
	cmd[9] = sync;
	cmd[10] = (uint8_t)(max_current >> 8); cmd[11] = (uint8_t)(max_current >> 0);
	cmd[12] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 13);
#endif
}

/**
 * @brief 快速位置模式 - 发送位置
 * @param addr 电机地址
 * @param pos 目标位置（Emm：脉冲数，X：角度）
 */
void ZDT_V5_Fast_Send_Pos(uint8_t addr, int32_t pos) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_POS_MODE_FAST_SEND;
	cmd[2] = (uint8_t)(pos >> 24); cmd[3] = (uint8_t)(pos >> 16);
	cmd[4] = (uint8_t)(pos >> 8); cmd[5] = (uint8_t)(pos >> 0);
	cmd[6] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 7);
}
#endif

/******************** 触发动作命令 *********************/

#if MOTOR_TRIGGER_ENCODER_CALIB
/**
 * @brief 触发编码器校准
 * @param addr 电机地址
 */
void ZDT_V5_Trig_Encoder_Cal(uint8_t addr) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_TRIGGER_ENCODER_CALIB; cmd[2] = AUX_CODE_ENCODER_CALIB; cmd[3] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 4);
}
#endif

#if MOTOR_TRIGGER_RESTART
/**
 * @brief 重启电机（X42S/Y42）
 * @param addr 电机地址
 */
void ZDT_V5_Reset_Motor(uint8_t addr) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_RESTART_MOTOR; cmd[2] = AUX_CODE_RESTART_MOTOR; cmd[3] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 4);
}
#endif

#if MOTOR_TRIGGER_RESET_POS
/**
 * @brief 将当前位置清零
 * @param addr 电机地址
 */
void ZDT_V5_Reset_CurPos_To_Zero(uint8_t addr) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_RESET_CURPOS_TO_ZERO; cmd[2] = AUX_CODE_RESET_POS; cmd[3] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 4);
}
#endif

#if MOTOR_TRIGGER_CLEAR_PROTECT
/**
 * @brief 解除堵转/过热/过流保护
 * @param addr 电机地址
 */
void ZDT_V5_Reset_Clog_Pro(uint8_t addr) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_CLEAR_PROTECTION; cmd[2] = AUX_CODE_CLEAR_PROTECT; cmd[3] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 4);
}
#endif

#if MOTOR_TRIGGER_FACTORY_RESET
/**
 * @brief 恢复出厂设置
 * @param addr 电机地址
 */
void ZDT_V5_Restore_Motor(uint8_t addr) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_RESTORE_FACTORY; cmd[2] = AUX_CODE_RESTORE_FACTORY; cmd[3] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 4);
}
#endif

/******************** 原点回零命令 *********************/

#if MOTOR_HOME_SET_ZERO
/**
 * @brief 设置单圈回零的零点位置
 * @param addr 电机地址
 * @param svF 存储标志,false为不存储,true为存储
 */
void ZDT_V5_Origin_Set_Zero(uint8_t addr, bool svF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_HOME_ZERO; cmd[2] = AUX_CODE_SET_HOME_ZERO; cmd[3] = svF; cmd[4] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 5);
}
#endif

#if MOTOR_HOME_TRIGGER
/**
 * @brief 触发回零
 * @param addr 电机地址
 * @param o_mode 回零模式,0为单圈就近回零,1为单圈方向回零,2为多圈无限位碰撞回零,3为多圈有限位开关回零
 * @param snF 多机同步标志,false为不启用,true为启用
 */
void ZDT_V5_Origin_Trigger_Return(uint8_t addr, uint8_t o_mode, bool snF) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_TRIGGER_HOME; cmd[2] = o_mode; cmd[3] = snF; cmd[4] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 5);
}
#endif

#if MOTOR_HOME_INTERRUPT
/**
 * @brief 强制中断并退出回零
 * @param addr 电机地址
 */
void ZDT_V5_Origin_Interrupt(uint8_t addr) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_HOME_INTERRUPT; cmd[2] = AUX_CODE_HOME_INTERRUPT; cmd[3] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 4);
}
#endif

#if MOTOR_DRIVER_HOME_WRITE
/**
 * @brief 修改回零参数
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param o_mode 回零模式,0为单圈就近回零,1为单圈方向回零,2为多圈无限位碰撞回零,3为多圈有限位开关回零
 * @param o_dir 回零方向,0为CW，其余值为CCW
 * @param o_vel 回零速度,单位：RPM（转/分钟）
 * @param o_tm 回零超时时间,单位：毫秒
 * @param sl_vel 无限位碰撞回零检测转速,单位：RPM（转/分钟）
 * @param sl_ma 无限位碰撞回零检测电流,单位：Ma（毫安）
 * @param sl_ms 无限位碰撞回零检测时间,单位：Ms（毫秒）
 * @param potF 上电自动触发回零,false为不使能,true为使能
 */
void ZDT_V5_Origin_Modify_Params(uint8_t addr, bool svF, uint8_t o_mode, uint8_t o_dir, uint16_t o_vel, uint32_t o_tm, uint16_t sl_vel, uint16_t sl_ma, uint16_t sl_ms, bool potF) {
	uint8_t cmd[32] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_HOME_PARAMS; cmd[2] = AUX_CODE_SET_HOME_PARAMS; cmd[3] = svF; cmd[4] = o_mode; cmd[5] = o_dir;
	cmd[6] = (uint8_t)(o_vel >> 8); cmd[7] = (uint8_t)(o_vel >> 0);
	cmd[8] = (uint8_t)(o_tm >> 24); cmd[9] = (uint8_t)(o_tm >> 16); cmd[10] = (uint8_t)(o_tm >> 8); cmd[11] = (uint8_t)(o_tm >> 0);
	cmd[12] = (uint8_t)(sl_vel >> 8); cmd[13] = (uint8_t)(sl_vel >> 0);
	cmd[14] = (uint8_t)(sl_ma >> 8); cmd[15] = (uint8_t)(sl_ma >> 0);
	cmd[16] = (uint8_t)(sl_ms >> 8); cmd[17] = (uint8_t)(sl_ms >> 0);
	cmd[18] = potF; cmd[19] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 20);
}
#endif

/******************** 电机参数设置 *********************/

#if MOTOR_SET_MOTOR_ID
/**
 * @brief 修改电机ID地址
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param id 默认电机ID为1,可修改为1-255,0为广播地址
 */
void ZDT_V5_Modify_Motor_ID(uint8_t addr, bool svF, uint8_t id) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_MOTOR_ID; cmd[2] = AUX_CODE_SET_MOTOR_ID; cmd[3] = svF; cmd[4] = id; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_MICRO_STEP
/**
 * @brief 修改电机细分值
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param mstep 细分值，如：1、2、4、8、16、32、64、128、256
 */
void ZDT_V5_Modify_MicroStep(uint8_t addr, bool svF, uint8_t mstep) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_MICRO_STEP; cmd[2] = AUX_CODE_SET_MICRO_STEP; cmd[3] = svF; cmd[4] = mstep; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_POWER_FLAG
/**
 * @brief 修改断电标志位
 * @param addr 电机地址
 * @param pdf 标志位,上电默认为true
 */
void ZDT_V5_Modify_PDFlag(uint8_t addr, bool pdf) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_POWER_FLAG; cmd[3] = pdf; cmd[4] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 5);
}
#endif

#if MOTOR_SET_MOTOR_TYPE
/**
 * @brief 修改电机类型
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param mottype 电机类型,false为1.8°,true为0.9°
 */
void ZDT_V5_Modify_Motor_Type(uint8_t addr, bool svF, bool mottype) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_MOTOR_TYPE; cmd[2] = AUX_CODE_SET_MOTOR_TYPE; cmd[3] = svF; cmd[4] = mottype; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_FIRMWARE_TYPE
/**
 * @brief 修改固件类型
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param fwtype 固件类型,false为X固件,true为Emm固件
 */
void ZDT_V5_Modify_Firmware_Type(uint8_t addr, bool svF, bool fwtype) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_FIRMWARE_TYPE; cmd[2] = AUX_CODE_SET_FIRMWARE_TYPE; cmd[3] = svF; cmd[4] = fwtype; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_OPENLOOP_CURRENT
/**
 * @brief 修改开环工作电流
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param om_ma 开环工作电流(mA) 0-5000
 */
void ZDT_V5_Modify_OM_mA(uint8_t addr, bool svF, uint16_t om_ma) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_OPENLOOP_CURRENT; cmd[2] = AUX_CODE_SET_OPENLOOP_CURRENT; cmd[3] = svF;
	cmd[4] = (uint8_t)(om_ma >> 8); cmd[5] = (uint8_t)(om_ma >> 0);
	cmd[6] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 7);
}
#endif

#if MOTOR_SET_CLOSEDLOOP_CURRENT
/**
 * @brief 修改闭环最大电流
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param foc_mA 闭环最大电流(mA) 0-5000
 */
void ZDT_V5_Modify_FOC_mA(uint8_t addr, bool svF, uint16_t foc_mA) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_CLOSEDLOOP_CURRENT; cmd[2] = AUX_CODE_SET_CLOSEDLOOP_CURRENT; cmd[3] = svF;
	cmd[4] = (uint8_t)(foc_mA >> 8); cmd[5] = (uint8_t)(foc_mA >> 0);
	cmd[6] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 7);
}
#endif

#if MOTOR_SET_CONTROL_MODE
/**
 * @brief 修改控制模式
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param ctrl_mode 控制模式,false为开环,true为闭环FOC
 */
void ZDT_V5_Modify_Ctrl_Mode(uint8_t addr, bool svF, bool ctrl_mode) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_CONTROL_MODE; cmd[2] = AUX_CODE_SET_CONTROL_MODE; cmd[3] = svF; cmd[4] = ctrl_mode; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_DIRECTION
/**
 * @brief 修改电机正方向
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param dir 电机正方向,0为CW，其他为CCW
 */
void ZDT_V5_Modify_Motor_Dir(uint8_t addr, bool svF, bool dir) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_DIRECTION; cmd[2] = AUX_CODE_SET_DIRECTION; cmd[3] = svF; cmd[4] = dir; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_LOCK_KEY
/**
 * @brief 修改锁定按钮功能
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param lockbtn 锁定按钮状态
 */
void ZDT_V5_Modify_Lock_Btn(uint8_t addr, bool svF, bool lockbtn) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_LOCK_KEY; cmd[2] = AUX_CODE_SET_LOCK_KEY; cmd[3] = svF; cmd[4] = lockbtn; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_SCALE_INPUT
/**
 * @brief 修改缩小倍数输入
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param s_vel 缩小倍数使能
 */
void ZDT_V5_Modify_S_Vel(uint8_t addr, bool svF, bool s_vel) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_SCALE_INPUT; cmd[2] = AUX_CODE_SET_SCALE_INPUT; cmd[3] = svF; cmd[4] = s_vel; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_SET_DRIVER_CONFIG_BATCH
/**
 * @brief 批量修改驱动配置参数
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param params 参数数据指针
 */
void ZDT_V5_Modify_Batch_Config(uint8_t addr, bool svF, uint8_t *params) {
	uint8_t cmd[64] = {0};
	uint8_t i = 0;
	cmd[i] = addr; ++i;
	cmd[i] = CMD_SET_DRIVER_CONFIG; ++i;
	cmd[i] = AUX_CODE_SET_DRIVER_CONFIG; ++i;
	cmd[i] = svF; ++i;
	for(uint8_t n = 0; n < 28; n++) {
		cmd[i] = params[n]; ++i;
	}
	cmd[i] = END_CODE; ++i;
	ZDT_V5_SEND_CMD(cmd, i);
}
#endif

#if MOTOR_SET_LOCK_PARAMS
/**
 * @brief 修改锁定参数（X42S/Y42）
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param lock_level 锁定等级 0-3
 */
void ZDT_V5_Modify_Lock_Params(uint8_t addr, bool svF, uint8_t lock_level) {
	uint8_t cmd[16] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_LOCK_PARAMS; cmd[2] = AUX_CODE_SET_LOCK_PARAMS; cmd[3] = svF; cmd[4] = lock_level; cmd[5] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 6);
}
#endif

#if MOTOR_DMX512_WRITE
/**
 * @brief 修改DMX512协议参数（X42S/Y42）
 * @param addr 电机地址
 * @param svF 是否存储标志,false为不存储,true为存储
 * @param tch 目标通道
 * @param nch 通道数量
 * @param mode 控制模式 0-位置, 1-速度, 2-力矩
 * @param vel 速度(RPM)
 * @param acc 加速度
 * @param vel_step 速度步进
 * @param pos_step 位置步进
 */
void ZDT_V5_Modify_DMX512_Params(uint8_t addr, bool svF, uint16_t tch, uint8_t nch, uint8_t mode, uint16_t vel, uint16_t acc, uint16_t vel_step, uint32_t pos_step) {
	uint8_t cmd[32] = {0};
	cmd[0] = addr; cmd[1] = CMD_SET_DMX512_PARAMS; cmd[2] = svF;
	cmd[3] = (uint8_t)(tch >> 8); cmd[4] = (uint8_t)(tch >> 0);
	cmd[5] = nch; cmd[6] = mode;
	cmd[7] = (uint8_t)(vel >> 8); cmd[8] = (uint8_t)(vel >> 0);
	cmd[9] = (uint8_t)(acc >> 8); cmd[10] = (uint8_t)(acc >> 0);
	cmd[11] = (uint8_t)(vel_step >> 8); cmd[12] = (uint8_t)(vel_step >> 0);
	cmd[13] = (uint8_t)(pos_step >> 24); cmd[14] = (uint8_t)(pos_step >> 16);
	cmd[15] = (uint8_t)(pos_step >> 8); cmd[16] = (uint8_t)(pos_step >> 0);
	cmd[17] = END_CODE;
	ZDT_V5_SEND_CMD(cmd, 18);
}
#endif
