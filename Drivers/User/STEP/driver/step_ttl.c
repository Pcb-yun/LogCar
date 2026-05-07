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


MotorStatusShared_t g_motor_status;

static void Motor_Process_Ctrl(uint8_t motor_id, MotorCtrl_t *ctrl);
static void Motor_Process_Param_Read(uint8_t motor_id, MotorParam_t *param);
static void Motor_Process_Param_Write(uint8_t motor_id, bool save, MotorParam_t *param);


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
    if (len < 3) {
        return;
    }

    uint8_t motor_id = data[0];
    uint8_t cmd_code = data[1];

    if (motor_id > 4) {
        return;
    }

    MotorStatus_t *motor = &g_motor_status.motors[motor_id - 1];
    motor->motor_id = motor_id;

    switch (cmd_code) {
#if CURRENT_FIRMWARE == FIRMWARE_X
        case CMD_TORQUE_MODE: case CMD_TORQUE_MODE_LIMIT: case CMD_VELOCITY_MODE_LIMIT: case CMD_POS_MODE_DIRECT:
        case CMD_POS_MODE_DIRECT_LIMIT: case CMD_POS_MODE_TRAPEZOIDAL: case CMD_POS_MODE_TRAPEZOIDAL_LIMIT:
#elif CURRENT_FIRMWARE == FIRMWARE_EMM
        case CMD_POS_MODE_EMM:
#endif
        case CMD_VELOCITY_MODE: case CMD_POS_MODE_FAST_SET: case CMD_POS_MODE_FAST_SEND: case CMD_STOP_NOW: case CMD_SYNC_MOTION:
        case CMD_MULTI_MOTOR: case CMD_MOTOR_ENABLE: case CMD_SET_HOME_ZERO: case CMD_TRIGGER_HOME: case CMD_HOME_INTERRUPT:
        case CMD_SET_MOTOR_ID: case CMD_SET_MICRO_STEP: case CMD_SET_POWER_FLAG: case CMD_SET_MOTOR_TYPE: case CMD_SET_FIRMWARE_TYPE:
        case CMD_SET_OPENLOOP_CURRENT: case CMD_SET_CLOSEDLOOP_CURRENT: case CMD_SET_PID_PARAMS: case CMD_SET_DMX512_PARAMS:
        case CMD_SET_POS_WINDOW: case CMD_SET_PROTECT_THRESHOLD: case CMD_SET_HEARTBEAT_TIME: case CMD_SET_INTEGRAL_LIMIT:
        case CMD_SET_COLLISION_ANGLE: case CMD_SET_LOCK_PARAMS: case CMD_SET_DRIVER_CONFIG: case CMD_RESET_CURPOS_TO_ZERO:
            if (len >= 4) {
                uint8_t response_code = data[2];
                if (response_code == 0xE2) { logError("Motor %d parameter error", motor_id); }
                else if (response_code == 0xEE) logError("Motor %d command format error", motor_id);
            } break;
        case CMD_BROADCAST_READ_ID: motor->is_online = true; break;
#if MOTOR_ELECTRICAL
        case CMD_READ_BUS_VOLTAGE: if (len >= 5) motor->voltage = (data[2] << 8) | data[3]; break;
#if CURRENT_FIRMWARE == FIRMWARE_X
        case CMD_READ_BUS_CURRENT: if (len >= 5) motor->bus_current = (data[2] << 8) | data[3]; break;
#endif
        case CMD_READ_PHASE_CURRENT: if (len >= 5) motor->phase_current = (data[2] << 8) | data[3]; break;
        case CMD_READ_TEMPERATURE:
            if (len >= 5) {
                int8_t temp = data[3]; if (data[2] == 0x01) temp = -temp; motor->temp = (int8_t)temp;
            } break;
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case CMD_READ_BATTERY_VOLTAGE: if (len >= 5) motor->battery_voltage = (data[2] << 8) | data[3]; break;
#endif
#endif /* MOTOR_ELECTRICAL */
#if MOTOR_MOTION
        case CMD_READ_SPEED:
            if (len >= 6) {
                int16_t vel = (data[3] << 8) | data[4]; if (data[2] == 0x01) vel = -vel;
#if CURRENT_FIRMWARE == FIRMWARE_X
                vel = vel / 10;
#endif
                motor->vel = vel;
            } break;
        case CMD_READ_POSITION:
            if (len >= 8) {
                int32_t pos = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) pos = -pos;
#if CURRENT_FIRMWARE == FIRMWARE_X
                pos = pos / 10;
#endif
                motor->pos = pos;
            } break;
        case CMD_READ_TARGET_POSITION:
            if (len >= 8) {
                int32_t pos = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) pos = -pos;
#if CURRENT_FIRMWARE == FIRMWARE_X
                pos = pos / 10;
#endif
                motor->target_pos = pos;
            } break;
        case CMD_READ_SET_POSITION:
            if (len >= 8) {
                int32_t pos = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) pos = -pos;
#if CURRENT_FIRMWARE == FIRMWARE_X
                pos = pos / 10;
#endif
                motor->set_pos = pos;
            } break;
        case CMD_READ_POSITION_ERROR:
            if (len >= 8) {
                int32_t error = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) error = -error;
#if CURRENT_FIRMWARE == FIRMWARE_X
                error = error / 100;
#endif
                motor->pos_error = error;
            } break;
#if CURRENT_FIRMWARE == FIRMWARE_X
        case CMD_READ_INPUT_PULSES:
            if (len >= 8) {
                int32_t pulses = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) pulses = -pulses; motor->input_pulses = pulses;
            } break;
#endif
#endif /* MOTOR_MOTION */
#if MOTOR_ENCODER
        case CMD_READ_ENCODER_VALUE: if (len >= 5) motor->encoder_linear = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_ENCODER */
#if MOTOR_STATUS_FLAGS
        case CMD_READ_MOTOR_STATUS: if (len >= 4) motor->status = data[2]; break;
        case CMD_READ_HOME_STATUS: if (len >= 4) motor->home_status = data[2]; break;
        case CMD_READ_STATUS_FLAGS: if (len >= 5) { motor->home_status = data[2]; motor->status = data[3]; } break;
        case CMD_READ_PIN_STATUS: if (len >= 4) motor->pin_status = data[2]; break;
#elif USE_HEARTBEAT
        case CMD_READ_MOTOR_STATUS: break;
#endif /* MOTOR_STATUS_FLAGS */
#if MOTOR_HOME
        case CMD_READ_HOME_PARAMS:
            if (len >= 17) {
                motor->home_mode = data[2]; motor->home_dir = data[3]; motor->home_speed = (data[4] << 8) | data[5];
                motor->home_timeout = ((uint32_t)data[6] << 24) | ((uint32_t)data[7] << 16) | ((uint32_t)data[8] << 8) | data[9];
                motor->collision_rpm = (data[10] << 8) | data[11]; motor->collision_current = (data[12] << 8) | data[13];
                motor->collision_time = (data[14] << 8) | data[15]; motor->home_auto_enable = data[16];
            } break;
#endif /* MOTOR_HOME */
#if MOTOR_SYSTEM
        case CMD_READ_VERSION_INFO:
            if (len >= 6) { motor->firmware_version = (data[2] << 8) | data[3]; motor->hardware_version = data[5]; } break;
#endif /* MOTOR_SYSTEM */
#if MOTOR_CONTROL
        case CMD_READ_PID_PARAMS:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
            if (len >= 15) {
                motor->kp = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) |
                                ((uint32_t)data[4] << 8) | data[5];
                motor->ki = ((uint32_t)data[6] << 24) | ((uint32_t)data[7] << 16) |
                                ((uint32_t)data[8] << 8) | data[9];
                motor->kd = ((uint32_t)data[10] << 24) | ((uint32_t)data[11] << 16) |
                                ((uint32_t)data[12] << 8) | data[13];
            }
#elif CURRENT_FIRMWARE == FIRMWARE_X
            if (len >= 21) {
                motor->kp = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) |
                                ((uint32_t)data[4] << 8) | data[5];
                motor->ki = ((uint32_t)data[6] << 24) | ((uint32_t)data[7] << 16) |
                                ((uint32_t)data[8] << 8) | data[9];
                motor->kd = ((uint32_t)data[10] << 24) | ((uint32_t)data[11] << 16) |
                                ((uint32_t)data[12] << 8) | data[13];
            }
#endif
            break;
#if CURRENT_FIRMWARE == FIRMWARE_X
        case CMD_READ_POSITION_WINDOW: if (len >= 5) motor->pos_window = (data[2] << 8) | data[3]; break;
#endif
#endif /* MOTOR_CONTROL */
#if MOTOR_PROTECTION
        case CMD_READ_PROTECT_THRESHOLD:
            if (len >= 9) {
                motor->temp_threshold = (data[2] << 8) | data[3]; motor->current_threshold = (data[4] << 8) | data[5];
                motor->protect_time = (data[6] << 8) | data[7];
            } break;
        case CMD_READ_HEARTBEAT_TIME:
            if (len >= 7) {
                motor->heartbeat_time = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) |
                                        ((uint32_t)data[4] << 8) | data[5];
            } break;
        case CMD_READ_COLLISION_ANGLE: if (len >= 5) motor->collision_angle = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_PROTECTION */
#if MOTOR_SYSTEM
        case CMD_READ_PHASE_PARAMS:
            if (len >= 6) {
                motor->phase_resistance = (data[2] << 8) | data[3]; motor->phase_inductance = (data[4] << 8) | data[5];
            } break;
        case CMD_READ_OPTION_PARAMS:
            if (len >= 5) { motor->option_params = data[2]; motor->lock_level = data[3] & 0x03; } break;
#endif /* MOTOR_SYSTEM */
#if MOTOR_CONTROL
        case CMD_READ_INTEGRAL_LIMIT:
#if CURRENT_FIRMWARE == FIRMWARE_X
            if (len >= 7) motor->integral_limit = ((uint32_t)data[2] << 24) |
                        ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[5]; break;
#endif
#endif /* MOTOR_CONTROL */
#if MOTOR_ELECTRICAL || MOTOR_MOTION || MOTOR_ENCODER || MOTOR_STATUS_FLAGS || MOTOR_HOME \
    || MOTOR_CLOG || MOTOR_CURRENT || MOTOR_DRIVER || MOTOR_COMM || MOTOR_CONTROL
        case CMD_READ_SYSTEM_STATUS:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
            if (len >= 31) {
#if MOTOR_ELECTRICAL
                motor->voltage = ((uint16_t)data[2] << 8) | (uint16_t)data[3];
                motor->phase_current = ((uint16_t)data[4] << 8) | (uint16_t)data[5];
                motor->temp = (data[26] == 0x01) ? -(int8_t)data[27] : (int8_t)data[27];
#endif
#if MOTOR_ENCODER
                motor->encoder_linear= ((uint16_t)data[6] << 8) | (uint16_t)data[7];
#endif
#if MOTOR_MOTION
                int32_t target_pos = ((int32_t)data[9] << 24) | ((int32_t)data[10] << 16) |
                                     ((int32_t)data[11] << 8)  | (int32_t)data[12];
                if (data[8] == 0x01) target_pos = -target_pos; motor->target_pos = target_pos;
                int16_t vel = ((int16_t)data[14] << 8) | (int16_t)data[15];
                if (data[13] == 0x01) vel = -vel; motor->vel = vel;
                int32_t pos = ((int32_t)data[17] << 24) | ((int32_t)data[18] << 16) |
                               ((int32_t)data[19] << 8)  | (int32_t)data[20];
                if (data[16] == 0x01) pos = -pos; motor->pos = pos;
                int32_t error = ((int32_t)data[22] << 24) | ((int32_t)data[23] << 16) |
                                 ((int32_t)data[24] << 8)  | (int32_t)data[25];
                if (data[21] == 0x01) error = -error; motor->pos_error = error;
#endif
#if MOTOR_STATUS_FLAGS
                motor->home_status = data[28];
                motor->status = data[29];
#endif
            }
#elif CURRENT_FIRMWARE == FIRMWARE_X
            if (len >= 37) {
#if MOTOR_ELECTRICAL
                motor->voltage = ((uint16_t)data[2] << 8) | (uint16_t)data[3];
                motor->bus_current = ((uint16_t)data[4] << 8) | (uint16_t)data[5];
                motor->phase_current = ((uint16_t)data[6] << 8) | (uint16_t)data[7];
                motor->temp = (data[30] == 0x01) ? -(int8_t)data[31] : (int8_t)data[31];
#endif
#if MOTOR_ENCODER
                motor->encoder_raw = ((uint16_t)data[8] << 8) | (uint16_t)data[9];
                motor->encoder_linear= ((uint16_t)data[10] << 8) | (uint16_t)data[11];
#endif
#if MOTOR_MOTION
                int32_t target_pos = ((int32_t)data[13] << 24) | ((int32_t)data[14] << 16) |
                                     ((int32_t)data[15] << 8)  | (int32_t)data[16];
                if (data[12] == 0x01) target_pos = -target_pos; motor->target_pos = target_pos;
                int16_t vel = ((int16_t)data[18] << 8) | (int16_t)data[19];
                if (data[17] == 0x01) vel = -vel; motor->vel = vel;
                int32_t pos = ((int32_t)data[21] << 24) | ((int32_t)data[22] << 16) |
                               ((int32_t)data[23] << 8)  | (int32_t)data[24];
                if (data[20] == 0x01) pos = -pos; motor->pos = pos;
                int32_t error = ((int32_t)data[26] << 24) | ((int32_t)data[27] << 16) |
                                 ((int32_t)data[28] << 8)  | (int32_t)data[29];
                if (data[25] == 0x01) error = -error; motor->pos_error = error;
#endif
#if MOTOR_STATUS_FLAGS
                motor->home_status = data[32];
                motor->status = data[33];
#endif
            }
#endif
            break;
        case CMD_READ_DRIVER_CONFIG:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
            if (len >= 33) {
#if MOTOR_DRIVER
                motor->motor_type = data[4]; motor->micro_step = data[9]; motor->interpolation = data[10];
                motor->motor_direction = data[11];
                motor->open_current = ((uint16_t)data[12] << 8) | (uint16_t)data[13];
                motor->close_current = ((uint16_t)data[14] << 8) | (uint16_t)data[15];
                motor->max_output_voltage = ((uint16_t)data[16] << 8) | (uint16_t)data[17];
#endif
#if MOTOR_COMM
                motor->uart_baudrate = data[18]; motor->can_baudrate = data[19];
                motor->motor_id = data[20]; motor->verify_mode = data[21];
                motor->response_mode = data[22];
#endif
#if MOTOR_CLOG
                motor->clog_enable = data[23];
                motor->clog_rpm = ((uint16_t)data[24] << 8) | (uint16_t)data[25];
                motor->clog_current = ((uint16_t)data[26] << 8) | (uint16_t)data[27];
                motor->clog_time = ((uint16_t)data[28] << 8) | (uint16_t)data[29];
#endif
#if MOTOR_CONTROL
                motor->pos_window = ((uint16_t)data[30] << 8) | (uint16_t)data[31];
#endif
            }
#elif CURRENT_FIRMWARE == FIRMWARE_X
            if (len >= 37) {
                motor->control_mode  = data[5]; motor->micro_step = data[10];
                motor->interpolation = data[11]; motor->motor_direction = data[19];
                motor->open_current = ((uint16_t)data[13] << 8) | (uint16_t)data[14];
                motor->close_current = ((uint16_t)data[15] << 8) | (uint16_t)data[16];
                motor->max_speed = ((uint16_t)data[17] << 8) | (uint16_t)data[18];
                motor->uart_baudrate = data[21]; motor->can_baudrate = data[22];
                motor->verify_mode = data[23]; motor->response_mode = data[24];
                motor->pos_scale = data[25];
#if MOTOR_CLOG
                motor->clog_enable = data[26];
                motor->clog_rpm = ((uint16_t)data[27] << 8) | (uint16_t)data[28];
                motor->clog_current = ((uint16_t)data[29] << 8) | (uint16_t)data[30];
                motor->clog_time = ((uint16_t)data[31] << 8) | (uint16_t)data[32];
#endif
#if MOTOR_CONTROL
                motor->pos_window = ((uint16_t)data[33] << 8) | (uint16_t)data[34];
#endif
            }
#endif
            break;
#endif /* MOTOR batch read guards */
        default: logWarning("Received motor response, but command unknown:"
                            " id:%d, cmd:%d", motor_id, cmd_code); break;
    }
}

/**
 * @brief 处理电机命令
 * @param cmd 电机命令指针
 */
void Motor_Process_Cmd(MotorCmd_t *cmd) {
    bool save = false;

    switch (cmd->op_type) {
        case OP_HEARTBEAT: ZDT_V5_Read_Sys_Params(cmd->motor_id, S_FLAG); break;
        case OP_CONTROL: Motor_Process_Ctrl(cmd->motor_id, &cmd->type.ctrl); break;
        case OP_PARAM_READ: Motor_Process_Param_Read(cmd->motor_id, &cmd->type.param); break;
        case OP_PARAM_WRITE:
            switch (cmd->type.param.type) {
#if MOTOR_DRIVER
                case PARAM_BASIC: save = cmd->type.param.p.basic.save; break;
#endif
#if MOTOR_CURRENT
                case PARAM_CURRENT: save = cmd->type.param.p.current.save; break;
#endif
#if MOTOR_CONTROL
                case PARAM_CONTROL: save = cmd->type.param.p.pid.save; break;
#endif
#if MOTOR_PROTECTION
                case PARAM_PROTECT: save = cmd->type.param.p.protect.save; break;
#endif
#if MOTOR_COMM
                case PARAM_COMM: save = cmd->type.param.p.comm.save; break;
#endif
                default: break;
            }
            Motor_Process_Param_Write(cmd->motor_id, save, &cmd->type.param); break;
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
        case CMD_ENABLE: ZDT_V5_En_Control(motor_id, ctrl->p.en.enable, ctrl->p.en.sync); break;
#endif
#if MOTOR_CMD_VELOCITY
        case CMD_VELOCITY: ZDT_V5_Vel_Control(motor_id, ctrl->p.vel.dir, ctrl->p.vel.vel,
                              ctrl->p.vel.acc, ctrl->p.vel.sync); break;
#endif
#if MOTOR_CMD_POSITION
        case CMD_POSITION: ZDT_V5_Pos_Control(motor_id, ctrl->p.pos.dir, ctrl->p.pos.vel,
                              ctrl->p.pos.acc,
#if CURRENT_FIRMWARE == FIRMWARE_X
                              ctrl->p.pos.dec,
#else
                              0,
#endif
                              (uint32_t)ctrl->p.pos.target,
                              ctrl->p.pos.mode, ctrl->p.pos.sync); break;
#endif
#if MOTOR_CMD_TORQUE && CURRENT_FIRMWARE == FIRMWARE_X
        case CMD_TORQUE: ZDT_V5_Torque_Control_With_Limit(motor_id, ctrl->p.torque.dir,
                                          ctrl->p.torque.slope, ctrl->p.torque.current,
                                          ctrl->p.torque.max_vel, ctrl->p.torque.sync); break;
#endif
#if MOTOR_CMD_STOP
        case CMD_STOP: ZDT_V5_Stop_Now(motor_id, ctrl->p.stop.sync); break;
#endif
#if MOTOR_CMD_HOME
        case CMD_HOME: ZDT_V5_Origin_Trigger_Return(motor_id, ctrl->p.home.mode,
                                        ctrl->p.home.sync); break;
#endif
        case CMD_SYNC: ZDT_V5_Synchronous_motion(0); break;
#if MOTOR_CMD_FAST
        case CMD_FAST_SET: ZDT_V5_Fast_Set_Param(motor_id, ctrl->p.fast_set.vel, ctrl->p.fast_set.acc,
#if CURRENT_FIRMWARE == FIRMWARE_X
                                ctrl->p.fast_set.dec, ctrl->p.fast_set.max_current,
#else
                                0, 0,
#endif
                                ctrl->p.fast_set.mode, ctrl->p.fast_set.sync); break;
        case CMD_FAST_SEND: ZDT_V5_Fast_Send_Pos(motor_id, ctrl->p.fast_send.pos); break;
#endif /* MOTOR_CMD_FAST */
        case CMD_NONE: default: logWarning("Unknown control command type: %d", ctrl->type); break;
    }
}

/**
 * @brief 处理电机参数读取命令
 * @param motor_id 电机ID
 * @param param 电机参数指针
 */
static void Motor_Process_Param_Read(uint8_t motor_id, MotorParam_t *param) {
    switch (param->type) {
#if MOTOR_DRIVER
        case PARAM_BASIC: ZDT_V5_Read_Option_Params(motor_id); break;
#endif
#if MOTOR_ELECTRICAL
        case PARAM_ELECTRICAL:
            ZDT_V5_Read_Sys_Params(motor_id, S_VBUS); osDelay(2);
#if CURRENT_FIRMWARE == FIRMWARE_X
            ZDT_V5_Read_Sys_Params(motor_id, S_CBUS); osDelay(2);
#endif
            ZDT_V5_Read_Sys_Params(motor_id, S_CPHA); osDelay(2);
            ZDT_V5_Read_Sys_Params(motor_id, S_TEMP); osDelay(2);
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
            ZDT_V5_Read_Sys_Params(motor_id, S_VBAT);
#endif
            break;
#endif /* MOTOR_ELECTRICAL */
#if MOTOR_MOTION
        case PARAM_MOTION:
            ZDT_V5_Read_Sys_Params(motor_id, S_VEL); osDelay(2);
            ZDT_V5_Read_Sys_Params(motor_id, S_CPOS); osDelay(2);
            ZDT_V5_Read_Sys_Params(motor_id, S_TPOS); osDelay(2);
            ZDT_V5_Read_Sys_Params(motor_id, S_SPOS); osDelay(2);
            ZDT_V5_Read_Sys_Params(motor_id, S_PERR); osDelay(2);
#if CURRENT_FIRMWARE == FIRMWARE_X
            ZDT_V5_Read_Sys_Params(motor_id, S_CLKI);
#endif
            break;
#endif /* MOTOR_MOTION */
#if MOTOR_ENCODER
        case PARAM_ENCODER:
#if CURRENT_FIRMWARE == FIRMWARE_X
            ZDT_V5_Read_Sys_Params(motor_id, S_ENCL); break;
#endif
#endif /* MOTOR_ENCODER */
#if MOTOR_STATUS_FLAGS
        case PARAM_STATUS:
#if CURRENT_FIRMWARE == FIRMWARE_X
            ZDT_V5_Read_Sys_Params(motor_id, S_FLAG); osDelay(2);
#endif
            ZDT_V5_Read_Sys_Params(motor_id, S_OFLAG); osDelay(2);
            ZDT_V5_Read_Sys_Params(motor_id, S_PIN); break;
#endif /* MOTOR_STATUS_FLAGS */
#if MOTOR_SYSTEM
        case PARAM_SYSTEM:
            ZDT_V5_Read_Version_Info(motor_id); osDelay(2);
            ZDT_V5_Read_Phase_Params(motor_id); osDelay(2);
            ZDT_V5_Read_Option_Params(motor_id); osDelay(2);
            ZDT_V5_Read_Motor_ID(motor_id); break;
#endif /* MOTOR_SYSTEM */
#if MOTOR_CONTROL
        case PARAM_CONTROL:
            ZDT_V5_Read_PID_Params(motor_id); osDelay(2);
#if CURRENT_FIRMWARE == FIRMWARE_X
            ZDT_V5_Read_Pos_Window(motor_id); osDelay(2);
            ZDT_V5_Read_Integral_Limit(motor_id);
#endif
            break;
#endif /* MOTOR_CONTROL */
#if MOTOR_PROTECTION
        case PARAM_PROTECT:
            ZDT_V5_Read_Otocp(motor_id); osDelay(2);
            ZDT_V5_Read_Heart_Protect(motor_id); osDelay(2);
            ZDT_V5_Read_Collision_Angle(motor_id); break;
#endif /* MOTOR_PROTECTION */
#if MOTOR_CLOG
        case PARAM_CLOG: ZDT_V5_Read_Batch_Config(motor_id); break;
#endif /* MOTOR_CLOG */
#if MOTOR_HOME
        case PARAM_HOME: ZDT_V5_Origin_Read_Params(motor_id); break;
#endif /* MOTOR_HOME */
#if MOTOR_DRIVER
        case PARAM_DRIVER: ZDT_V5_Read_Batch_Config(motor_id); break;
#endif /* MOTOR_DRIVER */
#if MOTOR_COMM
        case PARAM_COMM: ZDT_V5_Read_Batch_Config(motor_id); break;
#endif /* MOTOR_COMM */
        case PARAM_NONE: default: logWarning("Unknown param read type: %d", param->type); break;
    }
}

/**
 * @brief 处理电机参数写入命令
 * @param motor_id 电机ID
 * @param save 是否保存参数
 * @param param 电机参数指针
 */
static void Motor_Process_Param_Write(uint8_t motor_id, bool save, MotorParam_t *param) {
    switch (param->type) {
#if MOTOR_DRIVER && CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case PARAM_BASIC:
            ZDT_V5_Modify_MicroStep(motor_id, save, param->p.basic.micro_step); osDelay(2);
            ZDT_V5_Modify_Motor_Type(motor_id, save, param->p.basic.motor_type); osDelay(2);
            ZDT_V5_Modify_Firmware_Type(motor_id, save, param->p.basic.firmware); osDelay(2);
            ZDT_V5_Modify_Ctrl_Mode(motor_id, save, param->p.basic.ctrl_mode); osDelay(2);
            ZDT_V5_Modify_Motor_Dir(motor_id, save, param->p.basic.dir); break;
#endif
#if MOTOR_CURRENT
        case PARAM_CURRENT:
            ZDT_V5_Modify_OM_mA(motor_id, save, param->p.current.open_current); osDelay(2);
            ZDT_V5_Modify_FOC_mA(motor_id, save, param->p.current.close_current); break;
#endif
#if MOTOR_CONTROL
        case PARAM_CONTROL:
            ZDT_V5_Modify_PID_Params(motor_id, save, param->p.pid.kp, param->p.pid.ki, param->p.pid.kd); osDelay(2);
            ZDT_V5_Modify_Integral_Limit(motor_id, save, param->p.pid.integral_limit); break;
#endif
#if MOTOR_PROTECTION && CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case PARAM_PROTECT:
            ZDT_V5_Modify_Otocp(motor_id, save, param->p.protect.temp_threshold,
                                param->p.protect.current_threshold, param->p.protect.protect_time); osDelay(2);
            ZDT_V5_Modify_Pos_Window(motor_id, save, param->p.protect.pos_window); osDelay(2);
            ZDT_V5_Modify_Heart_Protect(motor_id, save, param->p.protect.heartbeat_time); break;
#endif
#if MOTOR_COMM
        case PARAM_COMM:
            ZDT_V5_Modify_Comm_Params(motor_id, param->p.comm.save, param->p.comm.uart_baudrate,
                                      param->p.comm.can_baudrate, param->p.comm.verify_mode,
                                      param->p.comm.response_mode); break;
#endif
        case PARAM_NONE: default: logWarning("Unknown param write type: %d", param->type); break;
    }
}
