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
    cmd->motor_id = motor_id;
    cmd->op_type = OP_PARAM_READ;

/******************** 系统状态参数 *********************/
{
    cmd->type.read.type = MP_SYS;
#if MOTOR_STATUS_READ_BATCH
    cmd->type.read.p.sys = S_BATCH;
#else
#if MOTOR_STATUS_BUS_VOLTAGE
    cmd->type.read.p.sys = S_VBUS;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_PHASE_CURRENT
    cmd->type.read.p.sys = S_CPHA;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_ENCODER_VALUE
    cmd->type.read.p.sys = S_ENCL;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_TARGET_POS
    cmd->type.read.p.sys = S_TPOS;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_SPEED
    cmd->type.read.p.sys = S_VEL;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_REAL_POS
    cmd->type.read.p.sys = S_CPOS;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_POS_ERROR
    cmd->type.read.p.sys = S_PERR;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_MOTOR_FLAGS
    cmd->type.read.p.sys = S_FLAG;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_HOME_FLAGS
    cmd->type.read.p.sys = S_OFLAG;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_BUS_CURRENT
    cmd->type.read.p.sys = S_CBUS;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_TEMPERATURE
    cmd->type.read.p.sys = S_TEMP;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_FLAGS_COMBINED
    cmd->type.read.p.sys = S_OAF;
    Motor_Send_Cmd(cmd);
#endif
#endif /* MOTOR_STATUS_READ_BATCH */
#if MOTOR_STATUS_INPUT_PULSES
    cmd->type.read.p.sys = S_CLKI;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_SET_POS
    cmd->type.read.p.sys = S_SPOS;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_PIN_STATUS
    cmd->type.read.p.sys = S_PIN;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_STATUS_BATTERY_VOLTAGE
    cmd->type.read.p.sys = S_VBAT;
    Motor_Send_Cmd(cmd);
#endif
}

/******************** 驱动配置参数 *********************/
{
    cmd->type.read.type = MP_DEV;
#if MOTOR_DRIVER_CONFIG_READ_BATCH
    cmd->type.read.p.drv = D_BATCH;
    Motor_Send_Cmd(cmd);
#elif MOTOR_DRIVER_POS_WINDOW
    cmd->type.read.p.drv = D_POS_WINDOW;
    Motor_Send_Cmd(cmd);
#endif
}

/******************** 电机控制参数 *********************/
{
    cmd->type.read.type = MP_CTRL;
#if MOTOR_PID_READ
    cmd->type.read.p.ctrl = C_PID;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_HOME_READ
    cmd->type.read.p.ctrl = C_HOME;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_DRIVER_INTEGRAL_LIMIT
    cmd->type.read.p.ctrl = C_INTEGRAL_LIMIT;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_PROTECT_THRESHOLD_READ
    cmd->type.read.p.ctrl = C_PROTECT_THRESHOLD;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_COLLISION_ANGLE_READ
    cmd->type.read.p.ctrl = C_COLLISION_ANGLE;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_HEARTBEAT_READ
    cmd->type.read.p.ctrl = C_HEARTBEAT;
    Motor_Send_Cmd(cmd);
#endif
}

/******************** 设备信息与特殊功能 *********************/
{
    cmd->type.read.type = MP_INFO;
#if MOTOR_DRIVER_DMX512
    cmd->type.read.p.info = I_DMX512;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_READ_VERSION
    cmd->type.read.p.info = I_VERSION;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_READ_PHASE_PARAMS
    cmd->type.read.p.info = I_PHASE_PARAMS;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_READ_OPTION_PARAMS
    cmd->type.read.p.info = I_OPTION;
    Motor_Send_Cmd(cmd);
#endif
#if MOTOR_BROADCAST_READ_ID
    cmd->type.read.p.info = I_ID;
    Motor_Send_Cmd(cmd);
#endif
}
}

#if USE_VIEW
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

#if MOTOR_HOME_READ
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
static const char* motor_type_str(uint8_t t) {
	if (t == 25) return "1.8"; if (t == 50) return "0.9";
#if CURRENT_FIRMWARE == FIRMWARE_X
	if (t == 0) {
		uint8_t opt = g_motor_status->motors[0].option_params;
		return (opt & 0x01) ? "0.9" : "1.8";
	}
#endif
	return "?";
}
#endif

#if MOTOR_HOME_READ || MOTOR_DRIVER_HOME_DIR
static const char* dir_str(uint8_t dir) { return dir == 0 ? "CW" : "CCW"; }
#endif

#if MOTOR_DRIVER_STALL_PROTECT
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

	if (!g_motor_status) {
		logWarning("Motor module not initialized");
		return;
	}

	out_buf = pvPortMalloc(4096);
	if (!out_buf) return;

	logPrintln("Motor Status Viewer - Press ^C to exit");

	while (1) {
		if (line == 0) {
			len += snprintf(out_buf + len, 4096 - len, "\033[1;1H\033[2J");
		} else {
			len += snprintf(out_buf + len, 4096 - len, "\033[%dA\033[2K\r", line);
			line = 0;
		}

		len += snprintf(out_buf + len, 4096 - len, "  ID | %3d   | %3d   | %3d   | %3d   |\r\n",
				g_motor_status->motors[0].motor_id,
				g_motor_status->motors[1].motor_id,
				g_motor_status->motors[2].motor_id,
				g_motor_status->motors[3].motor_id);
		line++;

#if MOTOR_STATUS_BUS_VOLTAGE
		len += snprintf(out_buf + len, 4096 - len, "V(mV)|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].voltage,
				g_motor_status->motors[1].voltage,
				g_motor_status->motors[2].voltage,
				g_motor_status->motors[3].voltage);
		line++;
#endif

#if MOTOR_STATUS_BUS_CURRENT
		len += snprintf(out_buf + len, 4096 - len, "BusI |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].bus_current,
				g_motor_status->motors[1].bus_current,
				g_motor_status->motors[2].bus_current,
				g_motor_status->motors[3].bus_current);
		line++;
#endif

#if MOTOR_STATUS_PHASE_CURRENT
		len += snprintf(out_buf + len, 4096 - len, "PhI  |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].phase_current,
				g_motor_status->motors[1].phase_current,
				g_motor_status->motors[2].phase_current,
				g_motor_status->motors[3].phase_current);
		line++;
#endif

#if MOTOR_STATUS_TEMPERATURE
		len += snprintf(out_buf + len, 4096 - len, "Temp | %4d  | %4d  | %4d  | %4d  |\r\n",
				g_motor_status->motors[0].temp,
				g_motor_status->motors[1].temp,
				g_motor_status->motors[2].temp,
				g_motor_status->motors[3].temp);
		line++;
#endif

#if MOTOR_STATUS_BATTERY_VOLTAGE
		len += snprintf(out_buf + len, 4096 - len, "BatV |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].battery_voltage,
				g_motor_status->motors[1].battery_voltage,
				g_motor_status->motors[2].battery_voltage,
				g_motor_status->motors[3].battery_voltage);
		line++;
#endif

#if MOTOR_STATUS_SPEED
		len += snprintf(out_buf + len, 4096 - len, "Vel  |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].vel,
				g_motor_status->motors[1].vel,
				g_motor_status->motors[2].vel,
				g_motor_status->motors[3].vel);
		line++;
#endif

#if MOTOR_STATUS_REAL_POS
		len += snprintf(out_buf + len, 4096 - len, "Pos  |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].pos),
				fmt_int(g_motor_status->motors[1].pos),
				fmt_int(g_motor_status->motors[2].pos),
				fmt_int(g_motor_status->motors[3].pos));
		line++;
#endif

#if MOTOR_STATUS_TARGET_POS
		len += snprintf(out_buf + len, 4096 - len, "TPos |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].target_pos),
				fmt_int(g_motor_status->motors[1].target_pos),
				fmt_int(g_motor_status->motors[2].target_pos),
				fmt_int(g_motor_status->motors[3].target_pos));
		line++;
#endif

#if MOTOR_STATUS_POS_ERROR
		len += snprintf(out_buf + len, 4096 - len, "Err  |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].pos_error),
				fmt_int(g_motor_status->motors[1].pos_error),
				fmt_int(g_motor_status->motors[2].pos_error),
				fmt_int(g_motor_status->motors[3].pos_error));
		line++;
#endif

#if MOTOR_STATUS_ENCODER_VALUE
		len += snprintf(out_buf + len, 4096 - len, "EncL |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].encoder_linear,
				g_motor_status->motors[1].encoder_linear,
				g_motor_status->motors[2].encoder_linear,
				g_motor_status->motors[3].encoder_linear);
		line++;
#endif

#if MOTOR_STATUS_ENCODER_RAW
		len += snprintf(out_buf + len, 4096 - len, "EncR |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].encoder_raw,
				g_motor_status->motors[1].encoder_raw,
				g_motor_status->motors[2].encoder_raw,
				g_motor_status->motors[3].encoder_raw);
		line++;
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
				sta[i] = 0;
#endif
#if MOTOR_STATUS_HOME_FLAGS
				hom[i] = (g_motor_status->motors[i].ocp_tf << 7) | (g_motor_status->motors[i].otp_tf << 4) |
						 (g_motor_status->motors[i].org_cf << 3) | (g_motor_status->motors[i].org_sf << 2) |
						 (g_motor_status->motors[i].cal_rdy << 1) | g_motor_status->motors[i].enc_rdy;
#else
				hom[i] = 0;
#endif
			}
#if MOTOR_STATUS_MOTOR_FLAGS
			len += snprintf(out_buf + len, 4096 - len, "Sta  |  %04X |  %04X |  %04X |  %04X |\r\n",
					sta[0], sta[1], sta[2], sta[3]);
			line++;
#endif
#if MOTOR_STATUS_HOME_FLAGS
			len += snprintf(out_buf + len, 4096 - len, "Hom  |  %04X |  %04X |  %04X |  %04X |\r\n",
					hom[0], hom[1], hom[2], hom[3]);
			line++;
#endif
		}
#endif

#if MOTOR_STATUS_PIN_STATUS
		len += snprintf(out_buf + len, 4096 - len, "Pin  |  %04X |  %04X |  %04X |  %04X |\r\n",
				g_motor_status->motors[0].pin_status,
				g_motor_status->motors[1].pin_status,
				g_motor_status->motors[2].pin_status,
				g_motor_status->motors[3].pin_status);
		line++;
#endif

#if MOTOR_STATUS_SET_POS
		len += snprintf(out_buf + len, 4096 - len, "SPos |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].set_pos),
				fmt_int(g_motor_status->motors[1].set_pos),
				fmt_int(g_motor_status->motors[2].set_pos),
				fmt_int(g_motor_status->motors[3].set_pos));
		line++;
#endif

#if MOTOR_STATUS_INPUT_PULSES
		len += snprintf(out_buf + len, 4096 - len, "Puls |%7s|%7s|%7s|%7s|\r\n",
				fmt_int(g_motor_status->motors[0].input_pulses),
				fmt_int(g_motor_status->motors[1].input_pulses),
				fmt_int(g_motor_status->motors[2].input_pulses),
				fmt_int(g_motor_status->motors[3].input_pulses));
		line++;
#endif

#if MOTOR_READ_VERSION
		{
			static char fw[4][8];
			for (int i = 0; i < 4; i++) {
				uint16_t v = g_motor_status->motors[i].firmware_version;
				sprintf(fw[i], "%d.%02d", v / 100, v % 100);
			}
			len += snprintf(out_buf + len, 4096 - len, "FWVer|%7s|%7s|%7s|%7s|\r\n",
					fw[0], fw[1], fw[2], fw[3]);
			line++;
		}
		len += snprintf(out_buf + len, 4096 - len, "HWVer|  %4d  |  %4d  |  %4d  |  %4d  |\r\n",
				g_motor_status->motors[0].hardware_version,
				g_motor_status->motors[1].hardware_version,
				g_motor_status->motors[2].hardware_version,
				g_motor_status->motors[3].hardware_version);
		line++;
#endif

#if MOTOR_READ_PHASE_PARAMS
		len += snprintf(out_buf + len, 4096 - len, "Res  |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].phase_resistance,
				g_motor_status->motors[1].phase_resistance,
				g_motor_status->motors[2].phase_resistance,
				g_motor_status->motors[3].phase_resistance);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "Ind  |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].phase_inductance,
				g_motor_status->motors[1].phase_inductance,
				g_motor_status->motors[2].phase_inductance,
				g_motor_status->motors[3].phase_inductance);
		line++;
#endif

#if MOTOR_READ_OPTION_PARAMS
		len += snprintf(out_buf + len, 4096 - len, "Opt  |  %04X |  %04X |  %04X |  %04X |\r\n",
				g_motor_status->motors[0].option_params,
				g_motor_status->motors[1].option_params,
				g_motor_status->motors[2].option_params,
				g_motor_status->motors[3].option_params);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "LockL|  %4d  |  %4d  |  %4d  |  %4d  |\r\n",
				g_motor_status->motors[0].lock_level,
				g_motor_status->motors[1].lock_level,
				g_motor_status->motors[2].lock_level,
				g_motor_status->motors[3].lock_level);
		line++;
#endif

#if MOTOR_PID_READ
#if CURRENT_FIRMWARE == FIRMWARE_EMM
		len += snprintf(out_buf + len, 4096 - len, "Kp   |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].kp,
				g_motor_status->motors[1].kp,
				g_motor_status->motors[2].kp,
				g_motor_status->motors[3].kp);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "Ki   |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].ki,
				g_motor_status->motors[1].ki,
				g_motor_status->motors[2].ki,
				g_motor_status->motors[3].ki);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "Kd   |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].kd,
				g_motor_status->motors[1].kd,
				g_motor_status->motors[2].kd,
				g_motor_status->motors[3].kd);
		line++;
#elif CURRENT_FIRMWARE == FIRMWARE_X
		len += snprintf(out_buf + len, 4096 - len, "TrKp |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].trapezoidal_kp,
				g_motor_status->motors[1].trapezoidal_kp,
				g_motor_status->motors[2].trapezoidal_kp,
				g_motor_status->motors[3].trapezoidal_kp);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "DiKp |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].direct_kp,
				g_motor_status->motors[1].direct_kp,
				g_motor_status->motors[2].direct_kp,
				g_motor_status->motors[3].direct_kp);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "VelKp|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].vel_kp,
				g_motor_status->motors[1].vel_kp,
				g_motor_status->motors[2].vel_kp,
				g_motor_status->motors[3].vel_kp);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "VelKi|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].vel_ki,
				g_motor_status->motors[1].vel_ki,
				g_motor_status->motors[2].vel_ki,
				g_motor_status->motors[3].vel_ki);
		line++;
#endif
#endif

#if MOTOR_DRIVER_POS_WINDOW
		len += snprintf(out_buf + len, 4096 - len, "PosW |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].pos_window,
				g_motor_status->motors[1].pos_window,
				g_motor_status->motors[2].pos_window,
				g_motor_status->motors[3].pos_window);
		line++;
#endif

#if MOTOR_DRIVER_INTEGRAL_LIMIT
		len += snprintf(out_buf + len, 4096 - len, "IntL |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].integral_limit,
				g_motor_status->motors[1].integral_limit,
				g_motor_status->motors[2].integral_limit,
				g_motor_status->motors[3].integral_limit);
		line++;
#endif

#if MOTOR_PROTECT_THRESHOLD_READ
		len += snprintf(out_buf + len, 4096 - len, "TempT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].temp_threshold,
				g_motor_status->motors[1].temp_threshold,
				g_motor_status->motors[2].temp_threshold,
				g_motor_status->motors[3].temp_threshold);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "CurrT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].current_threshold,
				g_motor_status->motors[1].current_threshold,
				g_motor_status->motors[2].current_threshold,
				g_motor_status->motors[3].current_threshold);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "ProtT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].protect_time,
				g_motor_status->motors[1].protect_time,
				g_motor_status->motors[2].protect_time,
				g_motor_status->motors[3].protect_time);
		line++;
#endif

#if MOTOR_HEARTBEAT_READ
		len += snprintf(out_buf + len, 4096 - len, "HearT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].heartbeat_time,
				g_motor_status->motors[1].heartbeat_time,
				g_motor_status->motors[2].heartbeat_time,
				g_motor_status->motors[3].heartbeat_time);
		line++;
#endif

#if MOTOR_COLLISION_ANGLE_READ
		len += snprintf(out_buf + len, 4096 - len, "ColA |%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].collision_angle,
				g_motor_status->motors[1].collision_angle,
				g_motor_status->motors[2].collision_angle,
				g_motor_status->motors[3].collision_angle);
		line++;
#endif

#if MOTOR_DRIVER_STALL_PROTECT
		len += snprintf(out_buf + len, 4096 - len, "ClogE| %-5s | %-5s | %-5s | %-5s |\r\n",
				onoff_str(g_motor_status->motors[0].clog_enable),
				onoff_str(g_motor_status->motors[1].clog_enable),
				onoff_str(g_motor_status->motors[2].clog_enable),
				onoff_str(g_motor_status->motors[3].clog_enable));
		line++;
#endif

#if MOTOR_DRIVER_STALL_SPEED
		len += snprintf(out_buf + len, 4096 - len, "ClogR|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].clog_rpm,
				g_motor_status->motors[1].clog_rpm,
				g_motor_status->motors[2].clog_rpm,
				g_motor_status->motors[3].clog_rpm);
		line++;
#endif

#if MOTOR_DRIVER_STALL_CURRENT
		len += snprintf(out_buf + len, 4096 - len, "ClogC|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].clog_current,
				g_motor_status->motors[1].clog_current,
				g_motor_status->motors[2].clog_current,
				g_motor_status->motors[3].clog_current);
		line++;
#endif

#if MOTOR_DRIVER_STALL_TIME
		len += snprintf(out_buf + len, 4096 - len, "ClogT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].clog_time,
				g_motor_status->motors[1].clog_time,
				g_motor_status->motors[2].clog_time,
				g_motor_status->motors[3].clog_time);
		line++;
#endif

#if MOTOR_HOME_READ
		len += snprintf(out_buf + len, 4096 - len, "HomeM| %-4s  | %-4s  | %-4s  | %-4s  |\r\n",
				home_mode_str(g_motor_status->motors[0].home_mode),
				home_mode_str(g_motor_status->motors[1].home_mode),
				home_mode_str(g_motor_status->motors[2].home_mode),
				home_mode_str(g_motor_status->motors[3].home_mode));
		line++;
		len += snprintf(out_buf + len, 4096 - len, "HomeD| %-4s | %-4s | %-4s | %-4s |\r\n",
				dir_str(g_motor_status->motors[0].home_dir),
				dir_str(g_motor_status->motors[1].home_dir),
				dir_str(g_motor_status->motors[2].home_dir),
				dir_str(g_motor_status->motors[3].home_dir));
		line++;
		len += snprintf(out_buf + len, 4096 - len, "HomeS|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].home_speed,
				g_motor_status->motors[1].home_speed,
				g_motor_status->motors[2].home_speed,
				g_motor_status->motors[3].home_speed);
		line++;
		len += snprintf(out_buf + len, 4096 - len, "HomeT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].home_timeout,
				g_motor_status->motors[1].home_timeout,
				g_motor_status->motors[2].home_timeout,
				g_motor_status->motors[3].home_timeout);
		line++;
#endif

#if MOTOR_DRIVER_HOME_SPEED
		len += snprintf(out_buf + len, 4096 - len, "SlVel|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].sl_vel,
				g_motor_status->motors[1].sl_vel,
				g_motor_status->motors[2].sl_vel,
				g_motor_status->motors[3].sl_vel);
		line++;
#endif

#if MOTOR_DRIVER_HOME_MODE
		len += snprintf(out_buf + len, 4096 - len, "HmAEn| %-4s  | %-4s  | %-4s  | %-4s  |\r\n",
				onoff_str(g_motor_status->motors[0].home_auto_enable),
				onoff_str(g_motor_status->motors[1].home_auto_enable),
				onoff_str(g_motor_status->motors[2].home_auto_enable),
				onoff_str(g_motor_status->motors[3].home_auto_enable));
		line++;
#endif

#if MOTOR_DRIVER_HOME_DIR
		len += snprintf(out_buf + len, 4096 - len, "SlCur|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].sl_current,
				g_motor_status->motors[1].sl_current,
				g_motor_status->motors[2].sl_current,
				g_motor_status->motors[3].sl_current);
		line++;
#endif

#if MOTOR_DRIVER_HOME_TIMEOUT
		len += snprintf(out_buf + len, 4096 - len, "SlTim|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].sl_time,
				g_motor_status->motors[1].sl_time,
				g_motor_status->motors[2].sl_time,
				g_motor_status->motors[3].sl_time);
		line++;
#endif

#if MOTOR_DRIVER_CONTROL_MODE
		len += snprintf(out_buf + len, 4096 - len, "CtrlM| %-4s  | %-4s  | %-4s  | %-4s  |\r\n",
				ctrl_str(g_motor_status->motors[0].control_mode),
				ctrl_str(g_motor_status->motors[1].control_mode),
				ctrl_str(g_motor_status->motors[2].control_mode),
				ctrl_str(g_motor_status->motors[3].control_mode));
		line++;
#endif

#if MOTOR_DRIVER_MOTOR_TYPE
		len += snprintf(out_buf + len, 4096 - len, "MotTp| %-4s | %-4s | %-4s | %-4s |\r\n",
				motor_type_str(g_motor_status->motors[0].motor_type),
				motor_type_str(g_motor_status->motors[1].motor_type),
				motor_type_str(g_motor_status->motors[2].motor_type),
				motor_type_str(g_motor_status->motors[3].motor_type));
		line++;
#endif

#if MOTOR_DRIVER_DIRECTION
		len += snprintf(out_buf + len, 4096 - len, "MotD | %-4s | %-4s | %-4s | %-4s |\r\n",
				dir_str(g_motor_status->motors[0].motor_direction),
				dir_str(g_motor_status->motors[1].motor_direction),
				dir_str(g_motor_status->motors[2].motor_direction),
				dir_str(g_motor_status->motors[3].motor_direction));
		line++;
#endif

#if MOTOR_DRIVER_MICRO_STEP
		{
			static char ms[4][6];
			for (int i = 0; i < 4; i++) {
				uint8_t v = g_motor_status->motors[i].micro_step;
				sprintf(ms[i], v == 0 ? "256" : "%d", v);
			}
			len += snprintf(out_buf + len, 4096 - len, "Micro| %-4s | %-4s | %-4s | %-4s |\r\n",
					ms[0], ms[1], ms[2], ms[3]);
			line++;
		}
#endif

#if MOTOR_DRIVER_FIRMWARE_TYPE
		len += snprintf(out_buf + len, 4096 - len, "Inter| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].interpolation),
				onoff_str(g_motor_status->motors[1].interpolation),
				onoff_str(g_motor_status->motors[2].interpolation),
				onoff_str(g_motor_status->motors[3].interpolation));
		line++;
#endif

#if MOTOR_DRIVER_POWER_FLAG
		len += snprintf(out_buf + len, 4096 - len, "PwrFl| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].power_flag),
				onoff_str(g_motor_status->motors[1].power_flag),
				onoff_str(g_motor_status->motors[2].power_flag),
				onoff_str(g_motor_status->motors[3].power_flag));
		line++;
#endif

#if MOTOR_DRIVER_OPENLOOP_CURRENT
		len += snprintf(out_buf + len, 4096 - len, "OpenI|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].open_current,
				g_motor_status->motors[1].open_current,
				g_motor_status->motors[2].open_current,
				g_motor_status->motors[3].open_current);
		line++;
#endif

#if MOTOR_DRIVER_CLOSEDLOOP_CURRENT
		len += snprintf(out_buf + len, 4096 - len, "ClosI|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].close_current,
				g_motor_status->motors[1].close_current,
				g_motor_status->motors[2].close_current,
				g_motor_status->motors[3].close_current);
		line++;
#endif

#if MOTOR_DRIVER_BAUDRATE
		len += snprintf(out_buf + len, 4096 - len, "Uart | %-6s | %-6s | %-6s | %-6s |\r\n",
				uart_baud_str(g_motor_status->motors[0].uart_baudrate),
				uart_baud_str(g_motor_status->motors[1].uart_baudrate),
				uart_baud_str(g_motor_status->motors[2].uart_baudrate),
				uart_baud_str(g_motor_status->motors[3].uart_baudrate));
		line++;
#endif

#if MOTOR_DRIVER_CAN_RATE
		len += snprintf(out_buf + len, 4096 - len, "Can  | %-6s | %-6s | %-6s | %-6s |\r\n",
				can_baud_str(g_motor_status->motors[0].can_baudrate),
				can_baud_str(g_motor_status->motors[1].can_baudrate),
				can_baud_str(g_motor_status->motors[2].can_baudrate),
				can_baud_str(g_motor_status->motors[3].can_baudrate));
		line++;
#endif

#if MOTOR_DRIVER_POS_ARRIVE
		len += snprintf(out_buf + len, 4096 - len, "PosAr| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].pos_arrive),
				onoff_str(g_motor_status->motors[1].pos_arrive),
				onoff_str(g_motor_status->motors[2].pos_arrive),
				onoff_str(g_motor_status->motors[3].pos_arrive));
		line++;
#endif

#if MOTOR_DRIVER_PULSE_THRESHOLD
		len += snprintf(out_buf + len, 4096 - len, "PulsT|%7d|%7d|%7d|%7d|\r\n",
				g_motor_status->motors[0].pulse_threshold,
				g_motor_status->motors[1].pulse_threshold,
				g_motor_status->motors[2].pulse_threshold,
				g_motor_status->motors[3].pulse_threshold);
		line++;
#endif

#if MOTOR_DRIVER_LOCK_KEY
		len += snprintf(out_buf + len, 4096 - len, "LockK| %-4s | %-4s | %-4s | %-4s |\r\n",
				onoff_str(g_motor_status->motors[0].lock_key),
				onoff_str(g_motor_status->motors[1].lock_key),
				onoff_str(g_motor_status->motors[2].lock_key),
				onoff_str(g_motor_status->motors[3].lock_key));
		line++;
#endif

		/* 输出缓冲区内容 */
		logPrintln("%s", out_buf);
		len = 0;

		osDelay(g_motor_status->update_time);
		if (shell.read(&ch, 1) == 1) {
			if (ch == 0x03) break;
		}
	}

	vPortFree(out_buf);
	logPrintln("\033[2J\033[1;1H");
}
#endif /* USE_VIEW */
