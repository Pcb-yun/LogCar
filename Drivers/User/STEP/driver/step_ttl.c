/**
 * @file step_ttl.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 串口处理逻辑层源文件
 */

#include "step_ttl.h"
#include "ZDT_V5_Driver.h"
#include "log.h"
#include "cmsis_os.h"
#include "usart.h"
#include "Events.h"
#include <stdlib.h>
#include <string.h>


extern MotorStatusShared_t *g_motor_status;

static void Motor_Process_Ctrl(uint8_t motor_id, MotorCtrl_t *ctrl);
static void Motor_Process_Trigger(uint8_t motor_id, MotorTrigger_t *trigger);
static void Motor_Process_Param_Read(uint8_t motor_id, MotorParamRead_t *read);
static void Motor_Process_Param_Write(uint8_t motor_id, MotorParamWrite_t *write);


/**
 * @brief 发送串口消息
 * @param cmd 串口消息指针
 * @param len 串口消息长度
 */
void Motor_TTL_Send(uint8_t *cmd, uint8_t len) {
    osEventFlagsClear(System_StatusHandle, UART6_TX_IDLE);

    if (HAL_UART_Transmit_DMA(&huart6, cmd, len) != HAL_OK) {
        logError("UART DMA transmit failed"); return;
    }

    osEventFlagsWait(System_StatusHandle, UART6_TX_IDLE, osFlagsWaitAny, osWaitForever);
}

/**
 * @brief 处理串口接收消息
 * @param data 串口接收消息指针
 * @param len 串口接收消息长度
 */
void Motor_Receive(uint8_t *data, uint8_t len) {
	if (len < 3) return;

	uint8_t motor_id = data[0];
	uint8_t cmd_code = data[1];

	if (motor_id > 4 || motor_id == 0) return;

	MotorStatus_t *motor = &g_motor_status->motors[motor_id - 1];
	motor->motor_id = motor_id;

	switch (cmd_code) {
	    /* ---- 命令应答处理 ---- */
#if CURRENT_FIRMWARE == FIRMWARE_X
        case CMD_TORQUE_MODE: case CMD_TORQUE_MODE_LIMIT: case CMD_VELOCITY_MODE_LIMIT: case CMD_POS_MODE_DIRECT: case CMD_POS_MODE_DIRECT_LIMIT:
        case CMD_POS_MODE_TRAPEZOIDAL: case CMD_POS_MODE_TRAPEZOIDAL_LIMIT:
#elif CURRENT_FIRMWARE == FIRMWARE_EMM
        case CMD_POS_MODE_EMM:
#endif
        case CMD_VELOCITY_MODE: case CMD_POS_MODE_FAST_SET: case CMD_POS_MODE_FAST_SEND: case CMD_STOP_NOW: case CMD_SYNC_MOTION: case CMD_MULTI_MOTOR: case CMD_MOTOR_ENABLE:
        case CMD_SET_HOME_ZERO: case CMD_TRIGGER_HOME: case CMD_HOME_INTERRUPT: case CMD_SET_MOTOR_ID: case CMD_SET_MICRO_STEP: case CMD_SET_POWER_FLAG: case CMD_SET_MOTOR_TYPE:
        case CMD_SET_FIRMWARE_TYPE: case CMD_SET_OPENLOOP_CURRENT: case CMD_SET_CLOSEDLOOP_CURRENT: case CMD_SET_PID_PARAMS: case CMD_SET_DMX512_PARAMS: case CMD_SET_POS_WINDOW:
        case CMD_SET_PROTECT_THRESHOLD: case CMD_SET_HEARTBEAT_TIME: case CMD_SET_INTEGRAL_LIMIT: case CMD_SET_COLLISION_ANGLE:
        case CMD_SET_LOCK_PARAMS: case CMD_SET_DRIVER_CONFIG: case CMD_RESET_CURPOS_TO_ZERO:
            if (len >= 4) {
                uint8_t response_code = data[2];
				switch (response_code) {
					case RESPONSE_PASS: break;
					case RESPONSE_PARAM_ERR: logWarning("Motor %d parameter error", motor_id); break;
					case RESPONSE_COD_ERR: logWarning("Motor %d command format error", motor_id); break;
					default: logWarning("Motor %d Response Code Unknown", motor_id, response_code); break;
				}
            } break;

	    /* ---- 系统状态参数读取应答 ---- */
#if MOTOR_STATUS_READ_BATCH
		case CMD_READ_SYSTEM_STATUS:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
		    if (len >= 31) {
#if MOTOR_STATUS_BUS_VOLTAGE
			    motor->voltage = ((uint16_t)data[4] << 8) | (uint16_t)data[5];
#endif /* MOTOR_STATUS_BUS_VOLTAGE */
#if MOTOR_STATUS_PHASE_CURRENT
			    motor->phase_current = ((uint16_t)data[6] << 8) | (uint16_t)data[7];
#endif /* MOTOR_STATUS_PHASE_CURRENT */
#if MOTOR_STATUS_ENCODER_VALUE
			    motor->encoder_linear = ((uint16_t)data[8] << 8) | (uint16_t)data[9];
#endif /* MOTOR_STATUS_ENCODER_VALUE */
#if MOTOR_STATUS_TARGET_POS
			    { int32_t tp = ((int32_t)data[11] << 24) | ((int32_t)data[12] << 16) | ((int32_t)data[13] << 8) | (int32_t)data[14]; if (data[10] == 0x01) tp = -tp; motor->target_pos = tp; }
#endif /* MOTOR_STATUS_TARGET_POS */
#if MOTOR_STATUS_SPEED
			    { int16_t spd = ((int16_t)data[16] << 8) | (int16_t)data[17]; if (data[15] == 0x01) spd = -spd; motor->vel = spd; }
#endif /* MOTOR_STATUS_SPEED */
#if MOTOR_STATUS_REAL_POS
			    { int32_t p = ((int32_t)data[19] << 24) | ((int32_t)data[20] << 16) | ((int32_t)data[21] << 8) | (int32_t)data[22]; if (data[18] == 0x01) p = -p; motor->pos = p; }
#endif /* MOTOR_STATUS_REAL_POS */
#if MOTOR_STATUS_POS_ERROR
			    { int32_t e = ((int32_t)data[24] << 24) | ((int32_t)data[25] << 16) | ((int32_t)data[26] << 8) | (int32_t)data[27]; if (data[23] == 0x01) e = -e; motor->pos_error = e; }
#endif /* MOTOR_STATUS_POS_ERROR */
#if MOTOR_STATUS_TEMPERATURE
			    motor->temp = (data[28] == 0x01) ? -(int8_t)data[29] : (int8_t)data[29];
#endif /* MOTOR_STATUS_TEMPERATURE */
#if MOTOR_STATUS_HOME_FLAGS
			    motor->home_flags = data[30];
#endif /* MOTOR_STATUS_HOME_FLAGS */
#if MOTOR_STATUS_MOTOR_FLAGS
			    { uint8_t st = data[31]; motor->ens = (st & 0x01) != 0; motor->prf = (st & 0x02) != 0; motor->cgi = (st & 0x04) != 0; motor->cgp = (st & 0x08) != 0; motor->esi_l = (st & 0x10) != 0; motor->esi_r = (st & 0x40) != 0; motor->oac = (st & 0x80) != 0; }
#endif /* MOTOR_STATUS_MOTOR_FLAGS */
            }
#elif CURRENT_FIRMWARE == FIRMWARE_X
		    if (len >= 37) {
#if MOTOR_STATUS_BUS_VOLTAGE
			    motor->voltage = ((uint16_t)data[4] << 8) | (uint16_t)data[5];
#endif /* MOTOR_STATUS_BUS_VOLTAGE */
#if MOTOR_STATUS_BUS_CURRENT
			    motor->bus_current = ((uint16_t)data[6] << 8) | (uint16_t)data[7];
#endif /* MOTOR_STATUS_BUS_CURRENT */
#if MOTOR_STATUS_PHASE_CURRENT
			    motor->phase_current = ((uint16_t)data[8] << 8) | (uint16_t)data[9];
#endif /* MOTOR_STATUS_PHASE_CURRENT */
#if MOTOR_STATUS_ENCODER_RAW
			    motor->encoder_raw = ((uint16_t)data[10] << 8) | (uint16_t)data[11];
#endif /* MOTOR_STATUS_ENCODER_RAW */
#if MOTOR_STATUS_ENCODER_VALUE
			    motor->encoder_linear = ((uint16_t)data[12] << 8) | (uint16_t)data[13];
#endif /* MOTOR_STATUS_ENCODER_VALUE */
#if MOTOR_STATUS_TARGET_POS
			    { int32_t tp = ((int32_t)data[15] << 24) | ((int32_t)data[16] << 16) | ((int32_t)data[17] << 8) | (int32_t)data[18]; if (data[14] == 0x01) tp = -tp; motor->target_pos = tp; }
#endif /* MOTOR_STATUS_TARGET_POS */
#if MOTOR_STATUS_SPEED
			    { int16_t spd = ((int16_t)data[20] << 8) | (int16_t)data[21]; if (data[19] == 0x01) spd = -spd; motor->vel = spd; }
#endif /* MOTOR_STATUS_SPEED */
#if MOTOR_STATUS_REAL_POS
			    { int32_t p = ((int32_t)data[23] << 24) | ((int32_t)data[24] << 16) | ((int32_t)data[25] << 8) | (int32_t)data[26]; if (data[22] == 0x01) p = -p; motor->pos = p; }
#endif /* MOTOR_STATUS_REAL_POS */
#if MOTOR_STATUS_POS_ERROR
                { int32_t e = ((int32_t)data[28] << 24) | ((int32_t)data[29] << 16) | ((int32_t)data[30] << 8) | (int32_t)data[31]; if (data[27] == 0x01) e = -e; motor->pos_error = e; }
#endif /* MOTOR_STATUS_POS_ERROR */
#if MOTOR_STATUS_TEMPERATURE
			    motor->temp = (data[32] == 0x01) ? -(int8_t)data[33] : (int8_t)data[33];
#endif /* MOTOR_STATUS_TEMPERATURE */
#if MOTOR_STATUS_HOME_FLAGS
			    motor->home_flags = data[34];
#endif /* MOTOR_STATUS_HOME_FLAGS */
#if MOTOR_STATUS_MOTOR_FLAGS
			    { uint8_t st = data[35]; motor->ens = (st & 0x01) != 0; motor->prf = (st & 0x02) != 0; motor->cgi = (st & 0x04) != 0; motor->cgp = (st & 0x08) != 0; motor->esi_l = (st & 0x10) != 0; motor->esi_r = (st & 0x40) != 0; motor->oac = (st & 0x80) != 0; }
#endif /* MOTOR_STATUS_MOTOR_FLAGS */
		    }
#endif /* CURRENT_FIRMWARE */
			break;
#else
#if MOTOR_STATUS_BUS_VOLTAGE
	    case CMD_READ_BUS_VOLTAGE: if (len >= 5) motor->voltage = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_BUS_VOLTAGE */
#if MOTOR_STATUS_PHASE_CURRENT
	    case CMD_READ_PHASE_CURRENT: if (len >= 5) motor->phase_current = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_PHASE_CURRENT */
#if MOTOR_STATUS_ENCODER_VALUE
	    case CMD_READ_ENCODER_VALUE: if (len >= 5) motor->encoder_linear = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_ENCODER_VALUE */
#if MOTOR_STATUS_TARGET_POS
	    case CMD_READ_TARGET_POSITION: if (len >= 8) { int32_t pos = ((int32_t)data[3] << 24) | ((uint32_t)data[4] << 16) | ((uint32_t)data[5] << 8) | (uint32_t)data[6]; if (data[2] == 0x01) pos = -pos;
#if CURRENT_FIRMWARE == FIRMWARE_X
			    pos /= 10;
#endif
			    motor->target_pos = pos; } break;
#endif /* MOTOR_STATUS_TARGET_POS */
#if MOTOR_STATUS_SPEED
	    case CMD_READ_SPEED: if (len >= 6) { int16_t vel = (data[3] << 8) | data[4]; if (data[2] == 0x01) vel = -vel;
#if CURRENT_FIRMWARE == FIRMWARE_X
			    vel /= 10;
#endif
			    motor->vel = vel; } break;
#endif /* MOTOR_STATUS_SPEED */
#if MOTOR_STATUS_REAL_POS
	    case CMD_READ_POSITION: if (len >= 8) { int32_t pos = ((int32_t)data[3] << 24) | ((uint32_t)data[4] << 16) | ((uint32_t)data[5] << 8) | (uint32_t)data[6]; if (data[2] == 0x01) pos = -pos;
#if CURRENT_FIRMWARE == FIRMWARE_X
                pos /= 10;
#endif
                motor->pos = pos; } break;
#endif /* MOTOR_STATUS_REAL_POS */
#if MOTOR_STATUS_POS_ERROR
	    case CMD_READ_POSITION_ERROR: if (len >= 8) { int32_t error = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6]; if (data[2] == 0x01) error = -error;
#if CURRENT_FIRMWARE == FIRMWARE_X
			    error /= 100;
#endif
			    motor->pos_error = error; } break;
#endif /* MOTOR_STATUS_POS_ERROR */
#if MOTOR_STATUS_MOTOR_FLAGS
        case CMD_READ_MOTOR_STATUS: if (len >= 4) { uint8_t status = data[2]; motor->ens = (status & 0x01) != 0; motor->prf = (status & 0x02) != 0; motor->cgi = (status & 0x04) != 0; motor->cgp = (status & 0x08) != 0; motor->esi_l = (status & 0x10) != 0; motor->esi_r = (status & 0x40) != 0; motor->oac = (status & 0x80) != 0; } break;
#endif /* MOTOR_STATUS_MOTOR_FLAGS */
#if MOTOR_STATUS_HOME_FLAGS
        case CMD_READ_HOME_STATUS: if (len >= 4) { uint8_t status = data[2]; motor->enc_rdy = (status & 0x01) != 0; motor->cal_rdy = (status & 0x02) != 0; motor->org_sf = (status & 0x04) != 0; motor->org_cf = (status & 0x08) != 0; motor->otp_tf = (status & 0x10) != 0; motor->ocp_tf = (status & 0x80) != 0; } break;
#endif /* MOTOR_STATUS_HOME_FLAGS */
#if MOTOR_STATUS_BUS_CURRENT
	    case CMD_READ_BUS_CURRENT: if (len >= 5) motor->bus_current = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_BUS_CURRENT */
#if MOTOR_STATUS_TEMPERATURE
        case CMD_READ_TEMPERATURE: if (len >= 5) { int8_t temp = data[3]; if (data[2] == 0x01) temp = -temp; motor->temp = temp; } break;
#endif /* MOTOR_STATUS_TEMPERATURE */
#if MOTOR_STATUS_FLAGS_COMBINED
        case CMD_READ_STATUS_FLAGS: if (len >= 5) { uint8_t home_st = data[2]; uint8_t mot_st = data[3]; motor->enc_rdy = (home_st & 0x01) != 0; motor->cal_rdy = (home_st & 0x02) != 0; motor->org_sf = (home_st & 0x04) != 0; motor->org_cf = (home_st & 0x08) != 0; motor->otp_tf = (home_st & 0x10) != 0; motor->ocp_tf = (home_st & 0x80) != 0; motor->ens = (mot_st & 0x01) != 0; motor->prf = (mot_st & 0x02) != 0; motor->cgi = (mot_st & 0x04) != 0; } break;
#endif /* MOTOR_STATUS_FLAGS_COMBINED */
#endif /* MOTOR_STATUS_READ_BATCH */
#if MOTOR_STATUS_INPUT_PULSES
	    case CMD_READ_INPUT_PULSES: if (len >= 8) { int32_t pulses = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6]; if (data[2] == 0x01) pulses = -pulses; motor->input_pulses = pulses; } break;
#endif /* MOTOR_STATUS_INPUT_PULSES */
#if MOTOR_STATUS_SET_POS
	    case CMD_READ_SET_POSITION: if (len >= 8) { int32_t pos = ((int32_t)data[3] << 24) | ((uint32_t)data[4] << 16) | ((uint32_t)data[5] << 8) | (uint32_t)data[6]; if (data[2] == 0x01) pos = -pos;
#if CURRENT_FIRMWARE == FIRMWARE_X
			    pos /= 10;
#endif
			    motor->set_pos = pos; } break;
#endif /* MOTOR_STATUS_SET_POS */
#if MOTOR_STATUS_PIN_STATUS
	    case CMD_READ_PIN_STATUS: if (len >= 4) motor->pin_status = data[2]; break;
#endif /* MOTOR_STATUS_PIN_STATUS */
#if MOTOR_STATUS_BATTERY_VOLTAGE
        case CMD_READ_BATTERY_VOLTAGE: if (len >= 5) motor->battery_voltage = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_BATTERY_VOLTAGE */

	    /* ---- 驱动配置参数读取应答 ---- */
#if MOTOR_DRIVER_READ_BATCH
	    case CMD_READ_DRIVER_CONFIG:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
		    if (len >= 33) {
#if MOTOR_DRIVER_MOTOR_TYPE
			    motor->motor_type = (data[4] == 0x19) ? true : false;  // 0x19=0.9°, 0x32=1.8°
#endif
#if MOTOR_DRIVER_PULSE_PORT_MODE
			    motor->pulse_port_mode = data[5];
#endif
#if MOTOR_DRIVER_COMM_PORT_MODE
			    motor->comm_port_mode = data[6];
#endif
#if MOTOR_DRIVER_EN_PIN_LEVEL
			    motor->en_pin_level = data[7];
#endif
#if MOTOR_DRIVER_DIR_PIN_LEVEL
			    motor->dir_pin_level = data[8];
#endif
#if MOTOR_DRIVER_MICRO_STEP
			    motor->micro_step = data[9];
#endif
#if MOTOR_DRIVER_FIRMWARE_TYPE
			    motor->interpolation = data[10];
#endif
#if MOTOR_DRIVER_AUTO_SCREEN_OFF
			    motor->auto_screen_off = data[11];
#endif
#if MOTOR_DRIVER_OPENLOOP_CURRENT
			    motor->open_current = ((uint16_t)data[12] << 8) | (uint16_t)data[13];
#endif
#if MOTOR_DRIVER_CLOSEDLOOP_CURRENT
			    motor->close_current = ((uint16_t)data[14] << 8) | (uint16_t)data[15];
#endif
#if MOTOR_DRIVER_CLOSEDLOOP_MAX_VOLTAGE
			    motor->close_max_voltage = ((uint16_t)data[16] << 8) | (uint16_t)data[17];
#endif
#if MOTOR_DRIVER_BAUDRATE
			    motor->uart_baudrate = data[18];
#endif
#if MOTOR_DRIVER_CAN_RATE
			    motor->can_baudrate = data[19];
#endif
#if MOTOR_DRIVER_COMM_CHECK_MODE
			    motor->comm_check_mode = data[21];
#endif
#if MOTOR_DRIVER_CMD_RESPONSE_MODE
			    motor->cmd_response_mode = data[22];
#endif
#if MOTOR_DRIVER_STALL_PROTECT
			    motor->clog_enable = data[23];
#endif
#if MOTOR_DRIVER_STALL_SPEED
			    motor->clog_rpm = ((uint16_t)data[24] << 8) | (uint16_t)data[25];
#endif
#if MOTOR_DRIVER_STALL_CURRENT
			    motor->clog_current = ((uint16_t)data[26] << 8) | (uint16_t)data[27];
#endif
#if MOTOR_DRIVER_STALL_TIME
			    motor->clog_time = ((uint16_t)data[28] << 8) | (uint16_t)data[29];
#endif
#if MOTOR_DRIVER_POS_WINDOW
			    motor->pos_window = ((uint16_t)data[30] << 8) | (uint16_t)data[31];
#endif
		    }
#elif CURRENT_FIRMWARE == FIRMWARE_X
		    if (len >= 37) {
#if MOTOR_DRIVER_LOCK_KEY
			    motor->lock_key = data[4];
#endif
#if MOTOR_DRIVER_CONTROL_MODE
			    motor->control_mode = data[5];
#endif
#if MOTOR_DRIVER_PULSE_PORT_MODE
			    motor->pulse_port_mode = data[6];
#endif
#if MOTOR_DRIVER_COMM_PORT_MODE
			    motor->comm_port_mode = data[7];
#endif
#if MOTOR_DRIVER_EN_PIN_LEVEL
			    motor->en_pin_level = data[8];
#endif
#if MOTOR_DRIVER_DIR_PIN_LEVEL
			    motor->dir_pin_level = data[9];
#endif
#if MOTOR_DRIVER_MICRO_STEP
			    motor->micro_step = data[10];
#endif
#if MOTOR_DRIVER_FIRMWARE_TYPE
			    motor->interpolation = data[11];
#endif
#if MOTOR_DRIVER_AUTO_SCREEN_OFF
			    motor->auto_screen_off = data[12];
#endif
#if MOTOR_DRIVER_OPENLOOP_CURRENT
			    motor->open_current = ((uint16_t)data[15] << 8) | (uint16_t)data[16];
#endif
#if MOTOR_DRIVER_CLOSEDLOOP_CURRENT
			    motor->close_current = ((uint16_t)data[17] << 8) | (uint16_t)data[18];
#endif
#if MOTOR_DRIVER_CLOSEDLOOP_MAX_SPEED
			    motor->close_max_speed = ((uint16_t)data[19] << 8) | (uint16_t)data[20];
#endif
#if MOTOR_DRIVER_CURRENT_LOOP_BANDWIDTH
			    motor->current_loop_bw = ((uint16_t)data[21] << 8) | (uint16_t)data[22];
#endif
#if MOTOR_DRIVER_BAUDRATE
			    motor->uart_baudrate = data[23];
#endif
#if MOTOR_DRIVER_CAN_RATE
			    motor->can_baudrate = data[24];
#endif
#if MOTOR_DRIVER_COMM_CHECK_MODE
			    motor->comm_check_mode = data[25];
#endif
#if MOTOR_DRIVER_CMD_RESPONSE_MODE
			    motor->cmd_response_mode = data[26];
#endif
#if MOTOR_DRIVER_SCALE_10X
			    motor->scale_10x = data[27];
#endif
#if MOTOR_DRIVER_STALL_PROTECT
			    motor->clog_enable = data[28];
#endif
#if MOTOR_DRIVER_STALL_SPEED
			    motor->clog_rpm = ((uint16_t)data[29] << 8) | (uint16_t)data[30];
#endif
#if MOTOR_DRIVER_STALL_CURRENT
			    motor->clog_current = ((uint16_t)data[31] << 8) | (uint16_t)data[32];
#endif
#if MOTOR_DRIVER_STALL_TIME
			    motor->clog_time = ((uint16_t)data[33] << 8) | (uint16_t)data[34];
#endif
#if MOTOR_DRIVER_POS_WINDOW
			    motor->pos_window = data[35];
#endif
		    }
#endif /* CURRENT_FIRMWARE */
#endif /* MOTOR_DRIVER_READ_BATCH */
#if MOTOR_DRIVER_POS_WINDOW
        case CMD_READ_POSITION_WINDOW: if (len >= 5) motor->pos_window = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_DRIVER_POS_WINDOW */
#if MOTOR_DRIVER_HOME
        case CMD_READ_HOME_PARAMS: if (len >= 18) {
#if MOTOR_DRIVER_HOME_MODE
		    motor->home_mode = data[2];
#endif
#if MOTOR_DRIVER_HOME_DIR
		    motor->home_dir = data[3];
#endif
#if MOTOR_DRIVER_HOME_SPEED
		    motor->home_speed = ((uint16_t)data[5] << 8) | (uint16_t)data[4];
#endif
#if MOTOR_DRIVER_HOME_TIMEOUT
		    motor->home_timeout = ((uint32_t)data[9] << 24) | ((uint32_t)data[8] << 16) | ((uint32_t)data[7] << 8) | (uint32_t)data[6];
#endif
#if MOTOR_DRIVER_HOME_COLLISION_SPEED
		    motor->sl_vel = ((uint16_t)data[11] << 8) | (uint16_t)data[10];
#endif
#if MOTOR_DRIVER_HOME_COLLISION_CURRENT
		    motor->sl_current = ((uint16_t)data[13] << 8) | (uint16_t)data[12];
#endif
#if MOTOR_DRIVER_HOME_COLLISION_TIME
		    motor->sl_time = ((uint16_t)data[15] << 8) | (uint16_t)data[14];
#endif
#if MOTOR_DRIVER_AUTO_HOME
		    motor->auto_home = data[16];
#endif
		} break;
#endif /* MOTOR_DRIVER_HOME */

/* ---- 控制参数读取应答 ---- */
#if MOTOR_PID_READ
	case CMD_READ_PID_PARAMS:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
		if (len >= 15) { motor->kp = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5]; motor->ki = ((uint32_t)data[6] << 24) | ((uint32_t)data[7] << 16) | ((uint32_t)data[8] << 8) | data[9]; motor->kd = ((uint32_t)data[10] << 24) | ((uint32_t)data[11] << 16) | ((uint32_t)data[12] << 8) | data[13]; }
#elif CURRENT_FIRMWARE == FIRMWARE_X
		if (len >= 19) { motor->trapezoidal_kp = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5]; motor->direct_kp = ((uint32_t)data[6] << 24) | ((uint32_t)data[7] << 16) | ((uint32_t)data[8] << 8) | data[9]; motor->vel_kp = ((uint32_t)data[10] << 24) | ((uint32_t)data[11] << 16) | ((uint32_t)data[12] << 8) | data[13]; motor->vel_ki = ((uint32_t)data[14] << 24) | ((uint32_t)data[15] << 16) | ((uint32_t)data[16] << 8) | data[17]; }
#endif /* CURRENT_FIRMWARE */
		break;
#endif /* MOTOR_PID_READ */
#if MOTOR_INTEGRAL_LIMIT_READ
	    case CMD_READ_INTEGRAL_LIMIT: if (len >= 7) motor->integral_limit = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5]; break;
#endif /* MOTOR_INTEGRAL_LIMIT_READ */
#if MOTOR_PROTECT_THRESHOLD_READ
        case CMD_READ_PROTECT_THRESHOLD: if (len >= 9) { motor->temp_threshold = (data[2] << 8) | data[3]; motor->current_threshold = (data[4] << 8) | data[5]; motor->protect_time = (data[6] << 8) | data[7]; } break;
#endif /* MOTOR_PROTECT_THRESHOLD_READ */
#if MOTOR_HEARTBEAT_READ
        case CMD_READ_HEARTBEAT_TIME: if (len >= 7) { motor->heartbeat_time = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5]; } break;
#endif /* MOTOR_HEARTBEAT_READ */
#if MOTOR_COLLISION_ANGLE_READ
	    case CMD_READ_COLLISION_ANGLE: if (len >= 5) motor->collision_angle = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_COLLISION_ANGLE_READ */

        /* ---- 设备信息读取应答 ---- */
#if MOTOR_READ_VERSION
        case CMD_READ_VERSION_INFO: if (len >= 6) { motor->firmware_version = (data[3] << 8) | data[2]; motor->hardware_series = (data[4] >> 4) & 0x0F; motor->hardware_type = data[4] & 0x0F; motor->hardware_version = data[5]; } break;
#endif /* MOTOR_READ_VERSION */
#if MOTOR_READ_PHASE_PARAMS
        case CMD_READ_PHASE_PARAMS: if (len >= 6) { motor->phase_resistance  = (data[2] << 8) | data[3]; motor->phase_inductance  = (data[4] << 8) | data[5]; } break;
#endif /* MOTOR_READ_PHASE_PARAMS */
#if MOTOR_READ_OPTION_PARAMS
        case CMD_READ_OPTION_PARAMS: if (len >= 5) {
#if MOTOR_LOCK_PARAMS
			motor->lock_level = data[2] & 0x03;
#endif
#if MOTOR_FIRMWARE_TYPE
			motor->firmware_type = (data[3] >> 1) & 0x01;
#endif
#if MOTOR_DIRECTION
			motor->motor_dir = (data[3] >> 4) & 0x01;
#endif
#if MOTOR_DRIVER_MOTOR_TYPE
                motor->motor_type = (data[3] >> 0) & 0x01;
#endif
#if MOTOR_DRIVER_CONTROL_MODE
                motor->control_mode = (data[3] >> 2) & 0x01;
#endif
#if MOTOR_DRIVER_LOCK_KEY
                motor->lock_key = (data[3] >> 5) & 0x01;
#endif
#if MOTOR_DRIVER_SCALE_10X
                motor->scale_10x = (data[3] >> 7) & 0x01;
#endif
        } break;
#endif /* MOTOR_READ_OPTION_PARAMS */
#if MOTOR_BROADCAST_READ_ID
        case CMD_BROADCAST_READ_ID: motor->is_online = true; break;
#endif /* MOTOR_BROADCAST_READ_ID */
	    default: logWarning("Received motor response, but command unknown: id: %d, cmd: 0x%02X", motor_id, cmd_code); break;
	}
}

/**
 * @brief 处理电机命令
 * @param cmd 电机命令指针
 */
void Motor_Process_Cmd(MotorCmd_t *cmd) {
	switch (cmd->op_type) {
        case OP_CONTROL: Motor_Process_Ctrl(cmd->motor_id, &cmd->type.ctrl); break;
        case OP_TRIGGER: Motor_Process_Trigger(cmd->motor_id, &cmd->type.trigger); break;
        case OP_PARAM_READ: Motor_Process_Param_Read(cmd->motor_id, &cmd->type.read); break;
        case OP_PARAM_WRITE: Motor_Process_Param_Write(cmd->motor_id, &cmd->type.write); break;
        case OP_NONE: default: logWarning("Unknown or none operation type: %d", cmd->op_type); break;
	}
}

/**
 * @brief 处理电机控制命令
 * @param motor_id 电机ID
 * @param ctrl 电机控制命令指针
 */
static void Motor_Process_Ctrl(uint8_t motor_id, MotorCtrl_t *ctrl) {
	switch (ctrl->type) {

#if MOTOR_CMD_ENABLE
	    case CTRL_ENABLE: ZDT_V5_En_Control(motor_id, ctrl->p.en.enable, ctrl->p.en.sync); break;
#endif
#if MOTOR_VELOCITY_MODE
	    case CTRL_VELOCITY: ZDT_V5_Vel_Control(motor_id, ctrl->p.vel.dir, ctrl->p.vel.vel, ctrl->p.vel.acc, ctrl->p.vel.sync); break;
#endif
#if MOTOR_VELOCITY_MODE_LIMIT
	    case CTRL_VELOCITY_LIMIT: ZDT_V5_Vel_Control_With_Limit(motor_id, ctrl->p.vel.dir, ctrl->p.vel.vel, ctrl->p.vel.acc, ctrl->p.vel.sync, ctrl->p.vel.max_current); break;
#endif
#if MOTOR_POS_MODE_TRAPEZOIDAL
	    case CTRL_POSITION: ZDT_V5_Pos_Control(motor_id, ctrl->p.pos.dir, ctrl->p.pos.vel, ctrl->p.pos.acc,
#if CURRENT_FIRMWARE == FIRMWARE_X
			    ctrl->p.pos.dec,
#else
			    0,
#endif
			    (uint32_t)ctrl->p.pos.target, ctrl->p.pos.mode, ctrl->p.pos.sync); break;
#endif
#if MOTOR_POS_MODE_TRAPEZOIDAL_LIMIT
	    case CTRL_POS_TRAPEZOIDAL_LIMIT: ZDT_V5_Pos_Control_Trapezoidal_With_Limit(motor_id, ctrl->p.pos.dir, ctrl->p.pos.vel, ctrl->p.pos.acc,
#if CURRENT_FIRMWARE == FIRMWARE_X
			    ctrl->p.pos.dec,
#else
			    0,
#endif
			    (uint32_t)ctrl->p.pos.target, ctrl->p.pos.mode, ctrl->p.pos.sync, ctrl->p.pos.max_current); break;
#endif
#if MOTOR_POS_MODE_DIRECT
	    case CTRL_POS_DIRECT: ZDT_V5_Pos_Control_Direct(motor_id, ctrl->p.pos_dir.dir, ctrl->p.pos_dir.vel, (uint32_t)ctrl->p.pos_dir.target, ctrl->p.pos_dir.mode, ctrl->p.pos_dir.snF); break;
#endif
#if MOTOR_POS_MODE_DIRECT_LIMIT
	    case CTRL_POS_DIRECT_LIMIT: ZDT_V5_Pos_Control_Direct_With_Limit(motor_id, ctrl->p.pos_dir.dir, ctrl->p.pos_dir.vel, (uint32_t)ctrl->p.pos_dir.target, ctrl->p.pos_dir.mode, ctrl->p.pos_dir.snF, ctrl->p.pos_dir.max_current); break;
#endif
#if MOTOR_TORQUE_MODE
	    case CTRL_TORQUE: ZDT_V5_Torque_Control(motor_id, ctrl->p.torque.dir, ctrl->p.torque.slope, ctrl->p.torque.current, ctrl->p.torque.sync); break;
#endif
#if MOTOR_TORQUE_MODE_LIMIT
	    case CTRL_TORQUE_LIMIT: ZDT_V5_Torque_Control_With_Limit(motor_id, ctrl->p.torque.dir, ctrl->p.torque.slope, ctrl->p.torque.current, ctrl->p.torque.max_vel, ctrl->p.torque.sync); break;
#endif
#if MOTOR_CMD_STOP
	    case CTRL_STOP: ZDT_V5_Stop(motor_id, ctrl->p.stop.sync); break;
#endif
#if MOTOR_SYNC_TRIGGER
	    case CTRL_SYNC: ZDT_V5_Synchronous_Motion(motor_id); break;
#endif
#if MOTOR_MULTI_CMD
        case CTRL_MULTI: ZDT_V5_Multi_Motor_Cmd(0x00, ctrl->p.multi.len, ctrl->p.multi.data); break;
#endif
#if MOTOR_POS_MODE_FAST
	case CTRL_FAST_SET: ZDT_V5_Fast_Set_Param(motor_id, ctrl->p.fast_set.vel, ctrl->p.fast_set.acc,
#if CURRENT_FIRMWARE == FIRMWARE_EMM
        0, 0,
#elif CURRENT_FIRMWARE == FIRMWARE_X
        ctrl->p.fast_set.dec, ctrl->p.fast_set.max_current,
#endif
        ctrl->p.fast_set.mode, ctrl->p.fast_set.sync); break;
	case CTRL_FAST_SEND: ZDT_V5_Fast_Send_Pos(motor_id, ctrl->p.fast_send.pos); break;
#endif /* MOTOR_POS_MODE_FAST */
        case CTRL_NONE: default: logWarning("Unknown or none control type: %d", ctrl->type); break;
	}
}

/**
 * @brief 处理触发动作命令
 * @param motor_id 电机ID
 * @param trigger 触发命令指针
 */
static void Motor_Process_Trigger(uint8_t motor_id, MotorTrigger_t *trigger) {
	switch (trigger->type) {
#if MOTOR_TRIGGER_ENCODER_CALIB
	    case TRIG_ENCODER_CALIB: ZDT_V5_Trig_Encoder_Cal(motor_id); break;
#endif
#if MOTOR_TRIGGER_RESTART
	    case TRIG_RESTART: ZDT_V5_Reset_Motor(motor_id); break;
#endif
#if MOTOR_TRIGGER_RESET_POS
	    case TRIG_RESET_POS: ZDT_V5_Reset_CurPos_To_Zero(motor_id); break;
#endif
#if MOTOR_TRIGGER_CLEAR_PROTECT
	    case TRIG_CLEAR_PROTECT: ZDT_V5_Reset_Clog_Pro(motor_id); break;
#endif
#if MOTOR_TRIGGER_FACTORY_RESET
	    case TRIG_FACTORY_RESET: ZDT_V5_Restore_Motor(motor_id); break;
#endif
#if MOTOR_HOME_SET_ZERO
	    case TRIG_HOME_SET_ZERO: ZDT_V5_Origin_Set_Zero(motor_id, trigger->p.set_zero.save); break;
#endif
#if MOTOR_HOME_TRIGGER
	    case TRIG_HOME_RETURN: ZDT_V5_Origin_Trigger_Return(motor_id, trigger->p.home.mode, trigger->p.home.sync); break;
#endif
#if MOTOR_HOME_INTERRUPT
	    case TRIG_HOME_INTERRUPT: ZDT_V5_Origin_Interrupt(motor_id); break;
#endif
        case TRIG_NONE: default: logWarning("Unknown or none trigger type: %d", trigger->type); break;
	}
}

/**
 * @brief 处理参数读取命令
 * @param motor_id 电机ID
 * @param read 参数读取命令指针
 */
static void Motor_Process_Param_Read(uint8_t motor_id, MotorParamRead_t *read) {
	switch(read->type) {
		case MP_SYS: ZDT_V5_Read_Sys_Params(motor_id, read->p.sys); break;
		case MP_DEV: ZDT_V5_Read_Driver_Params(motor_id, read->p.drv); break;
		case MP_CTRL: ZDT_V5_Read_Ctrl_Params(motor_id, read->p.ctrl); break;
		case MP_INFO: ZDT_V5_Read_Device_Info_Params(motor_id, read->p.info); break;
        case MP_NONE: default: logWarning("Unknown or none param read type: %d", read->type); break;
	}
}

/**
 * @brief 处理电机参数写入命令
 * @param motor_id 电机ID
 * @param write 参数写入命令指针
 */
static void Motor_Process_Param_Write(uint8_t motor_id, MotorParamWrite_t *write) {
	switch (write->type) {
#if MOTOR_SET_MOTOR_ID
	    case PARAM_MOTOR_ID: ZDT_V5_Modify_Motor_ID(motor_id, write->p.motor_id.save, write->p.motor_id.id); break;
#endif
#if MOTOR_SET_MICRO_STEP
	    case PARAM_MICRO_STEP: ZDT_V5_Modify_MicroStep(motor_id, true, write->p.micro_step.step); break;
#endif
#if MOTOR_SET_POWER_FLAG
	    case PARAM_POWER_FLAG: ZDT_V5_Modify_PDFlag(motor_id, write->p.power_flag.enable); break;
#endif
#if MOTOR_SET_MOTOR_TYPE
	    case PARAM_MOTOR_TYPE: ZDT_V5_Modify_Motor_Type(motor_id, write->p.motor_type.save, write->p.motor_type.type); break;
#endif
#if MOTOR_SET_FIRMWARE_TYPE
	    case PARAM_FIRMWARE_TYPE: ZDT_V5_Modify_Firmware_Type(motor_id, write->p.firmware_type.save, write->p.firmware_type.type); break;
#endif
#if MOTOR_SET_OPENLOOP_CURRENT
	    case PARAM_OPENLOOP_CURRENT: ZDT_V5_Modify_OM_mA(motor_id, write->p.openloop_current.save, write->p.openloop_current.current); break;
#endif
#if MOTOR_SET_CLOSEDLOOP_CURRENT
	    case PARAM_CLOSEDLOOP_CURRENT: ZDT_V5_Modify_FOC_mA(motor_id, write->p.closedloop_current.save, write->p.closedloop_current.current); break;
#endif
#if MOTOR_SET_CONTROL_MODE
	    case PARAM_CONTROL_MODE: ZDT_V5_Modify_Ctrl_Mode(motor_id, write->p.control_mode.save, write->p.control_mode.mode); break;
#endif
#if MOTOR_SET_DIRECTION
	    case PARAM_DIRECTION: ZDT_V5_Modify_Motor_Dir(motor_id, write->p.direction.save, write->p.direction.dir); break;
#endif
#if MOTOR_SET_LOCK_KEY
	    case PARAM_LOCK_KEY: ZDT_V5_Modify_Lock_Btn(motor_id, write->p.lock_key.save, write->p.lock_key.lock); break;
#endif
#if MOTOR_SET_SCALE_INPUT
	    case PARAM_SCALE_INPUT: ZDT_V5_Modify_S_Vel(motor_id, write->p.scale_input.save, write->p.scale_input.enable); break;
#endif
#if MOTOR_SET_LOCK_PARAMS
	    case PARAM_LOCK_PARAMS: ZDT_V5_Modify_Lock_Params(motor_id, write->p.lock_params.save, write->p.lock_params.level); break;
#endif
#if MOTOR_POS_WINDOW_WRITE
	    case PARAM_POS_WINDOW: ZDT_V5_Modify_Pos_Window(motor_id, write->p.pos_window.save, write->p.pos_window.window); break;
#endif
#if MOTOR_PID_WRITE
		case PARAM_PID:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
			ZDT_V5_Modify_PID_Params(motor_id, write->p.pid.save, write->p.pid.kp, write->p.pid.ki, write->p.pid.kd);
#elif CURRENT_FIRMWARE == FIRMWARE_X
			ZDT_V5_Modify_PID_Params(motor_id, write->p.pid.save, write->p.pid.trapezoidal_kp, write->p.pid.direct_kp, write->p.pid.vel_kp, write->p.pid.vel_ki);
#endif
			break;
#endif
#if MOTOR_INTEGRAL_LIMIT_WRITE
	    case PARAM_INTEGRAL_LIMIT: ZDT_V5_Modify_Integral_Limit(motor_id, write->p.integral_limit.save, write->p.integral_limit.value); break;
#endif
#if MOTOR_PROTECT_THRESHOLD_WRITE
	    case PARAM_PROTECT_THRESHOLD: ZDT_V5_Modify_Otocp(motor_id, write->p.protect_threshold.save, write->p.protect_threshold.temp, write->p.protect_threshold.current, write->p.protect_threshold.time_ms); break;
#endif
#if MOTOR_HEARTBEAT_WRITE
	    case PARAM_HEARTBEAT: ZDT_V5_Modify_Heart_Protect(motor_id, write->p.heartbeat.save, write->p.heartbeat.time_ms); break;
#endif
#if MOTOR_COLLISION_ANGLE_WRITE
	    case PARAM_COLLISION_ANGLE: ZDT_V5_Modify_Collision_Angle(motor_id, write->p.collision_angle.save, write->p.collision_angle.angle); break;
#endif
#if MOTOR_DMX512_WRITE
	    case PARAM_DMX512: ZDT_V5_Modify_DMX512_Params(motor_id, write->p.dmx512.save, write->p.dmx512.tch, write->p.dmx512.nch, write->p.dmx512.mode, write->p.dmx512.vel, write->p.dmx512.acc, write->p.dmx512.vel_step, write->p.dmx512.pos_step); break;
#endif
#if MOTOR_DRIVER_HOME_WRITE
	    case PARAM_HOME_PARAMS: ZDT_V5_Origin_Modify_Params(motor_id, write->p.home_params.save, write->p.home_params.mode, write->p.home_params.dir, write->p.home_params.vel, write->p.home_params.timeout, write->p.home_params.sl_vel, write->p.home_params.sl_current, write->p.home_params.sl_time, write->p.home_params.auto_home); break;
#endif
#if MOTOR_PERIODIC_RETURN
	    case PARAM_PERIODIC_RETURN: ZDT_V5_Auto_Return_Sys_Params_Timed(motor_id, write->p.periodic_return.param, write->p.periodic_return.time_ms); break;
#endif
        case PARAM_NONE: default: logWarning("Unknown or none param write type: %d", write->type); break;
	}
}
