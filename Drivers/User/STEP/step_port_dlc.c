/**
 * @file step_port_dlc.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机端口拓展源文件
 */

#include "ZDT_V5_Driver.h"
#include "step_port.h"
#include "log.h"
#include "cmsis_os2.h"
#include <stdio.h>

/**
 * @brief 电机更新辅助函数
 * @param cmd 电机命令指针
 * @param motor_id 电机ID
 */
void Motor_Update(MotorCmd_t *cmd, uint8_t motor_id) {
	bool keep_alive = false;
    cmd->motor_id = motor_id;
    cmd->op_type = OP_PARAM_READ;

/******************** 系统状态参数 *********************/
{
    cmd->type.read.type = MP_SYS;
#if MOTOR_STATUS_READ_BATCH
    cmd->type.read.p.sys = S_BATCH;
	Motor_Send_Cmd(cmd);
	keep_alive = true;
#else
#if MOTOR_STATUS_BUS_VOLTAGE
    cmd->type.read.p.sys = S_VBUS;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_PHASE_CURRENT
    cmd->type.read.p.sys = S_CPHA;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_ENCODER_VALUE
    cmd->type.read.p.sys = S_ENCL;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_TARGET_POS
    cmd->type.read.p.sys = S_TPOS;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_SPEED
    cmd->type.read.p.sys = S_VEL;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_REAL_POS
    cmd->type.read.p.sys = S_CPOS;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_POS_ERROR
    cmd->type.read.p.sys = S_PERR;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_MOTOR_FLAGS
    cmd->type.read.p.sys = S_FLAG;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_HOME_FLAGS
    cmd->type.read.p.sys = S_OFLAG;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_BUS_CURRENT
    cmd->type.read.p.sys = S_CBUS;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_TEMPERATURE
    cmd->type.read.p.sys = S_TEMP;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_FLAGS_COMBINED
    cmd->type.read.p.sys = S_OAF;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#endif /* MOTOR_STATUS_READ_BATCH */
#if MOTOR_STATUS_INPUT_PULSES
    cmd->type.read.p.sys = S_CLKI;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_SET_POS
    cmd->type.read.p.sys = S_SPOS;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_PIN_STATUS
    cmd->type.read.p.sys = S_PIN;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_STATUS_BATTERY_VOLTAGE
    cmd->type.read.p.sys = S_VBAT;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
}

/******************** 驱动配置参数 *********************/
{
    cmd->type.read.type = MP_DEV;
#if MOTOR_DRIVER_READ_BATCH
    cmd->type.read.p.drv = D_BATCH;
    Motor_Send_Cmd(cmd);
	keep_alive = true;

#elif MOTOR_DRIVER_POS_WINDOW
    cmd->type.read.p.drv = D_POS_WINDOW;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_DRIVER_HOME
    cmd->type.read.p.drv = D_HOME;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
}

/******************** 电机控制参数 *********************/
{
    cmd->type.read.type = MP_CTRL;
#if MOTOR_PID_READ
    cmd->type.read.p.ctrl = C_PID;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_INTEGRAL_LIMIT_READ
    cmd->type.read.p.ctrl = C_INTEGRAL_LIMIT;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_PROTECT_THRESHOLD_READ
    cmd->type.read.p.ctrl = C_PROTECT_THRESHOLD;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_COLLISION_ANGLE_READ
    cmd->type.read.p.ctrl = C_COLLISION_ANGLE;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_HEARTBEAT_READ
    cmd->type.read.p.ctrl = C_HEARTBEAT;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
}

// /******************** 设备信息与特殊功能 *********************/
{
    cmd->type.read.type = MP_INFO;
#if MOTOR_DRIVER_DMX512
    cmd->type.read.p.info = I_DMX512;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_READ_VERSION
    cmd->type.read.p.info = I_VERSION;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_READ_PHASE_PARAMS
    cmd->type.read.p.info = I_PHASE_PARAMS;
    Motor_Send_Cmd(cmd);
	keep_alive = true;
#endif
#if MOTOR_READ_OPTION_PARAMS
#if MOTOR_DRIVER_MOTOR_TYPE || MOTOR_DIRECTION || MOTOR_FIRMWARE_TYPE || MOTOR_LOCK_PARAMS || MOTOR_DRIVER_CONTROL_MODE || MOTOR_DRIVER_LOCK_KEY || MOTOR_DRIVER_SCALE_10X
	cmd->type.read.p.info = I_OPTION;
	Motor_Send_Cmd(cmd);
#elif USE_HEARTBEAT
	if (!keep_alive) {
		cmd->type.read.p.info = I_OPTION;
		Motor_Send_Cmd(cmd);
	}
#endif
#endif
}
}

#if USE_VIEW
#if MOTOR_STATUS_REAL_POS && MOTOR_STATUS_TARGET_POS && MOTOR_STATUS_POS_ERROR && MOTOR_STATUS_SET_POS && MOTOR_STATUS_INPUT_PULSES
/* 辅助函数：格式化整数显示 */
static const char* fmt_int(int32_t v) {
	static char buf[4][8];
	static uint8_t idx;
	idx = (idx + 1) & 3;
	if (v < 10000 && v > -10000) {
		sprintf(buf[idx], "%6d", v);
	} else if (v < 1000000 && v > -1000000) {
		sprintf(buf[idx], "%4.1fk", (float)v / 1000.0f);
	} else {
		sprintf(buf[idx], "%4.1fm", (float)v / 1000000.0f);
	}
	return buf[idx];
}
#endif

#if MOTOR_DRIVER_HOME
static const char* home_mode_str(uint8_t mode) {
	switch (mode) {
		case 0: return "Near"; case 1: return "Dir"; case 2: return "Col";
		case 3: return "Limit"; default: return "?";
	}
}
#endif

#if MOTOR_DRIVER_CONTROL_MODE
static const char* ctrl_str(bool v) { return v == 0 ? "Open" : "Close"; }
#endif

#if MOTOR_DRIVER_MOTOR_TYPE
static const char* motor_type_str(bool t) {
	return t ? "0.9" : "1.8";
}
#endif

#if MOTOR_DRIVER_HOME || MOTOR_DRIVER_HOME_DIR
static const char* dir_str(uint8_t dir) { return dir == 0 ? "CW" : "CCW"; }
#endif

#if MOTOR_DRIVER_SCALE_10X || MOTOR_DRIVER_STALL_PROTECT || MOTOR_DRIVER_AUTO_HOME || MOTOR_DRIVER_FIRMWARE_TYPE || MOTOR_DRIVER_LOCK_KEY || MOTOR_DRIVER_AUTO_SCREEN_OFF
static const char* onoff_str(bool v) { return v ? "On" : "Off"; }
#endif

#if MOTOR_DRIVER_BAUDRATE
static const char* uart_baud_str(uint8_t code) {
	switch (code) {
		case 0: return "9600"; case 1: return "19200"; case 2: return "25000"; case 3: return "38400";
		case 4: return "57600"; case 5: return "115200"; case 6: return "256000"; case 7: return "512000";
		case 8: return "921600"; default: return "?";
	}
}
#endif

#if MOTOR_DRIVER_CAN_RATE
static const char* can_baud_str(uint8_t code) {
	switch (code) {
		case 0: return "10K"; case 1: return "20K"; case 2: return "50K"; case 3: return "83K";
		case 4: return "100K"; case 5: return "125K"; case 6: return "250K"; case 7: return "500K";
		case 8: return "800K"; case 9: return "1M"; default: return "?";
	}
}
#endif

#if MOTOR_DRIVER_PULSE_PORT_MODE
static const char* pulse_port_str(uint8_t v) {
	switch (v) {
		case 0: return "OFF"; case 1: return "OPEN"; case 2: return "FOC";
		case 3: return "RCO"; case 4: return "pLR"; default: return "?";
	}
}
#endif

#if MOTOR_DRIVER_COMM_PORT_MODE
static const char* comm_port_str(uint8_t v) {
	switch (v) {
		case 0: return "OFF"; case 1: return "ALO"; case 2: return "UART";
		case 3: return "CAN"; case 4: return "uLR"; default: return "?";
	}
}
#endif

#if MOTOR_DRIVER_EN_PIN_LEVEL
static const char* en_pin_str(uint8_t v) {
	switch (v) {
		case 0: return "L"; case 1: return "H"; case 2: return "Hold"; default: return "?";
	}
}
#endif

#if MOTOR_DRIVER_DIR_PIN_LEVEL
static const char* dir_pin_str(uint8_t v) {
	switch (v) { case 0: return "CW"; case 1: return "CCW"; default: return "?"; }
}
#endif

#if MOTOR_DRIVER_COMM_CHECK_MODE
static const char* check_mode_str(uint8_t v) {
	switch (v) {
		case 0: return "6B"; case 1: return "XOR"; case 2: return "CRC8";
		case 3: return "Modbus"; case 4: return "DMX"; default: return "?";
	}
}
#endif

#if MOTOR_DRIVER_CMD_RESPONSE_MODE
static const char* rsp_mode_str(uint8_t v) {
	switch (v) {
		case 0: return "None"; case 1: return "Rcv"; case 2: return "Reach";
		case 3: return "Both"; case 4: return "Other"; default: return "?";
	}
}
#endif

/**
 * @brief 查看电机状态
 */
void Motor_View_Shell(void) {
	char ch;
	extern Shell shell;
    extern MotorStatusShared_t *g_motor_status;
	uint8_t line = 0;
	uint16_t len = 0;
	char* out_buf = NULL;
	#define BUFFER_SIZE 1024

	if (!g_motor_status) {
		logWarning("Motor module not initialized");
		return;
	}

	out_buf = pvPortMalloc(BUFFER_SIZE);
	if (!out_buf) return;

	logPrintln("Motor Status Viewer - Press ^C to exit\033[?25l");

	while (1) {
		if (line == 0) {
			len += snprintf(out_buf + len, BUFFER_SIZE - len, "\033[1;1H\033[2J");
		} else {
			len = 0;
			len += snprintf(out_buf + len, BUFFER_SIZE - len, "\033[%dA\033[2K\r", line);
			line = 0;
		}

		len += snprintf(out_buf + len, BUFFER_SIZE - len, "  ID | %3d   | %3d   | %3d   | %3d   |\r\n",
				g_motor_status->motors[0].motor_id,
				g_motor_status->motors[1].motor_id,
				g_motor_status->motors[2].motor_id,
				g_motor_status->motors[3].motor_id); line++;
#if MOTOR_STATUS_BUS_VOLTAGE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "V(mV)|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].voltage,
				g_motor_status->motors[1].voltage,
				g_motor_status->motors[2].voltage,
				g_motor_status->motors[3].voltage); line++;
#endif
#if MOTOR_STATUS_BUS_CURRENT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "BusI |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].bus_current,
				g_motor_status->motors[1].bus_current,
				g_motor_status->motors[2].bus_current,
				g_motor_status->motors[3].bus_current); line++;
#endif
#if MOTOR_STATUS_PHASE_CURRENT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "PhI  |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].phase_current,
				g_motor_status->motors[1].phase_current,
				g_motor_status->motors[2].phase_current,
				g_motor_status->motors[3].phase_current); line++;
#endif
#if MOTOR_STATUS_TEMPERATURE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Temp | %4d  | %4d  | %4d  | %4d  |\r\n",
				g_motor_status->motors[0].temp,
				g_motor_status->motors[1].temp,
				g_motor_status->motors[2].temp,
				g_motor_status->motors[3].temp); line++;
#endif
#if MOTOR_STATUS_BATTERY_VOLTAGE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "BatV |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].battery_voltage,
				g_motor_status->motors[1].battery_voltage,
				g_motor_status->motors[2].battery_voltage,
				g_motor_status->motors[3].battery_voltage); line++;
#endif
#if MOTOR_STATUS_SPEED
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Vel  |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].vel,
				g_motor_status->motors[1].vel,
				g_motor_status->motors[2].vel,
				g_motor_status->motors[3].vel); line++;
#endif
#if MOTOR_STATUS_REAL_POS
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Pos  |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].pos),
				fmt_int(g_motor_status->motors[1].pos),
				fmt_int(g_motor_status->motors[2].pos),
				fmt_int(g_motor_status->motors[3].pos)); line++;
#endif
#if MOTOR_STATUS_TARGET_POS
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "TPos |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].target_pos),
				fmt_int(g_motor_status->motors[1].target_pos),
				fmt_int(g_motor_status->motors[2].target_pos),
				fmt_int(g_motor_status->motors[3].target_pos)); line++;
#endif
#if MOTOR_STATUS_POS_ERROR
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Err  |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].pos_error),
				fmt_int(g_motor_status->motors[1].pos_error),
				fmt_int(g_motor_status->motors[2].pos_error),
				fmt_int(g_motor_status->motors[3].pos_error)); line++;
#endif
#if MOTOR_STATUS_ENCODER_VALUE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "EncL |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].encoder_linear,
				g_motor_status->motors[1].encoder_linear,
				g_motor_status->motors[2].encoder_linear,
				g_motor_status->motors[3].encoder_linear); line++;
#endif
#if MOTOR_STATUS_ENCODER_RAW
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "EncR |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].encoder_raw,
				g_motor_status->motors[1].encoder_raw,
				g_motor_status->motors[2].encoder_raw,
				g_motor_status->motors[3].encoder_raw); line++;
#endif
#if MOTOR_STATUS_MOTOR_FLAGS || MOTOR_STATUS_HOME_FLAGS
		{
			uint8_t sta[4], hom[4];
			for (int i = 0; i < 4; i++) {
#if MOTOR_STATUS_MOTOR_FLAGS
				sta[i] = (g_motor_status->motors[i].oac << 7) | (g_motor_status->motors[i].esi_r << 6) |
						 (g_motor_status->motors[i].esi_l << 4) | (g_motor_status->motors[i].cgp << 3) |
						 (g_motor_status->motors[i].cgi << 2) | (g_motor_status->motors[i].prf << 1) |
						 g_motor_status->motors[i].ens;
#else
				(void) sta[i];
#endif
#if MOTOR_STATUS_HOME_FLAGS
				hom[i] = (g_motor_status->motors[i].ocp_tf << 7) | (g_motor_status->motors[i].otp_tf << 4) |
						 (g_motor_status->motors[i].org_cf << 3) | (g_motor_status->motors[i].org_sf << 2) |
						 (g_motor_status->motors[i].cal_rdy << 1) | g_motor_status->motors[i].enc_rdy;
#else
				(void) hom[i];
#endif
			}
#if MOTOR_STATUS_MOTOR_FLAGS
			len += snprintf(out_buf + len, BUFFER_SIZE - len, "Sta  |  %04X |  %04X |  %04X |  %04X |\r\n",
					sta[0], sta[1], sta[2], sta[3]); line++;
#endif
#if MOTOR_STATUS_HOME_FLAGS
			len += snprintf(out_buf + len, BUFFER_SIZE - len, "Hom  |  %04X |  %04X |  %04X |  %04X |\r\n",
					hom[0], hom[1], hom[2], hom[3]); line++;
#endif
		}
#endif
#if MOTOR_STATUS_PIN_STATUS
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Pin  |  %04X |  %04X |  %04X |  %04X |\r\n",
				g_motor_status->motors[0].pin_status,
				g_motor_status->motors[1].pin_status,
				g_motor_status->motors[2].pin_status,
				g_motor_status->motors[3].pin_status); line++;
#endif
#if MOTOR_STATUS_SET_POS
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "SPos |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].set_pos),
				fmt_int(g_motor_status->motors[1].set_pos),
				fmt_int(g_motor_status->motors[2].set_pos),
				fmt_int(g_motor_status->motors[3].set_pos)); line++;
#endif
#if MOTOR_STATUS_INPUT_PULSES
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Puls |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].input_pulses),
				fmt_int(g_motor_status->motors[1].input_pulses),
				fmt_int(g_motor_status->motors[2].input_pulses),
				fmt_int(g_motor_status->motors[3].input_pulses)); line++;
#endif
#if MOTOR_READ_VERSION
		{
			static char fw[4][8];
			for (int i = 0; i < 4; i++) {
				uint16_t v = g_motor_status->motors[i].firmware_version;
				sprintf(fw[i], "%d.%02d", v / 100, v % 100);
			}
			len += snprintf(out_buf + len, BUFFER_SIZE - len, "FWVer|%7s|%7s|%7s|%7s|\r\n",
					fw[0], fw[1], fw[2], fw[3]); line++;
		}
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "HWVer|  %4d  |  %4d  |  %4d  |  %4d  |\r\n",
				g_motor_status->motors[0].hardware_version,
				g_motor_status->motors[1].hardware_version,
				g_motor_status->motors[2].hardware_version,
				g_motor_status->motors[3].hardware_version); line++;
		{
			static char hw_type[4][8];
			for (int i = 0; i < 4; i++) {
				uint8_t series = g_motor_status->motors[i].hardware_series;
				uint8_t type = g_motor_status->motors[i].hardware_type;
				sprintf(hw_type[i], "%s-%d", series == 0 ? "X" : "Y", type);
			}
			len += snprintf(out_buf + len, BUFFER_SIZE - len, "HWTyp|%5s|%5s|%5s|%5s|\r\n",
					hw_type[0], hw_type[1], hw_type[2], hw_type[3]); line++;
		}
#endif
#if MOTOR_LOCK_PARAMS
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "LockL|  %4d  |  %4d  |  %4d  |  %4d  |\r\n",
				g_motor_status->motors[0].lock_level,
				g_motor_status->motors[1].lock_level,
				g_motor_status->motors[2].lock_level,
				g_motor_status->motors[3].lock_level); line++;
#endif
#if MOTOR_FIRMWARE_TYPE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "FwTyp|  %-4s |  %-4s |  %-4s |  %-4s |\r\n",
				g_motor_status->motors[0].firmware_type ? "Emm" : "X",
				g_motor_status->motors[1].firmware_type ? "Emm" : "X",
				g_motor_status->motors[2].firmware_type ? "Emm" : "X",
				g_motor_status->motors[3].firmware_type ? "Emm" : "X"); line++;
#endif
#if MOTOR_DIRECTION
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "MotDr|  %-4s |  %-4s |  %-4s |  %-4s |\r\n",
				g_motor_status->motors[0].motor_dir ? "CCW" : "CW",
				g_motor_status->motors[1].motor_dir ? "CCW" : "CW",
				g_motor_status->motors[2].motor_dir ? "CCW" : "CW",
				g_motor_status->motors[3].motor_dir ? "CCW" : "CW"); line++;
#endif
#if MOTOR_PID_READ
#if CURRENT_FIRMWARE == FIRMWARE_EMM
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Kp   |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].kp,
				g_motor_status->motors[1].kp,
				g_motor_status->motors[2].kp,
				g_motor_status->motors[3].kp); line++;
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Ki   |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].ki,
				g_motor_status->motors[1].ki,
				g_motor_status->motors[2].ki,
				g_motor_status->motors[3].ki); line++;
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Kd   |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].kd,
				g_motor_status->motors[1].kd,
				g_motor_status->motors[2].kd,
				g_motor_status->motors[3].kd); line++;
#elif CURRENT_FIRMWARE == FIRMWARE_X
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "TrKp |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].trapezoidal_kp,
				g_motor_status->motors[1].trapezoidal_kp,
				g_motor_status->motors[2].trapezoidal_kp,
				g_motor_status->motors[3].trapezoidal_kp); line++;
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "DiKp |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].direct_kp,
				g_motor_status->motors[1].direct_kp,
				g_motor_status->motors[2].direct_kp,
				g_motor_status->motors[3].direct_kp); line++;
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "VelKp|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].vel_kp,
				g_motor_status->motors[1].vel_kp,
				g_motor_status->motors[2].vel_kp,
				g_motor_status->motors[3].vel_kp); line++;
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "VelKi|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].vel_ki,
				g_motor_status->motors[1].vel_ki,
				g_motor_status->motors[2].vel_ki,
				g_motor_status->motors[3].vel_ki); line++;
#endif
#endif
#if MOTOR_DRIVER_POS_WINDOW
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "PosW |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].pos_window,
				g_motor_status->motors[1].pos_window,
				g_motor_status->motors[2].pos_window,
				g_motor_status->motors[3].pos_window); line++;
#endif
#if MOTOR_INTEGRAL_LIMIT_READ
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "IntL |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].integral_limit,
				g_motor_status->motors[1].integral_limit,
				g_motor_status->motors[2].integral_limit,
				g_motor_status->motors[3].integral_limit); line++;
#endif
#if MOTOR_PROTECT_THRESHOLD_READ
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "TempT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].temp_threshold,
				g_motor_status->motors[1].temp_threshold,
				g_motor_status->motors[2].temp_threshold,
				g_motor_status->motors[3].temp_threshold); line++;
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "CurrT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].current_threshold,
				g_motor_status->motors[1].current_threshold,
				g_motor_status->motors[2].current_threshold,
				g_motor_status->motors[3].current_threshold); line++;
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ProtT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].protect_time,
				g_motor_status->motors[1].protect_time,
				g_motor_status->motors[2].protect_time,
				g_motor_status->motors[3].protect_time); line++;
#endif
#if MOTOR_HEARTBEAT_READ
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "HearT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].heartbeat_time,
				g_motor_status->motors[1].heartbeat_time,
				g_motor_status->motors[2].heartbeat_time,
				g_motor_status->motors[3].heartbeat_time); line++;
#endif
#if MOTOR_COLLISION_ANGLE_READ
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ColA |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].collision_angle,
				g_motor_status->motors[1].collision_angle,
				g_motor_status->motors[2].collision_angle,
				g_motor_status->motors[3].collision_angle); line++;
#endif
#if MOTOR_DRIVER_STALL_PROTECT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ClogE| %-5s | %-5s | %-5s | %-5s |\r\n",
				onoff_str(g_motor_status->motors[0].clog_enable),
				onoff_str(g_motor_status->motors[1].clog_enable),
				onoff_str(g_motor_status->motors[2].clog_enable),
				onoff_str(g_motor_status->motors[3].clog_enable)); line++;
#endif
#if MOTOR_DRIVER_STALL_SPEED
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ClogR|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].clog_rpm,
				g_motor_status->motors[1].clog_rpm,
				g_motor_status->motors[2].clog_rpm,
				g_motor_status->motors[3].clog_rpm); line++;
#endif
#if MOTOR_DRIVER_STALL_CURRENT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ClogC|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].clog_current,
				g_motor_status->motors[1].clog_current,
				g_motor_status->motors[2].clog_current,
				g_motor_status->motors[3].clog_current); line++;
#endif
#if MOTOR_DRIVER_STALL_TIME
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ClogT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].clog_time,
				g_motor_status->motors[1].clog_time,
				g_motor_status->motors[2].clog_time,
				g_motor_status->motors[3].clog_time); line++;
#endif
#if MOTOR_DRIVER_HOME
#if MOTOR_DRIVER_HOME_MODE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "HomeM| %-4s  | %-4s  | %-4s  | %-4s  |\r\n",
				home_mode_str(g_motor_status->motors[0].home_mode),
				home_mode_str(g_motor_status->motors[1].home_mode),
				home_mode_str(g_motor_status->motors[2].home_mode),
				home_mode_str(g_motor_status->motors[3].home_mode)); line++;
#endif
#if MOTOR_DRIVER_HOME_DIR
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "HomeD| %-4s | %-4s | %-4s | %-4s |\r\n",
				dir_str(g_motor_status->motors[0].home_dir),
				dir_str(g_motor_status->motors[1].home_dir),
				dir_str(g_motor_status->motors[2].home_dir),
				dir_str(g_motor_status->motors[3].home_dir)); line++;
#endif
#if MOTOR_DRIVER_HOME_SPEED
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "HomeS|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].home_speed,
				g_motor_status->motors[1].home_speed,
				g_motor_status->motors[2].home_speed,
				g_motor_status->motors[3].home_speed); line++;
#endif
#if MOTOR_DRIVER_HOME_TIMEOUT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "HomeT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].home_timeout,
				g_motor_status->motors[1].home_timeout,
				g_motor_status->motors[2].home_timeout,
				g_motor_status->motors[3].home_timeout); line++;
#endif
#if MOTOR_DRIVER_AUTO_HOME
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "HmAEn| %-4s  | %-4s  | %-4s  | %-4s  |\r\n",
				onoff_str(g_motor_status->motors[0].auto_home),
				onoff_str(g_motor_status->motors[1].auto_home),
				onoff_str(g_motor_status->motors[2].auto_home),
				onoff_str(g_motor_status->motors[3].auto_home)); line++;
#endif
#if MOTOR_DRIVER_HOME_COLLISION_SPEED
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "SlVel|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].sl_vel,
				g_motor_status->motors[1].sl_vel,
				g_motor_status->motors[2].sl_vel,
				g_motor_status->motors[3].sl_vel); line++;
#endif
#if MOTOR_DRIVER_HOME_COLLISION_CURRENT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "SlCur|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].sl_current,
				g_motor_status->motors[1].sl_current,
				g_motor_status->motors[2].sl_current,
				g_motor_status->motors[3].sl_current); line++;
#endif
#if MOTOR_DRIVER_HOME_COLLISION_TIME
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "SlTim|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].sl_time,
				g_motor_status->motors[1].sl_time,
				g_motor_status->motors[2].sl_time,
				g_motor_status->motors[3].sl_time); line++;
#endif
#endif /* MOTOR_DRIVER_HOME */

#if MOTOR_DRIVER_CONTROL_MODE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "CtrlM| %-4s  | %-4s  | %-4s  | %-4s  |\r\n",
				ctrl_str(g_motor_status->motors[0].control_mode),
				ctrl_str(g_motor_status->motors[1].control_mode),
				ctrl_str(g_motor_status->motors[2].control_mode),
				ctrl_str(g_motor_status->motors[3].control_mode));
		line++;
#endif

#if MOTOR_DRIVER_MOTOR_TYPE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "MotTp| %-4s | %-4s | %-4s | %-4s |\r\n",
				motor_type_str(g_motor_status->motors[0].motor_type),
				motor_type_str(g_motor_status->motors[1].motor_type),
				motor_type_str(g_motor_status->motors[2].motor_type),
				motor_type_str(g_motor_status->motors[3].motor_type));
		line++;
#endif

#if MOTOR_DRIVER_MICRO_STEP
		{
			static char ms[4][6];
			for (int i = 0; i < 4; i++) {
				uint8_t v = g_motor_status->motors[i].micro_step;
				sprintf(ms[i], v == 0 ? "256" : "%d", v);
			}
			len += snprintf(out_buf + len, BUFFER_SIZE - len, "Micro| %-4s | %-4s | %-4s | %-4s |\r\n",
					ms[0], ms[1], ms[2], ms[3]);
			line++;
		}
#endif

#if MOTOR_DRIVER_FIRMWARE_TYPE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Inter| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].interpolation),
				onoff_str(g_motor_status->motors[1].interpolation),
				onoff_str(g_motor_status->motors[2].interpolation),
				onoff_str(g_motor_status->motors[3].interpolation));
		line++;
#endif

#if MOTOR_DRIVER_OPENLOOP_CURRENT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "OpenI|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].open_current,
				g_motor_status->motors[1].open_current,
				g_motor_status->motors[2].open_current,
				g_motor_status->motors[3].open_current);
		line++;
#endif

#if MOTOR_DRIVER_CLOSEDLOOP_CURRENT
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ClosI|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].close_current,
				g_motor_status->motors[1].close_current,
				g_motor_status->motors[2].close_current,
				g_motor_status->motors[3].close_current);
		line++;
#endif

#if MOTOR_DRIVER_BAUDRATE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Uart | %-6s | %-6s | %-6s | %-6s |\r\n",
				uart_baud_str(g_motor_status->motors[0].uart_baudrate),
				uart_baud_str(g_motor_status->motors[1].uart_baudrate),
				uart_baud_str(g_motor_status->motors[2].uart_baudrate),
				uart_baud_str(g_motor_status->motors[3].uart_baudrate));
		line++;
#endif

#if MOTOR_DRIVER_CAN_RATE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Can  | %-6s | %-6s | %-6s | %-6s |\r\n",
				can_baud_str(g_motor_status->motors[0].can_baudrate),
				can_baud_str(g_motor_status->motors[1].can_baudrate),
				can_baud_str(g_motor_status->motors[2].can_baudrate),
				can_baud_str(g_motor_status->motors[3].can_baudrate));
		line++;
#endif

#if MOTOR_DRIVER_LOCK_KEY
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "LockK| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].lock_key),
				onoff_str(g_motor_status->motors[1].lock_key),
				onoff_str(g_motor_status->motors[2].lock_key),
				onoff_str(g_motor_status->motors[3].lock_key));
		line++;
#endif

#if MOTOR_DRIVER_PULSE_PORT_MODE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "PulsP| %-5s| %-5s| %-5s| %-5s|\r\n",
				pulse_port_str(g_motor_status->motors[0].pulse_port_mode),
				pulse_port_str(g_motor_status->motors[1].pulse_port_mode),
				pulse_port_str(g_motor_status->motors[2].pulse_port_mode),
				pulse_port_str(g_motor_status->motors[3].pulse_port_mode));
		line++;
#endif

#if MOTOR_DRIVER_COMM_PORT_MODE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "CommP| %-5s| %-5s| %-5s| %-5s|\r\n",
				comm_port_str(g_motor_status->motors[0].comm_port_mode),
				comm_port_str(g_motor_status->motors[1].comm_port_mode),
				comm_port_str(g_motor_status->motors[2].comm_port_mode),
				comm_port_str(g_motor_status->motors[3].comm_port_mode));
		line++;
#endif

#if MOTOR_DRIVER_EN_PIN_LEVEL
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "EnPin| %-5s| %-5s| %-5s| %-5s|\r\n",
				en_pin_str(g_motor_status->motors[0].en_pin_level),
				en_pin_str(g_motor_status->motors[1].en_pin_level),
				en_pin_str(g_motor_status->motors[2].en_pin_level),
				en_pin_str(g_motor_status->motors[3].en_pin_level));
		line++;
#endif

#if MOTOR_DRIVER_DIR_PIN_LEVEL
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "DirPi| %-5s| %-5s| %-5s| %-5s|\r\n",
				dir_pin_str(g_motor_status->motors[0].dir_pin_level),
				dir_pin_str(g_motor_status->motors[1].dir_pin_level),
				dir_pin_str(g_motor_status->motors[2].dir_pin_level),
				dir_pin_str(g_motor_status->motors[3].dir_pin_level));
		line++;
#endif

#if MOTOR_DRIVER_AUTO_SCREEN_OFF
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ScrOf| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].auto_screen_off),
				onoff_str(g_motor_status->motors[1].auto_screen_off),
				onoff_str(g_motor_status->motors[2].auto_screen_off),
				onoff_str(g_motor_status->motors[3].auto_screen_off));
		line++;
#endif

#if MOTOR_DRIVER_COMM_CHECK_MODE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ChkMd| %-5s| %-5s| %-5s| %-5s|\r\n",
				check_mode_str(g_motor_status->motors[0].comm_check_mode),
				check_mode_str(g_motor_status->motors[1].comm_check_mode),
				check_mode_str(g_motor_status->motors[2].comm_check_mode),
				check_mode_str(g_motor_status->motors[3].comm_check_mode));
		line++;
#endif

#if MOTOR_DRIVER_CMD_RESPONSE_MODE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "RspMd| %-5s| %-5s| %-5s| %-5s|\r\n",
				rsp_mode_str(g_motor_status->motors[0].cmd_response_mode),
				rsp_mode_str(g_motor_status->motors[1].cmd_response_mode),
				rsp_mode_str(g_motor_status->motors[2].cmd_response_mode),
				rsp_mode_str(g_motor_status->motors[3].cmd_response_mode));
		line++;
#endif

#if MOTOR_DRIVER_CLOSEDLOOP_MAX_SPEED
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "ClSpd|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].close_max_speed,
				g_motor_status->motors[1].close_max_speed,
				g_motor_status->motors[2].close_max_speed,
				g_motor_status->motors[3].close_max_speed);
		line++;
#endif

#if MOTOR_DRIVER_CURRENT_LOOP_BANDWIDTH
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "CurBW|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].current_loop_bw,
				g_motor_status->motors[1].current_loop_bw,
				g_motor_status->motors[2].current_loop_bw,
				g_motor_status->motors[3].current_loop_bw);
		line++;
#endif

#if MOTOR_DRIVER_SCALE_10X
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "Spd10| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].scale_10x),
				onoff_str(g_motor_status->motors[1].scale_10x),
				onoff_str(g_motor_status->motors[2].scale_10x),
				onoff_str(g_motor_status->motors[3].scale_10x));
		line++;
#endif

#if MOTOR_DRIVER_CLOSEDLOOP_MAX_VOLTAGE
		len += snprintf(out_buf + len, BUFFER_SIZE - len, "MaxVo|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].close_max_voltage,
				g_motor_status->motors[1].close_max_voltage,
				g_motor_status->motors[2].close_max_voltage,
				g_motor_status->motors[3].close_max_voltage);
		line++;
#endif

		shell.write(out_buf, len);

		osDelay(g_motor_status->update_time);
		if (shell.read(&ch, 1) == 1) {
			if (ch == 0x03) break;
		}
	}

	vPortFree(out_buf);
	logPrintln("\033[?25h\033[2J\033[1;1H");
}
#endif /* USE_VIEW */
