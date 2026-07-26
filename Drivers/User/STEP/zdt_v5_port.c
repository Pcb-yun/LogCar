/**
 * @file zdt_v5_port.c
 * @brief 张大头V5步进电机移植接口
 *
 * zdt-v5-driver - 张大头V5步进电机通用驱动
 * Copyright (C) 2024-2026  Pcb-yun
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * 将本文件复制到工程中，并改名为 zdt_v5_port.c
 */

#include "zdt_v5_port.h"
#include "zdt_v5_drv.h"
#include "zdt_v5_engine.h"
#include "shell.h"
#include "log.h"
#include "shell_cmd_group.h"
#include "cmsis_os.h"
#include "usart.h"
#include "Events.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern osMutexId_t Motor_MutexHandle;
extern osMessageQueueId_t Usart6_Rx_DataHandle;
extern osMessageQueueId_t MotorCmdsHandle;
MotorStatus_t *g_motor_status[ZDT_STEP_NUM];
static bool is_init = false;    // 电机模块是否初始化
uint32_t step_update_time = 50; // 电机状态更新时间间隔（毫秒）

/**
 * @brief 串口发送函数（必须）
 * @param cmd 命令指针
 * @param len 命令长度
 */
void zdt_v5_port_send(uint8_t *cmd, uint8_t len) {
    osEventFlagsClear(System_StatusHandle, UART6_TX_IDLE);
    if (HAL_UART_Transmit_DMA(&huart6, cmd, len) != HAL_OK) {
        logError("UART DMA transmit failed"); return;
    }
    osEventFlagsWait(System_StatusHandle, UART6_TX_IDLE, osFlagsWaitAny, osWaitForever);
}

/**
 * @brief 初始化电机模块
 */
bool Motor_Init(void) {
    extern uint8_t rx6Buffer[USART6_RX_BUF_SIZE];

    for (uint8_t i = 0; i < ZDT_STEP_NUM; i++) {
        g_motor_status[i] = pvPortMalloc(sizeof(MotorStatus_t));
        if (!g_motor_status[i]) {
            for (uint8_t j = 0; j < i; j++) {
                vPortFree(g_motor_status[j]);
            }
            return false;
        }
        memset(g_motor_status[i], 0, sizeof(MotorStatus_t));
        if (!ZDT_V5_Register_Motor(i + 1, g_motor_status[i])) {
            for (uint8_t j = 0; j < i; j++) {
                vPortFree(g_motor_status[j]);
            }
            return false;
        }
    }

    MX_USART6_UART_Init();

	for (uint8_t i = 0; i < ZDT_STEP_NUM; i++) {
#if USE_HEARTBEAT
        ZDT_V5_Modify_Heart_Protect(i + 1, false, step_update_time + 100);
#else
#if MOTOR_BROADCAST_READ_ID
		ZDT_V5_Read_Device_Info_Params(0, I_ID);
#else
		is_init = true;
		logWarning("MOTOR_BROADCAST_READ_ID is not enabled, the motor state cannot be detected");
#endif
#endif /* USE_HEARTBEAT */
        if (HAL_UART_Receive(&huart6, rx6Buffer, 4, 10) == HAL_OK) {
            is_init |= true;
        }
    }

    if (is_init) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx6Buffer, USART6_RX_BUF_SIZE);
    }

    return is_init;
}

/**
 * @brief 发送电机控制命令到队列
 * @param cmd 电机控制命令指针
 * @return true 成功发送，false 失败
 */
bool Motor_Send_Cmd(MotorCmd_t *cmd) {
    if (!is_init) return false;
    osStatus_t status = osMessageQueuePut(MotorCmdsHandle, cmd, 0, 10);
    return (status == osOK);
}

/**
 * @brief 电机控制任务
 */
void Motor_Ctrl_Task(void *argument) {
    (void)argument;
    MotorCmd_t cmd;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!is_init)  {
        osMessageQueueDelete(MotorCmdsHandle);
        vTaskDelete(NULL);
    }

    for(;;) {
        if (osMessageQueueGet(MotorCmdsHandle, &cmd, NULL, osWaitForever) == osOK) {
            ZDT_V5_Process_Cmd(&cmd);
            osDelay(2);     // 防止粘包
        }
    }
}

/**
 * @brief 电机信息处理任务
 */
void Motor_Receive_Task(void *argument) {
    (void)argument;
    Usart6_RxBuf_t rxBuf;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!is_init) {
        osMessageQueueDelete(Usart6_Rx_DataHandle);
        vTaskDelete(NULL);
    }

    for(;;) {
        if (osMessageQueueGet(Usart6_Rx_DataHandle, &rxBuf, NULL, osWaitForever) == osOK) {
            if (osMutexAcquire(Motor_MutexHandle, osWaitForever) == osOK) {
                ZDT_V5_Receive(rxBuf.data, rxBuf.len);
                osMutexRelease(Motor_MutexHandle);
            }
        }
    }
}

/**
 * @brief 电机状态更新任务
 */
void Motor_Update_Task(void *argument) {
	(void)argument;

	osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
	if (!is_init) vTaskDelete(NULL);

	extern void Motor_Update(MotorCmd_t *cmd, uint8_t motor_id);
	MotorCmd_t cmd;

	for(;;) {
		osDelay(step_update_time);
		for(uint8_t i = 0; i < 4; i++) {
			Motor_Update(&cmd, i + 1);
		}
	}
}

#if MOTOR_TRIGGER_RESET_POS
/**
 * @brief 电机位置清零
 */
void Motor_zero(uint8_t motor_id) {
	MotorCmd_t cmd;
	cmd.op_type = OP_TRIGGER;
	cmd.motor_id = motor_id;

	MotorTrigger_t trigger;
	trigger.type = TRIG_RESET_POS;
	cmd.type.trigger = trigger;
	Motor_Send_Cmd(&cmd);
}
#endif /* MOTOR_TRIGGER_RESET_POS */

#if MOTOR_BROADCAST_READ_ID
/**
 * @brief 检查电机是否在线
 * @param motor_id 电机ID
 * @return true 电机在线，false 电机不在线
 */
static bool Motor_isonline(uint8_t motor_id) {
	MotorStatus_t *motor = g_motor_status[motor_id - 1];
	motor->is_online = false;
	ZDT_V5_Read_Device_Info_Params(0, I_ID);
	osDelay(step_update_time + 50);
	return motor->is_online;
}
#endif /* MOTOR_BROADCAST_READ_ID */

#if MOTOR_CMD_ENABLE
/**
 * @brief 紧急停止
 */
static void Step_ES(int argc, char *argv[]) {
	if (argc == 2 && strcmp(argv[1], "r") == 0) {
		ZDT_V5_En_Control(0, true, false);
		logPrintln("Motor Enabled");
	} else {
		ZDT_V5_En_Control(0, false, false);
		logPrintln("Motor Emergency Stop\r\nUse: es r to reset");
	}
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
es, Step_ES, Emergency Stop);

/**
 * @brief 紧急停止按键导出
 */
static void ES_Key(void) {
	Step_ES(0, NULL);
}
SHELL_EXPORT_KEY(SHELL_CMD_PERMISSION(0), 0x18000000, ES_Key, ^X);
#endif /* MOTOR_CMD_ENABLE */

#if MOTOR_VELOCITY_MODE
/**
 * @brief 单个电机速度控制
 */
static void Motor_vel_Shell(int argc, char *argv[]) {
	if (!is_init) {
		logWarning("Motor module not initialized"); return;
	}

	if (argc != 5) {
		logPrintln("Usage: vel [id] [dir] [vel] [acc]");
		return;
	}

	uint8_t motor_id = atoi(argv[1]);
	uint8_t dir = atoi(argv[2]);
	uint16_t vel = atoi(argv[3]);
	uint16_t acc = atoi(argv[4]);

	MotorCmd_t cmd;
	cmd.op_type = OP_CONTROL;
	cmd.motor_id = motor_id;

	MotorCtrl_t ctrl;
	ctrl.type = CTRL_VEL;
	ctrl.p.vel.dir = dir;
	ctrl.p.vel.vel = vel;
	ctrl.p.vel.acc = acc;
	ctrl.p.vel.sync = false;
	cmd.type.ctrl = ctrl;

	if (Motor_Send_Cmd(&cmd)) {
		logPrintln("Velocity cmd sent to motor %d: dir=%d, vel=%d, acc=%d", motor_id, dir, vel, acc);
	} else {
		logPrintln("Failed to send velocity command");
	}
}
#endif /* MOTOR_VELOCITY_MODE */

static void Tool_Help(void) {
	logPrintln("Usage: tool COMMAND [value...]\r\n"
			   "\r\n"
			   "commands:\r\n"
			   "  cmd       Send Motor Command\r\n"
#if MOTOR_BROADCAST_READ_ID
			   "  online    Check Motor Online\r\n"
#endif
#if MOTOR_CMD_ENABLE
			   "  en        Enable/Disable Motor\r\n"
#endif
#if MOTOR_VELOCITY_MODE
			   "  found     Find Motor\r\n"
#endif
#if MOTOR_CMD_STOP
			   "  stop      Stop Motor\r\n"
#endif
#if MOTOR_TRIGGER_RESET_POS
			   "  zero      Reset Motor Position to Zero\r\n"
#endif
#if MOTOR_HOME_TRIGGER
			   "  home      Trigger Motor Homing\r\n"
#endif
#if MOTOR_DRIVER_POS_WINDOW
			   "  window    Set Motor Window\r\n"
#endif
#if MOTOR_PID_WRITE
			   "  pid       Set motor PID\r\n"
#endif
#if MOTOR_INTEGRAL_LIMIT_WRITE
			   "  ilimit    Set motor integral limit\r\n"
#endif
#if MOTOR_TRIGGER_ENCODER_CALIB
			   "  cal       Calibrate Motor Encoder\r\n"
#endif
#if MOTOR_TRIGGER_RESTART
			   "  rstrt     Restart Motor\r\n"
#endif
			   "  time      View or Set Update Time\r\n");
            }

#if MOTOR_POS_MODE
/**
 * @brief 单个电机位置控制
 */
static void Motor_pos_Shell(int argc, char *argv[]) {
	if (!is_init) {
		logWarning("Motor module not initialized"); return;
	}

#if CURRENT_FIRMWARE == FIRMWARE_X
	if (argc != 8) {
		logPrintln("Usage: pos [id] [dir] [vel] [acc] [angle] [mode] [dec]\r\n"
				"mode: 0-relative to last target, 1-absolute, 2-relative to current");
		return;
	}
#elif CURRENT_FIRMWARE == FIRMWARE_EMM
if (argc != 7) {
    logPrintln("Usage: pos [id] [dir] [vel] [acc] [target] [mode]\r\n"
            "mode: 0-relative to last target, 1-absolute, 2-relative to current");
    return;
}
#endif

	uint8_t motor_id = atoi(argv[1]);
	uint8_t dir = atoi(argv[2]);
	uint16_t vel = atoi(argv[3]);
	uint16_t acc = atoi(argv[4]);
	int32_t target_angle = atoi(argv[5]);
	uint8_t mode = atoi(argv[6]);

	MotorCmd_t cmd;
	cmd.op_type = OP_CONTROL;
	cmd.motor_id = motor_id;

	MotorCtrl_t ctrl;
	ctrl.type = CTRL_POS;
	ctrl.p.pos.dir = dir;
	ctrl.p.pos.vel = vel;
	ctrl.p.pos.acc = acc;
	#if CURRENT_FIRMWARE == FIRMWARE_X
    uint16_t dec = atoi(argv[6]);
	ctrl.p.pos.dec = dec;
	#endif
	ctrl.p.pos.target = target_angle;
	ctrl.p.pos.mode = mode;
	ctrl.p.pos.sync = false;
	cmd.type.ctrl = ctrl;

	if (Motor_Send_Cmd(&cmd)) {
#if CURRENT_FIRMWARE == FIRMWARE_X
    logPrintln("Position cmd sent to motor %d: dir=%d, vel=%d, acc=%d, dec= %d, target/angle=%ld, mode=%d",
        motor_id, dir, vel, acc, dec, target_angle, mode);
#elif CURRENT_FIRMWARE == FIRMWARE_EMM
    logPrintln("Position cmd sent to motor %d: dir=%d, vel=%d, acc=%d, target/angle=%ld, mode=%d",
            motor_id, dir, vel, acc, target_angle, mode);
#endif
	} else {
		logPrintln("Failed to send position command");
	}
}
#endif /* MOTOR_POS_MODE */

/**
 * @brief 电机实用工具组
 */
static void Motor_tool_Shell(int argc, char *argv[]) {
	if (!is_init) {
		logWarning("Motor module not initialized"); return;
	}

	if (argc < 2) { Tool_Help(); return; }
	MotorCmd_t cmd;
	MotorCtrl_t ctrl;
	MotorTrigger_t trigger;
	MotorParamWrite_t write;
	uint8_t motor_id;
#if MOTOR_POS_WINDOW_WRITE || MOTOR_PID_WRITE || MOTOR_INTEGRAL_LIMIT_WRITE
	bool save = false;
#endif
	char *endptr;

	if (strcmp(argv[1], "cmd") == 0) {
		if (argc != 3) {
			logPrintln("Usage: tool cmd [cmd]"); return;
		}

		char *cmd_str = argv[2];
		uint8_t cmd_buffer[16];
		uint8_t cmd_len = 0;

		char *token = strtok(cmd_str, " ");
		while (token != NULL && cmd_len < sizeof(cmd_buffer)) {
			uint8_t byte = 0;
			sscanf(token, "%2hhx", &byte);
			cmd_buffer[cmd_len++] = byte;
			token = strtok(NULL, " ");
		}
		zdt_v5_port_send(cmd_buffer, cmd_len);
	}
#if MOTOR_CMD_ENABLE
	else if (strcmp(argv[1], "en") == 0) {
		if (argc != 4) { logPrintln("Usage: tool en [id] [state]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;
		cmd.op_type = OP_CONTROL;
		cmd.motor_id = motor_id;

		bool state = atoi(argv[3]);
		ctrl.type = CTRL_ENABLE;
		ctrl.p.en.enable = state;
		ctrl.p.en.sync = false;
		cmd.type.ctrl = ctrl;
		if (Motor_Send_Cmd(&cmd))
			logPrintln("Motor %d is %s", motor_id, state ? "enabled" : "disabled");
		else
			logPrintln("Failed to sen command");
	}
#endif /* MOTOR_CMD_ENABLE */
#if MOTOR_CMD_STOP
	else if (strcmp(argv[1], "stop") == 0) {
		if (argc != 3) { logPrintln("Usage: tool stop [id]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;
		cmd.op_type = OP_CONTROL;
		cmd.motor_id = motor_id;

		ctrl.type = CTRL_STOP;
		ctrl.p.stop.sync = false;
		cmd.type.ctrl = ctrl;
		if (Motor_Send_Cmd(&cmd))
			logPrintln("Motor %d is stopped", motor_id);
		else
			logPrintln("Failed to sen command");
	}
#endif /* MOTOR_CMD_STOP */
#if MOTOR_TRIGGER_RESET_POS
	else if (strcmp(argv[1], "zero") == 0) {
		if (argc != 3) { logPrintln("Usage: tool zero [id]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;
		cmd.op_type = OP_TRIGGER;
		cmd.motor_id = motor_id;

		trigger.type = TRIG_RESET_POS;
		cmd.type.trigger = trigger;
		if (Motor_Send_Cmd(&cmd))
			logPrintln("Motor %d position reset to zero", motor_id);
		else
			logPrintln("Failed to sen command");
	}
#endif /* MOTOR_TRIGGER_RESET_POS */
#if MOTOR_HOME_TRIGGER
	else if (strcmp(argv[1], "home") == 0) {
		if (argc != 3) { logPrintln("Usage: tool home [id]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;
		cmd.op_type = OP_TRIGGER;
		cmd.motor_id = motor_id;

		trigger.type = TRIG_HOME_RETURN;
		trigger.p.home.mode = 0;
		trigger.p.home.sync = false;
		cmd.type.trigger = trigger;
		if (Motor_Send_Cmd(&cmd))
			logPrintln("Motor %d homing triggered", motor_id);
		else
			logPrintln("Failed to sen command");
	}
#endif /* MOTOR_HOME_TRIGGER */
#if MOTOR_TRIGGER_ENCODER_CALIB
	else if (strcmp(argv[1], "cal") == 0) {
		if (argc != 3) { logPrintln("Usage: tool cal [id]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;
		cmd.op_type = OP_TRIGGER;
		cmd.motor_id = motor_id;

		trigger.type = TRIG_ENCODER_CALIB;
		cmd.type.trigger = trigger;
		if (Motor_Send_Cmd(&cmd))
			logPrintln("Starting calibration for motor %d...", motor_id);
		else
			logPrintln("Failed to sen command");
	}
#endif /* MOTOR_TRIGGER_ENCODER_CALIB */
    else if (strcmp(argv[1], "time") == 0) {
		if (argc > 3) {
			logPrintln("Usage: tool time [time]"); return;
		} else if (argc == 2) {
			logPrintln("current time: %d ms", step_update_time); return;
		}

#if MOTOR_HEARTBEAT_WRITE
		cmd.motor_id = motor_id;
		cmd.op_type = OP_PARAM_WRITE;

		write.type = PARAM_HEARTBEAT;
		write.p.heartbeat.save = false;
		write.p.heartbeat.time_ms = atoi(argv[2]) + 100;
		cmd.type.write = write;
		if (Motor_Send_Cmd(&cmd)) {
			step_update_time = atoi(argv[2]);
			logPrintln("Motor update time set to: %d ms", step_update_time);
		} else {
			logPrintln("Failed to sen command");
		}
#else
		step_update_time = atoi(argv[2]);
		logPrintln("Motor update time set to: %d ms", step_update_time);
#endif /* MOTOR_HEARTBEAT_WRITE */
	}
#if MOTOR_BROADCAST_READ_ID
    else if (strcmp(argv[1], "online") == 0){
		if (argc != 3) { logPrintln("Usage: tool online [id]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;

		logPrintln("Motor %d is %s", motor_id, Motor_isonline(motor_id) ? "online" : "offline");
	}
#endif /* MOTOR_BROADCAST_READ_ID */
#if MOTOR_VELOCITY_MODE
	else if (strcmp(argv[1], "found") == 0) {
		if (argc != 3) { logPrintln("Usage: tool found [id]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;
		cmd.op_type = OP_CONTROL;
		cmd.motor_id = motor_id;

		ctrl.type = CTRL_VEL;
		ctrl.p.vel.dir = 0;
		ctrl.p.vel.vel = 300;
		ctrl.p.vel.acc = 100;
		ctrl.p.vel.sync = false;
		cmd.type.ctrl = ctrl;
		if (!Motor_Send_Cmd(&cmd)) logPrintln("Failed to send velocity command");
		logPrintln("Motor %d running 3s", motor_id);
		osDelay(3000);
		ctrl.p.vel.vel = 0;
		while (!Motor_Send_Cmd(&cmd));
	}
#endif /* MOTOR_VELOCITY_MODE */
#if MOTOR_POS_WINDOW_WRITE
	else if (strcmp(argv[1], "window") == 0) {
		if (argc != 5) { logPrintln("Usage: tool window [id] [deg*10] [save]"); return; }
		long id_val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') { logPrintln("invalid id value: %s", argv[2]); return; }
		long win_val = strtol(argv[3], &endptr, 10);
		if(*endptr != '\0') { logPrintln("invalid window value: %s", argv[3]); return; }
		long save_val = strtol(argv[4], &endptr, 10);
		if(*endptr != '\0') { logPrintln("invalid save value: %s", argv[4]); return; }
		motor_id = (uint8_t)id_val;
		uint16_t window = (uint16_t)win_val;
		save = (save_val != 0);

		cmd.op_type = OP_PARAM_WRITE;
		cmd.motor_id = motor_id;

		write.type = PARAM_POS_WINDOW;
		write.p.pos_window.window = window;
		write.p.pos_window.save = save;
		cmd.type.write = write;
		if (Motor_Send_Cmd(&cmd))
			logPrintln("Motor %d window set: %d save=%d", motor_id, window, save);
		else
			logPrintln("Failed to sen command");

	}
#endif /* MOTOR_POS_WINDOW_WRITE */
#if MOTOR_PID_WRITE
	else if (strcmp(argv[1], "pid") == 0) {
#if CURRENT_FIRMWARE == FIRMWARE_EMM
		/* Emm: tool pid [id] [kp] [ki] [kd] [save] → 6 args */
		int req_argc = 6;
#elif CURRENT_FIRMWARE == FIRMWARE_X
		/* X:   tool pid [id] [trap_kp] [direct_kp] [vel_kp] [vel_ki] [save] → 7 args */
		int req_argc = 7;
#endif
		if (argc == 3 || argc == req_argc) {
			long id_val = strtol(argv[2], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid id value: %s", argv[2]); return; }
			motor_id = (uint8_t)id_val;

			if (argc == 3) {
				if (motor_id > 4 || motor_id < 1) { logPrintln("Motor must be 1-4"); return; }
				if (osMutexAcquire(Motor_MutexHandle, osWaitForever) == osOK) {
					MotorStatus_t *m = &g_motor_status->motors[motor_id - 1];
#if CURRENT_FIRMWARE == FIRMWARE_EMM
					logPrintln("id=%d kp=%lu ki=%lu kd=%lu", motor_id,
							   (unsigned long)m->kp, (unsigned long)m->ki,
							   (unsigned long)m->kd);
#elif CURRENT_FIRMWARE == FIRMWARE_X
					logPrintln("id=%d trap_kp=%lu direct_kp=%lu vel_kp=%lu vel_ki=%lu",
							   motor_id,
							   (unsigned long)m->trapezoidal_kp,
							   (unsigned long)m->direct_kp,
							   (unsigned long)m->vel_kp,
							   (unsigned long)m->vel_ki);
#endif
					osMutexRelease(Motor_MutexHandle);
				}
				return;
			}

			long save_val;

#if CURRENT_FIRMWARE == FIRMWARE_EMM
			uint32_t kp = (uint32_t)strtoul(argv[3], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid kp value: %s", argv[3]); return; }
			uint32_t ki = (uint32_t)strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid ki value: %s", argv[4]); return; }
			uint32_t kd = (uint32_t)strtoul(argv[5], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid kd value: %s", argv[5]); return; }
			save_val = strtol(argv[6], &endptr, 10);
			if(*endptr != '\0') { logPrintln("invalid save value: %s", argv[6]); return; }
			save = (save_val != 0);

			cmd.motor_id = motor_id;
			cmd.op_type = OP_PARAM_WRITE;

			write.type = PARAM_PID;
			write.p.pid.kp = kp;
			write.p.pid.ki = ki;
			write.p.pid.kd = kd;
			write.p.pid.save = save;
			cmd.type.write = write;
			if (Motor_Send_Cmd(&cmd))
				logPrintln("Motor %d PID set: kp=%lu ki=%lu kd=%lu save=%d",
						motor_id, (unsigned long)kp, (unsigned long)ki, (unsigned long)kd, save);
			else
				logPrintln("Failed to sen command");

#elif CURRENT_FIRMWARE == FIRMWARE_X
			uint32_t trap_kp = (uint32_t)strtoul(argv[3], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid trapezoidal_kp value: %s", argv[3]); return; }
			uint32_t direct_kp = (uint32_t)strtoul(argv[4], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid direct_kp value: %s", argv[4]); return; }
			uint32_t vel_kp = (uint32_t)strtoul(argv[5], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid vel_kp value: %s", argv[5]); return; }
			uint32_t vel_ki_val = (uint32_t)strtoul(argv[6], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid vel_ki value: %s", argv[6]); return; }
			save_val = strtol(argv[7], &endptr, 10);
			if(*endptr != '\0') { logPrintln("invalid save value: %s", argv[7]); return; }
			save = (save_val != 0);

			cmd.motor_id = motor_id;
			cmd.op_type = OP_PARAM_WRITE;

			write.type = PARAM_PID;
			write.p.pid.trapezoidal_kp = trap_kp;
			write.p.pid.direct_kp = direct_kp;
			write.p.pid.vel_kp = vel_kp;
			write.p.pid.vel_ki = vel_ki_val;
			write.p.pid.save = save;
			cmd.type.write = write;
			if (Motor_Send_Cmd(&cmd))
				logPrintln("Motor %d PID set: trap_kp=%lu direct_kp=%lu vel_kp=%lu vel_ki=%lu save=%d",
						motor_id, (unsigned long)trap_kp, (unsigned long)direct_kp, (unsigned long)vel_kp, (unsigned long)vel_ki_val, save);
			else
				logPrintln("Failed to sen command");
#endif
		} else {
			logPrintln("Usage: tool pid [id]                - show motor PID parameters");
#if CURRENT_FIRMWARE == FIRMWARE_EMM
			logPrintln("       tool pid [id] [kp] [ki] [kd] [save]");
#elif CURRENT_FIRMWARE == FIRMWARE_X
			logPrintln("       tool pid [id] [trap_kp] [direct_kp] [vel_kp] [vel_ki] [save]");
#endif
			return;
		}
	}
#endif /* MOTOR_PID_WRITE */
#if MOTOR_INTEGRAL_LIMIT_WRITE
	else if (strcmp(argv[1], "ilimit") == 0) {
		int req_argc = 5;
		if (argc == 3 || argc == req_argc) {
			long id_val = strtol(argv[2], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid id value: %s", argv[2]); return; }
			motor_id = (uint8_t)id_val;

			if (argc == 3) {
				if (motor_id > 4 || motor_id < 1) { logPrintln("Motor must be 1-4"); return; }
				if (osMutexAcquire(Motor_MutexHandle, osWaitForever) == osOK) {
					MotorStatus_t *m = &g_motor_status->motors[motor_id - 1];
					logPrintln("id=%d integral_limit=%lu", motor_id, (unsigned long)m->integral_limit);
					osMutexRelease(Motor_MutexHandle);
				}
				return;
			}

			uint32_t ilimit_val = (uint32_t)strtoul(argv[3], &endptr, 10);
			if (*endptr != '\0') { logPrintln("invalid value: %s", argv[3]); return; }

			long save_val = strtol(argv[4], &endptr, 10);
			if(*endptr != '\0') { logPrintln("invalid save value: %s", argv[4]); return; }
			bool save = (save_val != 0);

			cmd.motor_id = motor_id;
			cmd.op_type = OP_PARAM_WRITE;

			write.type = PARAM_INTEGRAL_LIMIT;
			write.p.integral_limit.value = ilimit_val;
			write.p.integral_limit.save = save;
			cmd.type.write = write;
			if (Motor_Send_Cmd(&cmd))
				logPrintln("Motor %d integral_limit set: value=%lu save=%d", motor_id, (unsigned long)ilimit_val, save);
			else
				logPrintln("Failed to send command");
		} else {
			logPrintln("Usage: tool ilimit [id]                - show motor integral_limit");
			logPrintln("       tool ilimit [id] [value] [save] - set motor integral_limit");
			return;
		}
	}
#endif
#if MOTOR_TRIGGER_RESTART
	else if (strcmp(argv[1], "rstrt") == 0) {
		if (argc != 3) { logPrintln("Usage: tool rstrt [id]"); return; }
		long val = strtol(argv[2], &endptr, 10);
		if(*endptr != '\0') {
			logPrintln("invalid id value: %s", argv[2]);
			return;
		}
		motor_id = (uint8_t)val;
		cmd.motor_id = motor_id;
		cmd.op_type = OP_TRIGGER;

		trigger.type = TRIG_RESTART;
		cmd.type.trigger = trigger;
		if (Motor_Send_Cmd(&cmd))
			logPrintln("Motor %d restart success", motor_id);
		else
			logPrintln("Failed to send command");
	}
#endif /* MOTOR_TRIGGER_RESTART */
	else {
		logPrintln("Invalid command: %s", argv[1]);
		Tool_Help();
	}
}

#if USE_VIEW
extern void Motor_View_Shell(void);
#endif

ShellCommand StepGroup[] = {
#if USE_VIEW
	SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, view, Motor_View_Shell, View Motor Status),
#endif
#if MOTOR_VELOCITY_MODE
	SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, vel, Motor_vel_Shell, Set Motor Velocity),
#endif
#if MOTOR_POS_MODE
	SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pos, Motor_pos_Shell, Set Motor Position),
#endif
	SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, tool, Motor_tool_Shell, Motor Tools),
	SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
step, StepGroup, Step Control CMD Group);
