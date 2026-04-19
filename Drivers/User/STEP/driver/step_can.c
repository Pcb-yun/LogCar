/**
 * @file step_can.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief CAN处理逻辑层源文件
 */

#include "step_can.h"
#include "ZDT_V5_Driver.h"
#include "log.h"
#include "cmsis_os.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t buffer[64];
    uint8_t length;
    uint8_t current_packet;
    uint32_t start_time;
    uint8_t motor_id;
    uint8_t cmd_code;
    bool is_multi_packet;
} MultiPacket_Info_t;

MotorStatusShared_t g_motor_status;

static MultiPacket_Info_t multi_packet;
static uint32_t last_check_time = 0;
static void Motor_CAN_Receive(uint8_t *data, uint8_t len);
static void Motor_Process_Ctrl(uint8_t motor_id, MotorCtrl_t *ctrl);
static void Motor_Process_Param_Read(uint8_t motor_id, MotorParam_t *param);
static void Motor_Process_Param_Write(uint8_t motor_id, uint8_t save, MotorParam_t *param);

/**
 * @brief 重置多包信息
 */
static void reset_multi_packet(void) {
    memset(&multi_packet, 0, sizeof(MultiPacket_Info_t));
}

/**
 * @brief 处理多包数据
 * @param msg CAN接收消息指针
 * @return true 成功, false 失败
 */
bool process_multi_packet(CAN_Rx_Message_t *msg) {
    uint8_t motor_id = (uint8_t)(msg->ExtId >> 8);
    uint8_t packet = (uint8_t)(msg->ExtId & 0xFF);
    uint8_t cmd_code = msg->data[0];

    if (packet == 0) {
        reset_multi_packet();
        multi_packet.motor_id = motor_id;
        multi_packet.cmd_code = cmd_code;
        multi_packet.current_packet = 0;
        multi_packet.start_time = HAL_GetTick();

        uint8_t data_len = msg->DLC - 1;
        if (data_len > 0) {
            memcpy(&multi_packet.buffer[0], &msg->data[1], data_len);
            multi_packet.length = data_len;
        }

        if (msg->DLC <= 8) {
            uint8_t full_data[9];
            full_data[0] = motor_id;
            memcpy(&full_data[1], msg->data, msg->DLC);
            Motor_CAN_Receive(full_data, msg->DLC + 1);
            return true;
        }

        multi_packet.is_multi_packet = true;
    } else {
        if (multi_packet.is_multi_packet &&
            multi_packet.motor_id == motor_id &&
            multi_packet.cmd_code == cmd_code &&
            multi_packet.current_packet + 1 == packet) {

            uint8_t data_len = msg->DLC - 1;
            if (data_len > 0 && multi_packet.length + data_len < sizeof(multi_packet.buffer)) {
                memcpy(&multi_packet.buffer[multi_packet.length], &msg->data[1], data_len);
                multi_packet.length += data_len;
                multi_packet.current_packet = packet;
            }

            if (msg->DLC < 8) {
                uint8_t full_data[65];
                full_data[0] = motor_id;
                full_data[1] = cmd_code;
                memcpy(&full_data[2], multi_packet.buffer, multi_packet.length);
                Motor_CAN_Receive(full_data, 2 + multi_packet.length);
                reset_multi_packet();
                return true;
            }
        } else {
            reset_multi_packet();
        }
    }

    if (multi_packet.is_multi_packet &&
        HAL_GetTick() - last_check_time >= PACKET_TIMEOUT) {
        if (HAL_GetTick() - multi_packet.start_time > PACKET_TIMEOUT) {
            reset_multi_packet();
        }
        last_check_time = HAL_GetTick();
    }

    return false;
}

/**
 * @brief 发送CAN消息
 * @param cmd CAN消息指针
 * @param len CAN消息长度
 */
void Motor_CAN_Send(uint8_t *cmd, uint8_t len) {
    uint8_t motor_id = cmd[0];
    uint8_t *data_start = &cmd[1];
    uint8_t data_len = len - 1;

    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;

    tx_header.IDE = CAN_ID_EXT;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.TransmitGlobalTime = DISABLE;

    uint8_t packet = 0;
    uint8_t offset = 0;

    while (offset < data_len) {
        uint8_t chunk_len = (data_len - offset > 8) ? 8 : (data_len - offset);

        tx_header.ExtId = ((uint32_t)motor_id << 8) | packet;
        tx_header.DLC = chunk_len;

        if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, &data_start[offset], &tx_mailbox) != HAL_OK) {
            logWarning("AddTxMessage failed");
            return;
        }

        uint32_t start_time = HAL_GetTick();
        while (HAL_CAN_IsTxMessagePending(&hcan1, tx_mailbox)) {
            if (HAL_GetTick() - start_time > PACKET_TIMEOUT) {
                logWarning("Send timeout");
                return;
            }
            osDelay(1);
        }

        offset += chunk_len;
        packet++;
    }
}

/**
 * @brief 处理CAN接收消息
 * @param data CAN接收消息指针
 * @param len CAN接收消息长度
 */
static void Motor_CAN_Receive(uint8_t *data, uint8_t len) {
    uint8_t motor_id = data[0];
    uint8_t cmd_code = data[1];

    MotorStatus_t *motor = &g_motor_status.motors[motor_id];
    motor->motor_id = motor_id;

    switch (cmd_code) {
#if MOTOR_STATUS_ELECTRICAL
        case CMD_READ_BUS_VOLTAGE:
            if (len >= 5) motor->voltage = (data[2] << 8) | data[3]; break;
        case CMD_READ_BUS_CURRENT:
            if (len >= 5) motor->current = (data[2] << 8) | data[3]; break;
        case CMD_READ_PHASE_CURRENT:
            if (len >= 5) motor->phase_current = (data[2] << 8) | data[3]; break;
        case CMD_READ_TEMPERATURE:
            if (len >= 5) {
                int8_t temp = data[3];
                if (data[2] == 0x00) {
                    temp = -temp;
                }
                motor->temp = (uint8_t)abs(temp);
            } break;
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case CMD_READ_BATTERY_VOLTAGE:
            if (len >= 5) motor->battery_voltage = (data[2] << 8) | data[3]; break;
#endif
#endif /* MOTOR_STATUS_ELECTRICAL */
#if MOTOR_STATUS_MOTION
        case CMD_READ_SPEED:
            if (len >= 6) {
                int16_t vel = (data[3] << 8) | data[4];
                if (data[2] == 0x01) {
                    vel = -vel;
                }
#if CURRENT_FIRMWARE == FIRMWARE_X
                vel = vel / 10;
#endif
                motor->vel = vel;
            } break;
        case CMD_READ_POSITION:
            if (len >= 8) {
                int32_t pos = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) {
                    pos = -pos;
                }
#if CURRENT_FIRMWARE == FIRMWARE_X
                pos = pos / 10;
#endif
                motor->pos = pos;
            } break;
        case CMD_READ_TARGET_POSITION:
            if (len >= 8) {
                int32_t pos = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) {
                    pos = -pos;
                }
#if CURRENT_FIRMWARE == FIRMWARE_X
                pos = pos / 10;
#endif
                motor->target_pos = pos;
            } break;
        case CMD_READ_SET_POSITION:
            if (len >= 8) {
                int32_t pos = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) {
                    pos = -pos;
                }
#if CURRENT_FIRMWARE == FIRMWARE_X
                pos = pos / 10;
#endif
                motor->set_pos = pos;
            } break;
        case CMD_READ_POSITION_ERROR:
            if (len >= 8) {
                int32_t error = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) {
                    error = -error;
                }
#if CURRENT_FIRMWARE == FIRMWARE_X
                error = error / 100;
#endif
                motor->pos_error = error;
            } break;
        case CMD_READ_INPUT_PULSES:
            if (len >= 8) {
                int32_t pulses = (data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6];
                if (data[2] == 0x01) {
                    pulses = -pulses;
                }
                motor->input_pulses = pulses;
            } break;
#endif /* MOTOR_STATUS_MOTION */
#if MOTOR_STATUS_ENCODER
        case CMD_READ_ENCODER_VALUE:
            if (len >= 5) motor->encoder_value = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_ENCODER */
#if MOTOR_STATUS_STATUS
        case CMD_READ_MOTOR_STATUS:
            if (len >= 4) motor->status = data[2]; break;
        case CMD_READ_HOME_STATUS:
            if (len >= 4) motor->home_status = data[2]; break;
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case CMD_READ_STATUS_FLAGS:
            if (len >= 5) {
                motor->home_status = data[2];
                motor->status = data[3];
            } break;
        case CMD_READ_PIN_STATUS:
            if (len >= 4) motor->pin_status = data[2]; break;
#endif
#endif /* MOTOR_STATUS_STATUS */
#if MOTOR_STATUS_SYSTEM
        case CMD_READ_VERSION_INFO:
            if (len >= 9) {
                motor->firmware_version = (data[2] << 8) | data[3];
                motor->hardware_version = data[6];
            } break;
#endif /* MOTOR_STATUS_SYSTEM */
#if MOTOR_STATUS_CONTROL
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
                motor->ki = ((uint32_t)data[10] << 24) | ((uint32_t)data[11] << 16) |
                                ((uint32_t)data[12] << 8) | data[13];
                motor->kd = 0;
            }
#endif
            break;
        case CMD_READ_POSITION_WINDOW:
            if (len >= 5) motor->pos_window = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_CONTROL */
#if MOTOR_STATUS_PROTECTION
        case CMD_READ_PROTECT_THRESHOLD:
            if (len >= 7) {
                motor->temp_threshold = (data[2] << 8) | data[3];
                motor->current_threshold = (data[4] << 8) | data[5];
            } break;
        case CMD_READ_HEARTBEAT_TIME:
            if (len >= 7) {
                motor->heartbeat_time = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) |
                                        ((uint32_t)data[4] << 8) | data[5];
            } break;
        case CMD_READ_COLLISION_ANGLE:
            if (len >= 5) motor->collision_angle = (data[2] << 8) | data[3]; break;
#endif /* MOTOR_STATUS_PROTECTION */
#if MOTOR_STATUS_SYSTEM
        case CMD_READ_PHASE_PARAMS:
            if (len >= 6) {
                motor->phase_resistance = (data[2] << 8) | data[3];
                motor->phase_inductance = (data[4] << 8) | data[5];
            } break;
        case CMD_READ_OPTION_PARAMS:
            if (len >= 4) motor->option_params = data[2]; break;
        case CMD_READ_MOTOR_ID:
            if (len >= 4) motor->motor_id = data[2]; break;
#endif /* MOTOR_STATUS_SYSTEM */
#if MOTOR_STATUS_CONTROL
        case CMD_READ_INTEGRAL_LIMIT:
            if (len >= 7) {
                motor->integral_limit = ((uint32_t)data[2] << 24) | ((uint32_t)data[3] << 16) |
                                        ((uint32_t)data[4] << 8) | data[5];
            } break;
#endif /* MOTOR_STATUS_CONTROL */
#if MOTOR_STATUS_BATCH
        case CMD_READ_SYSTEM_STATUS:
#if CURRENT_FIRMWARE == FIRMWARE_EMM
            if (len >= 31) {
                motor->voltage = (data[2] << 8) | data[3];
                motor->phase_current = (data[4] << 8) | data[5];
                motor->encoder_value = (data[6] << 8) | data[7];

                int32_t target_pos = (data[9] << 24) | (data[10] << 16) | (data[11] << 8) | data[12];
                if (data[8] == 0x01) {
                    target_pos = -target_pos;
                }
                motor->target_pos = target_pos;

                int16_t vel = (data[14] << 8) | data[15];
                if (data[13] == 0x01) {
                    vel = -vel;
                }
                motor->vel = vel;

                int32_t pos = (data[17] << 24) | (data[18] << 16) | (data[19] << 8) | data[20];
                if (data[16] == 0x01) {
                    pos = -pos;
                }
                motor->pos = pos;

                int32_t error = (data[22] << 24) | (data[23] << 16) | (data[24] << 8) | data[25];
                if (data[21] == 0x01) {
                    error = -error;
                }
                motor->pos_error = error;

                motor->home_status = data[26];
                motor->status = data[27];
            }
#elif CURRENT_FIRMWARE == FIRMWARE_X
            if (len >= 37) {
                motor->voltage = (data[2] << 8) | data[3];
                motor->current = (data[4] << 8) | data[5];
                motor->phase_current = (data[6] << 8) | data[7];
                motor->encoder_value = (data[10] << 8) | data[11];

                int32_t target_pos = (data[13] << 24) | (data[14] << 16) | (data[15] << 8) | data[16];
                if (data[12] == 0x01) {
                    target_pos = -target_pos;
                }
                motor->target_pos = target_pos;

                int16_t vel = (data[18] << 8) | data[19];
                if (data[17] == 0x01) {
                    vel = -vel;
                }
                motor->vel = vel;

                int32_t pos = (data[22] << 24) | (data[23] << 16) | (data[24] << 8) | data[25];
                if (data[21] == 0x01) {
                    pos = -pos;
                }
                motor->pos = pos;

                int32_t error = (data[28] << 24) | (data[29] << 16) | (data[30] << 8) | data[31];
                if (data[27] == 0x01) {
                    error = -error;
                }
                motor->pos_error = error;

                motor->temp = data[32];
                motor->home_status = data[33];
                motor->status = data[34];
            }
#endif
            break;
        case CMD_READ_DRIVER_CONFIG:
            if (len >= 33) {
                motor->micro_step = data[7];
                motor->open_current = (data[8] << 8) | data[9];
                motor->close_current = (data[10] << 8) | data[11];
#if MOTOR_STATUS_COMM
                motor->uart_baudrate = data[13];
                motor->can_baudrate = data[14];
                motor->verify_mode = data[15];
                motor->response_mode = data[16];
#endif
            } break;
#endif /* MOTOR_STATUS_BATCH */
    }
}

/**
 * @brief 处理电机命令
 * @param cmd 电机命令指针
 */
void Motor_Process_Cmd(MotorCmd_t *cmd) {
    uint8_t save = 0;

    switch (cmd->op_type) {
        case OP_CONTROL: Motor_Process_Ctrl(cmd->motor_id, &cmd->type.ctrl); break;
        case OP_PARAM_READ: Motor_Process_Param_Read(cmd->motor_id, &cmd->type.param); break;
        case OP_PARAM_WRITE:
            switch (cmd->type.param.type) {
                case PARAM_BASIC: save = cmd->type.param.p.basic.save; break;
                case PARAM_CURRENT: save = cmd->type.param.p.current.save; break;
                case PARAM_PID: save = cmd->type.param.p.pid.save; break;
                case PARAM_PROTECT: save = cmd->type.param.p.protect.save; break;
                case PARAM_COMM: save = cmd->type.param.p.comm.save; break;
            }
            Motor_Process_Param_Write(cmd->motor_id, save, &cmd->type.param); break;
        case OP_NONE:
        default: logWarning("Unknown or none operation type: %d", cmd->op_type); break;
    }
}

static void Motor_Process_Ctrl(uint8_t motor_id, MotorCtrl_t *ctrl) {
    switch (ctrl->type) {
#if MOTOR_CMD_ENABLE
        case CMD_ENABLE: ZDT_V5_En_Control(motor_id, ctrl->p.en.enable, ctrl->p.en.sync); break;
#endif
#if MOTOR_CMD_VELOCITY
        case CMD_VELOCITY: ZDT_V5_Vel_Control(motor_id, ctrl->p.vel.dir, ctrl->p.vel.vel,
                              ctrl->p.vel.acc, 0); break;
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
        case MOTOR_CMD_TORQUE: ZDT_V5_Torque_Control_With_Limit(motor_id, ctrl->p.torque.dir,
                                          ctrl->p.torque.slope, ctrl->p.torque.current,
                                          ctrl->p.torque.max_vel, false); break;
#endif
#if MOTOR_CMD_STOP
        case CMD_STOP: ZDT_V5_Stop_Now(motor_id, ctrl->p.stop.stop); break;
#endif
#if MOTOR_CMD_HOME
        case CMD_HOME: ZDT_V5_Origin_Trigger_Return(motor_id, ctrl->p.home.mode,
                                        ctrl->p.home.sync_flag); break;
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
        case CMD_NONE:
        default: logWarning("Unknown control command type: %d", ctrl->type); break;
    }
}

static void Motor_Process_Param_Read(uint8_t motor_id, MotorParam_t *param) {
    switch (param->type) {
#if MOTOR_PARAM_BASIC && CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case PARAM_BASIC: ZDT_V5_Read_Opt_Param_Sta(motor_id); break;
#endif
#if MOTOR_PARAM_CURRENT
        case PARAM_CURRENT: ZDT_V5_Read_PID_Params(motor_id); break;
#endif
#if MOTOR_PARAM_PID
        case PARAM_PID: ZDT_V5_Read_PID_Params(motor_id); break;
#endif
#if MOTOR_PARAM_PROTECT && CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case PARAM_PROTECT:
            ZDT_V5_Read_Otocp(motor_id);
            ZDT_V5_Read_Pos_Window(motor_id); break;
#endif
#if MOTOR_PARAM_COMM
        case PARAM_COMM: ZDT_V5_Read_Comm_Params(motor_id); break;
#endif
        case PARAM_BATCH_STATUS: ZDT_V5_Read_Batch_Status(motor_id); break;
        case PARAM_BATCH_CONFIG: ZDT_V5_Read_Batch_Config(motor_id); break;
        case PARAM_NONE:
        default: logWarning("Unknown param read type: %d", param->type); break;
    }
}

static void Motor_Process_Param_Write(uint8_t motor_id, uint8_t save, MotorParam_t *param) {
    bool svF = save;

    switch (param->type) {
#if MOTOR_PARAM_BASIC && CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case PARAM_BASIC:
            ZDT_V5_Modify_MicroStep(motor_id, svF, param->p.basic.micro_step);
            ZDT_V5_Modify_Motor_Type(motor_id, svF, param->p.basic.motor_type);
            ZDT_V5_Modify_Firmware_Type(motor_id, svF, param->p.basic.firmware);
            ZDT_V5_Modify_Ctrl_Mode(motor_id, svF, param->p.basic.ctrl_mode);
            ZDT_V5_Modify_Motor_Dir(motor_id, svF, param->p.basic.dir); break;
#endif
#if MOTOR_PARAM_CURRENT
        case PARAM_CURRENT:
            ZDT_V5_Modify_OM_mA(motor_id, svF, param->p.current.open_current);
            ZDT_V5_Modify_FOC_mA(motor_id, svF, param->p.current.close_current); break;
#endif
#if MOTOR_PARAM_PID
        case PARAM_PID:
            ZDT_V5_Modify_PID_Params(motor_id, svF, param->p.pid.kp, param->p.pid.ki, param->p.pid.kd);
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
            ZDT_V5_Modify_Integral_Limit(motor_id, svF, param->p.pid.integral_limit);
#endif
            break;
#endif
#if MOTOR_PARAM_PROTECT && CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        case PARAM_PROTECT:
            ZDT_V5_Modify_Otocp(motor_id, svF, param->p.protect.temp_threshold,
                                param->p.protect.current_threshold, param->p.protect.protect_time);
            ZDT_V5_Modify_Pos_Window(motor_id, svF, param->p.protect.pos_window);
            ZDT_V5_Modify_Heart_Protect(motor_id, svF, param->p.protect.heartbeat_time); break;
#endif
#if MOTOR_PARAM_COMM
        case PARAM_COMM:
            ZDT_V5_Modify_Comm_Params(motor_id, param->p.comm.save, param->p.comm.uart_baudrate,
                                      param->p.comm.can_baudrate, param->p.comm.verify_mode,
                                      param->p.comm.response_mode); break;
#endif
        case PARAM_NONE:
        default: logWarning("Unknown param write type: %d", param->type); break;
    }
}
