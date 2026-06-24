/**
 * @file step_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机端口层源文件
 */

#include "step_port.h"
#include "shell.h"
#include "log.h"
#include "shell_cmd_group.h"
#include "cmsis_os.h"
#include "usart.h"
#include "Events.h"
#include "ZDT_V5_Driver.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern osMutexId_t Motor_MutexHandle;
extern osMessageQueueId_t Usart6_Rx_DataHandle;
extern osMessageQueueId_t MotorCmdsHandle;
MotorStatusShared_t *g_motor_status;
static bool is_init = false;    // 电机模块是否初始化
static uint16_t update_time;    // 电机状态更新时间间隔 (ms)


/**
 * @brief 初始化电机模块
 */
bool Motor_Init(void) {
    extern uint8_t rx6Buffer[USART6_RX_BUF_SIZE];
    update_time = 50;

    g_motor_status = pvPortMalloc(sizeof(MotorStatusShared_t));
    if (!g_motor_status) {
        return false;
    }
    MX_USART6_UART_Init();

    for (uint8_t i = 0; i < 4; i++) {
#if USE_HEARTBEAT
        ZDT_V5_Modify_Heart_Protect(i + 1, false, update_time + 200);
#else
        ZDT_V5_Read_Motor_ID(i + 1);
#endif
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
    osStatus_t status = osMessageQueuePut(MotorCmdsHandle, cmd, 0, 100);
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
            Motor_Process_Cmd(&cmd);
            osDelay(2);     // 防止粘包
        }
    }
}

/**
 * @brief 电机状态获取任务
*/
void Motor_Get_Sta_Task(void *argument) {
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
                Motor_Receive(rxBuf.data, rxBuf.len);
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

    MotorCmd_t cmd;

    for(;;) {
        osDelay(update_time);
        for(uint8_t i = 0; i < 4; i++) {
            cmd.motor_id = i + 1;
#if USE_VIEW
            cmd.op_type = OP_PARAM_READ;
        #if MOTOR_ELECTRICAL
            cmd.type.param.type = PARAM_ELECTRICAL;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_MOTION
            cmd.type.param.type = PARAM_MOTION;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_ENCODER
            cmd.type.param.type = PARAM_ENCODER;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_STATUS_FLAGS
            cmd.type.param.type = PARAM_STATUS;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_SYSTEM
            cmd.type.param.type = PARAM_SYSTEM;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_CONTROL
            cmd.type.param.type = PARAM_CONTROL;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_PROTECTION
            cmd.type.param.type = PARAM_PROTECT;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_CLOG
            cmd.type.param.type = PARAM_CLOG;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_HOME
            cmd.type.param.type = PARAM_HOME;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_DRIVER
            cmd.type.param.type = PARAM_DRIVER;
            Motor_Send_Cmd(&cmd);
        #endif
        #if MOTOR_COMM
            cmd.type.param.type = PARAM_COMM;
            Motor_Send_Cmd(&cmd);
        #endif
#elif USE_HEARTBEAT
            cmd.op_type = OP_HEARTBEAT;
            Motor_Send_Cmd(&cmd);
#endif /* USE_VIEW */
        }
    }
}

/**
 * @brief 电机位置清零
 */
void Motor_zero(void) {
    MotorCmd_t cmd;
    cmd.op_type = OP_CONTROL;
    cmd.type.ctrl.type = CMD_ZERO;
    cmd.motor_id = 0;
    while (!Motor_Send_Cmd(&cmd));
}

/**
 * @brief 检查电机是否在线
 * @param motor_id 电机ID
 * @return true 电机在线，false 电机不在线
 */
static bool Motor_isonline(uint8_t motor_id) {
    MotorStatus_t *motor = &g_motor_status->motors[motor_id - 1];
    motor->is_online = false;
    ZDT_V5_Read_Motor_ID(motor_id);
    osDelay(update_time + 50);
    return motor->is_online;
}

#if MOTOR_CMD_ENABLE
/**
 * @brief 紧急停止
 * */
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

#if MOTOR_CMD_VELOCITY
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
    cmd.type.ctrl.type = CMD_VELOCITY;
    cmd.type.ctrl.p.vel.dir = dir;
    cmd.type.ctrl.p.vel.vel = vel;
    cmd.type.ctrl.p.vel.acc = acc;
    cmd.type.ctrl.p.vel.sync = false;

    if (Motor_Send_Cmd(&cmd)) {
        logPrintln("Velocity cmd sent to motor %d: dir=%d, vel=%d, acc=%d", motor_id, dir, vel, acc);
    } else {
        logPrintln("Failed to send velocity command");
    }
}
#endif /* MOTOR_CMD_VELOCITY */

static void Tool_Help(void) {
    logPrintln("Usage: tool COMMAND [value...]\r\n"
               "\r\n"
               "commands:\r\n"
               "  cmd       Send Motor Command\r\n"
               "  online    Check Motor Online\r\n"
               "  time      View or Set Update Time\r\n"
#if MOTOR_CMD_ENABLE
               "  en        Enable/Disable Motor\r\n"
#endif
#if MOTOR_CMD_VELOCITY
               "  found     Find Motor\r\n"
#endif
#if MOTOR_CMD_STOP
               "  stop      Stop Motor\r\n"
#endif
#if MOTOR_CMD_HOME
               "  zero      Reset Motor Position to Zero\r\n"
               "  home      Trigger Motor Homing\r\n"
#endif
#if MOTOR_CONTROL
               "  window    Set Motor Window\r\n"
               "  pid       Set motor PID and integral limit\r\n"
#endif
               "  cal       Calibrate Motor Encoder");
}

#if MOTOR_CMD_POSITION
/**
 * @brief 单个电机位置控制
 */
static void Motor_pos_Shell(int argc, char *argv[]) {
    if (!is_init) {
        logWarning("Motor module not initialized"); return;
    }

    if (argc != 7) {
        logPrintln("Usage: pos [id] [dir] [vel] [acc] [target/angle] [mode] [dec]\r\n"
                "mode: 0-relative to last target, 1-absolute, 2-relative to current");
        return;
    }

    uint8_t motor_id = atoi(argv[1]);
    uint8_t dir = atoi(argv[2]);
    uint16_t vel = atoi(argv[3]);
    uint16_t acc = atoi(argv[4]);
    int32_t target_angle = atoi(argv[5]);
    uint8_t mode = atoi(argv[6]);

    MotorCmd_t cmd;
    cmd.op_type = OP_CONTROL;
    cmd.type.ctrl.type = CMD_POSITION;
    cmd.type.ctrl.p.pos.dir = dir;
    cmd.type.ctrl.p.pos.vel = vel;
    cmd.type.ctrl.p.pos.acc = acc;
#if CURRENT_FIRMWARE == FIRMWARE_X
    cmd.type.ctrl.p.pos.dec = acc;
#endif
    cmd.type.ctrl.p.pos.target = target_angle;
    cmd.type.ctrl.p.pos.mode = mode;
    cmd.type.ctrl.p.pos.sync = false;
    cmd.motor_id = motor_id;

    if (Motor_Send_Cmd(&cmd)) {
        logPrintln("Position cmd sent to motor %d: dir=%d, vel=%d, acc=%d, target/angle=%ld, mode=%d",
                  motor_id, dir, vel, acc, target_angle, mode);
    } else {
        logPrintln("Failed to send position command");
    }
}
#endif /* MOTOR_CMD_POSITION */

/**
 * @brief 电机实用工具组
 */
static void Motor_tool_Shell(int argc, char *argv[]) {
    if (!is_init) {
        logWarning("Motor module not initialized"); return;
    }

    if (argc < 2) { Tool_Help(); return; }
    uint8_t motor_id;
    bool save = false;
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
        ZDT_V5_SEND_CMD(cmd_buffer, cmd_len);
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
        bool state = atoi(argv[3]);

        MotorCmd_t cmd; cmd.op_type = OP_CONTROL; cmd.type.ctrl.type = CMD_ENABLE;
        cmd.type.ctrl.p.en.enable = state; cmd.type.ctrl.p.en.sync = false;
        cmd.motor_id = motor_id;

        if (Motor_Send_Cmd(&cmd))
        logPrintln("Motor %d is %s", motor_id, state ? "enabled" : "disabled");
        else logPrintln("Failed to send enable command");
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

        ZDT_V5_Stop_Now(motor_id, false);
        logPrintln("Motor %d is stopped", motor_id);
    }
#endif /* MOTOR_CMD_STOP */
#if MOTOR_CMD_HOME
    else if (strcmp(argv[1], "zero") == 0) {
        if (argc != 3) { logPrintln("Usage: tool zero [id]"); return; }
        long val = strtol(argv[2], &endptr, 10);
        if(*endptr != '\0') {
            logPrintln("invalid id value: %s", argv[2]);
            return;
        }
        motor_id = (uint8_t)val;

        MotorCmd_t cmd; cmd.op_type = OP_CONTROL; cmd.motor_id = motor_id;
        cmd.type.ctrl.type = CMD_ZERO;
        if (Motor_Send_Cmd(&cmd)) {
            logPrintln("Motor %d position reset to zero", motor_id);
            ZDT_V5_Origin_Set_O(motor_id, true);
        } else logPrintln("Failed to send zero command");
    }
    else if (strcmp(argv[1], "home") == 0) {
        if (argc != 3) { logPrintln("Usage: tool home [id]"); return; }
        long val = strtol(argv[2], &endptr, 10);
        if(*endptr != '\0') {
            logPrintln("invalid id value: %s", argv[2]);
            return;
        }
        motor_id = (uint8_t)val;

        ZDT_V5_Origin_Trigger_Return(motor_id, 0, false);
        logPrintln("Motor %d homing triggered", motor_id);
    }
#endif /* MOTOR_CMD_HOME */
    else if (strcmp(argv[1], "cal") == 0) {
        if (argc != 3) { logPrintln("Usage: tool cal [id]"); return; }
        long val = strtol(argv[2], &endptr, 10);
        if(*endptr != '\0') {
            logPrintln("invalid id value: %s", argv[2]);
            return;
        }
        motor_id = (uint8_t)val;

        ZDT_V5_Trig_Encoder_Cal(motor_id);
        logPrintln("Starting calibration for motor %d...", motor_id);
    } else if (strcmp(argv[1], "time") == 0) {
        if (argc > 3) {
            logPrintln("Usage: tool time [time]"); return;
        } else if (argc == 2) {
            logPrintln("current time: %d ms", update_time); return;
        }

#if USE_HEARTBEAT
        for (uint8_t i = 0; i < 4; i++) {
            ZDT_V5_Modify_Heart_Protect(i + 1, false, update_time + 500);
        }
#endif

        update_time = atoi(argv[2]);
        logPrintln("Motor update time set to: %d ms", update_time);
    } else if (strcmp(argv[1], "online") == 0){
        if (argc != 3) { logPrintln("Usage: tool online [id]"); return; }
        long val = strtol(argv[2], &endptr, 10);
        if(*endptr != '\0') {
            logPrintln("invalid id value: %s", argv[2]);
            return;
        }
        motor_id = (uint8_t)val;

        logPrintln("Motor %d is %s", motor_id, Motor_isonline(motor_id) ? "online" : "offline");
    }
#if MOTOR_CMD_VELOCITY
    else if (strcmp(argv[1], "found") == 0) {
        if (argc != 3) { logPrintln("Usage: tool found [id]"); return; }
        long val = strtol(argv[2], &endptr, 10);
        if(*endptr != '\0') {
            logPrintln("invalid id value: %s", argv[2]);
            return;
        }
        motor_id = (uint8_t)val;

        MotorCmd_t cmd; cmd.op_type = OP_CONTROL; cmd.motor_id = motor_id;
        cmd.type.ctrl.type = CMD_VELOCITY; cmd.type.ctrl.p.vel.dir = 0;
        cmd.type.ctrl.p.vel.vel = 300; cmd.type.ctrl.p.vel.acc = 100;
        cmd.type.ctrl.p.vel.sync = false;

        if (!Motor_Send_Cmd(&cmd)) logPrintln("Failed to send velocity command");
        logPrintln("Motor %d running 3s", motor_id);
        osDelay(3000);
        cmd.type.ctrl.p.vel.vel = 0;
        while (!Motor_Send_Cmd(&cmd));
    }
#endif /* MOTOR_CMD_VELOCITY */
#if MOTOR_CONTROL
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

        MotorCmd_t cmd;
        cmd.op_type = OP_PARAM_WRITE;
        cmd.motor_id = motor_id;
        cmd.type.param.type = PARAM_CONTROL;
        if (osMutexAcquire(Motor_MutexHandle, osWaitForever) == osOK) {
        cmd.type.param.p.pid.integral_limit = g_motor_status->motors[motor_id - 1].integral_limit;
        cmd.type.param.p.pid.kd = g_motor_status->motors[motor_id - 1].kd;
        cmd.type.param.p.pid.ki = g_motor_status->motors[motor_id - 1].ki;
        cmd.type.param.p.pid.kp = g_motor_status->motors[motor_id - 1].kp;
        osMutexRelease(Motor_MutexHandle);
        }
        cmd.type.param.p.pid.pos_window = window;
        cmd.type.param.p.pid.save = save;

        if (Motor_Send_Cmd(&cmd)) {
            logPrintln("Motor %d window set: %d save=%d", motor_id, window, save);
        } else {
            logPrintln("Failed to send window command");
        }
    } else if (strcmp(argv[1], "pid") == 0) {
        if (argc == 3 || argc == 7) {
            long id_val = strtol(argv[2], &endptr, 10);
            if (*endptr != '\0') { logPrintln("invalid id value: %s", argv[2]); return; }
            motor_id = (uint8_t)id_val;

            if (argc == 3) {
                if (motor_id > 4) { logPrintln("Motor must be 1-4"); return; }
                if (osMutexAcquire(Motor_MutexHandle, osWaitForever) == osOK) {
                    logPrintln("id=%d kp=%lu ki=%lu kd=%lu il=%lu window=%u", motor_id,
                               (unsigned long)g_motor_status->motors[motor_id - 1].kp, (unsigned long)g_motor_status->motors[motor_id - 1].ki,
                               (unsigned long)g_motor_status->motors[motor_id - 1].kd, (unsigned long)g_motor_status->motors[motor_id - 1].integral_limit,
                               (unsigned)g_motor_status->motors[motor_id - 1].pos_window);
                    osMutexRelease(Motor_MutexHandle);
                } return;
            }

            uint32_t kp = (uint32_t)strtoul(argv[3], &endptr, 10);
            if (*endptr != '\0') { logPrintln("invalid kp value: %s", argv[3]); return; }
            uint32_t ki = (uint32_t)strtoul(argv[4], &endptr, 10);
            if (*endptr != '\0') { logPrintln("invalid ki value: %s", argv[4]); return; }
            uint32_t kd = (uint32_t)strtoul(argv[5], &endptr, 10);
            if (*endptr != '\0') { logPrintln("invalid kd value: %s", argv[5]); return; }
            uint32_t il = (uint32_t)strtoul(argv[6], &endptr, 10);
            if (*endptr != '\0') { logPrintln("invalid integral_limit value: %s", argv[6]); return; }
            long save_val = strtol(argv[7], &endptr, 10);
            if(*endptr != '\0') { logPrintln("invalid save value: %s", argv[7]); return; }
            save = (save_val != 0);

            MotorCmd_t cmd;
            cmd.op_type = OP_PARAM_WRITE; cmd.motor_id = motor_id; cmd.type.param.type = PARAM_CONTROL;
            cmd.type.param.p.pid.kp = kp; cmd.type.param.p.pid.ki = ki; cmd.type.param.p.pid.kd = kd;
            cmd.type.param.p.pid.integral_limit = il;
            if (osMutexAcquire(Motor_MutexHandle, osWaitForever) == osOK) {
                cmd.type.param.p.pid.pos_window = g_motor_status->motors[motor_id - 1].pos_window;
                osMutexRelease(Motor_MutexHandle);
            }
            cmd.type.param.p.pid.save = save;

            if (Motor_Send_Cmd(&cmd)) {
                logPrintln("Motor %d PID/limit set: kp=%lu ki=%lu kd=%lu il=%lu save=%d", motor_id, (unsigned long)kp,
                           (unsigned long)ki, (unsigned long)kd, (unsigned long)il, save);
            } else {
                logPrintln("Failed to send pid command");
            }
        } else {
            logPrintln("Usage: tool pid [id]            - show motor PID parameters");
            logPrintln("       tool pid [id] [kp] [ki] [kd] [integral_limit] [save]");
            return;
        }
    }
#endif /* MOTOR_CONTROL */
    else {
        logPrintln("Invalid command: %s", argv[1]);
        Tool_Help();
    }
}

#if USE_VIEW
static void Motor_View_Shell(void);
#endif

ShellCommand StepGroup[] = {
#if USE_VIEW
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, view, Motor_View_Shell, View Motor Status),
#endif
#if MOTOR_CMD_VELOCITY
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, vel, Motor_vel_Shell, Set Motor Velocity),
#endif
#if MOTOR_CMD_POSITION
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pos, Motor_pos_Shell, Set Motor Position),
#endif
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, tool, Motor_tool_Shell, Motor Tools),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
step, StepGroup, Step Control CMD Group);


#if USE_VIEW
#if MOTOR_HOME
static const char* home_mode_str(uint8_t mode) {
	switch (mode) {
		case 0: return "Near"; case 1: return "Dir"; case 2: return "Col";
		case 3: return "Limit"; default: return "?";
	}
}
#endif
#if MOTOR_DRIVER
static const char* ctrl_str(uint8_t v) { return v == 0 ? "Open" : "Close"; }
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
#if MOTOR_DRIVER || MOTOR_HOME
static const char* dir_str(uint8_t dir) { return dir == 0 ? "CW" : "CCW"; }
#endif
#if MOTOR_DRIVER || MOTOR_HOME || MOTOR_CLOG
static const char* onoff_str(uint8_t v) { return v ? "On" : "Off"; }
#endif
#if MOTOR_COMM
static const char* uart_baud_str(uint8_t code) {
	switch (code) {
		case 0: return "9600"; case 1: return "19200"; case 2: return "25000"; case 3: return "38400";
		case 4: return "57600"; case 5: return "115200"; case 6: return "256000"; case 7: return "512000";
		case 8: return "921600"; default: return "?";
	}
}
static const char* can_baud_str(uint8_t code) {
	switch (code) {
		case 0: return "10K"; case 1: return "20K"; case 2: return "50K"; case 3: return "83K";
		case 4: return "100K"; case 5: return "125K"; case 6: return "250K"; case 7: return "500K";
		case 8: return "800K"; case 9: return "1M"; default: return "?";
	}
}
static const char* verify_str(uint8_t code) {
	switch (code) {
		case 0: return "None"; case 1: return "Even"; case 2: return "Odd";
		case 3: return "Low"; case 4: return "High"; default: return "?";
	}
}
static const char* respond_str(uint8_t code) {
	switch (code) {
		case 0: return "Always"; case 1: return "Once"; case 2: return "Silent";
		case 3: return "Broad"; case 4: return "None"; default: return "?";
	}
}
#endif /* MOTOR_COMM */
#if MOTOR_MOTION
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
#endif /* MOTOR_MOTION */
/**
 * @brief 查看电机状态
 */
static void Motor_View_Shell(void) {
    char ch;
    extern Shell shell;
    uint8_t line_count = 0;

    if (!is_init) {
        logWarning("Motor module not initialized"); return;
    }

{   // 首次输出包含
    logPrintln("Motor Status Viewer - Press ^C to exit\r\n"
               "  ID |   -   |   -   |   -   |   -   |");
    line_count += 1;
#if MOTOR_ELECTRICAL
    logPrintln("V(mV)|-------|-------|-------|-------|");
#if CURRENT_FIRMWARE == FIRMWARE_X
    logPrintln("BusI |-------|-------|-------|-------|");
    line_count++;
#endif
    logPrintln("PhI  |-------|-------|-------|-------|\r\n"
               "Temp |------|------|------|------|");
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
    logPrintln("BatV |-------|-------|-------|-------|");
    line_count++;
#endif
    line_count += 3;
#endif /* MOTOR_ELECTRICAL */
#if MOTOR_MOTION
    logPrintln("Vel  |-------|-------|-------|-------|\r\n"
               "Pos  |-------|-------|-------|-------|\r\n"
               "TPos |-------|-------|-------|-------|\r\n"
               "Err  |-------|-------|-------|-------|");
    line_count += 4;
#endif
#if MOTOR_ENCODER
    logPrintln("EncL |-------|-------|-------|-------|");
#if CURRENT_FIRMWARE == FIRMWARE_X
    logPrintln("EncR |-------|-------|-------|-------|");
    line_count++;
#endif
    line_count++;
#endif
#if MOTOR_STATUS_FLAGS
    logPrintln("Sta  |-------|-------|-------|-------|\r\n"
               "Hom  |-------|-------|-------|-------|\r\n"
               "Pin  |-------|-------|-------|-------|");
    line_count += 3;
#endif
#if MOTOR_MOTION
    logPrintln("SPos |-------|-------|-------|-------|\r\n"
               "Puls |-------|-------|-------|-------|");
    line_count += 2;
#endif
#if MOTOR_SYSTEM
    logPrintln("FWVer|-------|-------|-------|-------|\r\n"
               "HWVer|-------|-------|-------|-------|\r\n"
               "Res  |-------|-------|-------|-------|\r\n"
               "Ind  |-------|-------|-------|-------|\r\n"
               "Opt  |-------|-------|-------|-------|\r\n"
               "LockL|-------|-------|-------|-------|");
    line_count += 6;
#endif
#if MOTOR_CONTROL
    logPrintln("Kp   |-------|-------|-------|-------|\r\n"
               "Ki   |-------|-------|-------|-------|\r\n"
               "Kd   |-------|-------|-------|-------|\r\n"
               "PosW |-------|-------|-------|-------|\r\n"
               "IntL |-------|-------|-------|-------|");
    line_count += 5;
#endif
#if MOTOR_PROTECTION
    logPrintln("TempT|-------|-------|-------|-------|\r\n"
               "CurrT|-------|-------|-------|-------|\r\n"
               "ProtT|-------|-------|-------|-------|\r\n"
               "HearT|-------|-------|-------|-------|\r\n"
               "ColA |-------|-------|-------|-------|");
    line_count += 5;
#endif
#if MOTOR_CLOG
    logPrintln("ClogE|-------|-------|-------|-------|\r\n"
               "ClogR|-------|-------|-------|-------|\r\n"
               "ClogC|-------|-------|-------|-------|\r\n"
               "ClogT|-------|-------|-------|-------|");
    line_count += 4;
#endif
#if MOTOR_HOME
    logPrintln("HomeM|-------|-------|-------|-------|\r\n"
               "HomeD|-------|-------|-------|-------|\r\n"
               "HomeS|-------|-------|-------|-------|\r\n"
               "HomeT|-------|-------|-------|-------|\r\n"
               "HmAEn|-------|-------|-------|-------|\r\n"
               "ColR |-------|-------|-------|-------|\r\n"
               "ColC |-------|-------|-------|-------|\r\n"
               "ColT |-------|-------|-------|-------|");
    line_count += 8;
#endif
#if MOTOR_DRIVER
    logPrintln("CtrlM|-------|-------|-------|-------|\r\n"
               "MotTp|-------|-------|-------|-------|\r\n"
               "MotD |-------|-------|-------|-------|\r\n"
               "Micro|-------|-------|-------|-------|\r\n"
               "Inter|-------|-------|-------|-------|\r\n"
               "OpenI|-------|-------|-------|-------|\r\n"
               "ClosI|-------|-------|-------|-------|");
#if CURRENT_FIRMWARE == FIRMWARE_EMM
    logPrintln("MaxV |-------|-------|-------|-------|");
    line_count++;
#elif CURRENT_FIRMWARE == FIRMWARE_X
    logPrintln("MaxS |-------|-------|-------|-------|");
    line_count++;
#endif
    line_count += 7;
#endif
#if MOTOR_COMM
    logPrintln("Uart |--------|--------|--------|--------|\r\n"
               "Can  |--------|--------|--------|--------|\r\n"
               "Vrfy |--------|--------|--------|--------|\r\n"
               "Resp |--------|--------|--------|--------|");
#if CURRENT_FIRMWARE == FIRMWARE_X
    logPrintln("PScl |--------|--------|--------|--------|");
    line_count++;
#endif
    line_count += 4;
#endif
}

    for(;;) {
        logPrintln("\033[%dA\033[2K\r  ID | %3d   | %3d   | %3d   | %3d   |",
                line_count,
                g_motor_status->motors[0].motor_id,
                g_motor_status->motors[1].motor_id,
                g_motor_status->motors[2].motor_id,
                g_motor_status->motors[3].motor_id);
#if MOTOR_ELECTRICAL
        logPrintln("\033[2K\rV(mV)|%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].voltage,
                g_motor_status->motors[1].voltage,
                g_motor_status->motors[2].voltage,
                g_motor_status->motors[3].voltage);
#if CURRENT_FIRMWARE == FIRMWARE_X
        logPrintln("\033[2K\rBusI|%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].bus_current,
                g_motor_status->motors[1].bus_current,
                g_motor_status->motors[2].bus_current,
                g_motor_status->motors[3].bus_current);
#endif
        logPrintln("\033[2K\rPhI  |%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rTemp | %4d  | %4d  | %4d  | %4d  |",
                g_motor_status->motors[0].phase_current,
                g_motor_status->motors[1].phase_current,
                g_motor_status->motors[2].phase_current,
                g_motor_status->motors[3].phase_current,
                g_motor_status->motors[0].temp,
                g_motor_status->motors[1].temp,
                g_motor_status->motors[2].temp,
                g_motor_status->motors[3].temp);
#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        logPrintln("\033[2K\rBatV |%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].battery_voltage,
                g_motor_status->motors[1].battery_voltage,
                g_motor_status->motors[2].battery_voltage,
                g_motor_status->motors[3].battery_voltage);
#endif
#endif /* MOTOR_ELECTRICAL */
#if MOTOR_MOTION
        logPrintln("\033[2K\rVel  |%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rPos  |%7s|%7s|%7s|%7s|\r\n"
                "\033[2K\rTPos |%7s|%7s|%7s|%7s|\r\n"
                "\033[2K\rErr  |%7s|%7s|%7s|%7s|",
                g_motor_status->motors[0].vel,
                g_motor_status->motors[1].vel,
                g_motor_status->motors[2].vel,
                g_motor_status->motors[3].vel,
                fmt_int(g_motor_status->motors[0].pos),
                fmt_int(g_motor_status->motors[1].pos),
                fmt_int(g_motor_status->motors[2].pos),
                fmt_int(g_motor_status->motors[3].pos),
                fmt_int(g_motor_status->motors[0].target_pos),
                fmt_int(g_motor_status->motors[1].target_pos),
                fmt_int(g_motor_status->motors[2].target_pos),
                fmt_int(g_motor_status->motors[3].target_pos),
                fmt_int(g_motor_status->motors[0].pos_error),
                fmt_int(g_motor_status->motors[1].pos_error),
                fmt_int(g_motor_status->motors[2].pos_error),
                fmt_int(g_motor_status->motors[3].pos_error));
#endif
#if MOTOR_ENCODER
                logPrintln("\033[2K\rEncL |%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].encoder_linear,
                g_motor_status->motors[1].encoder_linear,
                g_motor_status->motors[2].encoder_linear,
                g_motor_status->motors[3].encoder_linear);
#if CURRENT_FIRMWARE == FIRMWARE_X
                logPrintln("\033[2K\rEncR |%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].encoder_raw,
                g_motor_status->motors[1].encoder_raw,
                g_motor_status->motors[2].encoder_raw,
                g_motor_status->motors[3].encoder_raw);
#endif
#endif
#if MOTOR_STATUS_FLAGS
        {
            uint8_t sta[4], hom[4];
            for (int i = 0; i < 4; i++) {
                sta[i] = (g_motor_status->motors[i].oac << 7) | (g_motor_status->motors[i].esi_r << 6) |
                         (g_motor_status->motors[i].esi_l << 4) | (g_motor_status->motors[i].cgp << 3) |
                         (g_motor_status->motors[i].cgi << 2) | (g_motor_status->motors[i].prf << 1) |
                         g_motor_status->motors[i].ens;
                hom[i] = (g_motor_status->motors[i].ocp_tf << 7) | (g_motor_status->motors[i].otp_tf << 4) |
                         (g_motor_status->motors[i].org_cf << 3) | (g_motor_status->motors[i].org_sf << 2) |
                         (g_motor_status->motors[i].cal_rdy << 1) | g_motor_status->motors[i].enc_rdy;
            }
            logPrintln("\033[2K\rSta  |  %04X |  %04X |  %04X |  %04X |\r\n"
                    "\033[2K\rHom  |  %04X |  %04X |  %04X |  %04X |\r\n"
                    "\033[2K\rPin  |  %04X |  %04X |  %04X |  %04X |",
                    sta[0], sta[1], sta[2], sta[3],
                    hom[0], hom[1], hom[2], hom[3],
                    g_motor_status->motors[0].pin_status,
                    g_motor_status->motors[1].pin_status,
                    g_motor_status->motors[2].pin_status,
                    g_motor_status->motors[3].pin_status);
        }
#endif
#if MOTOR_MOTION
        logPrintln("\033[2K\rSPos |%7s|%7s|%7s|%7s|\r\n"
                "\033[2K\rPuls |%7s|%7s|%7s|%7s|",
                fmt_int(g_motor_status->motors[0].set_pos),
                fmt_int(g_motor_status->motors[1].set_pos),
                fmt_int(g_motor_status->motors[2].set_pos),
                fmt_int(g_motor_status->motors[3].set_pos),
                fmt_int(g_motor_status->motors[0].input_pulses),
                fmt_int(g_motor_status->motors[1].input_pulses),
                fmt_int(g_motor_status->motors[2].input_pulses),
                fmt_int(g_motor_status->motors[3].input_pulses));
#endif
#if MOTOR_SYSTEM
    {   static char fw[4][8];
        for (int _i = 0; _i < 4; _i++) {
            uint16_t v = g_motor_status->motors[_i].firmware_version;
            sprintf(fw[_i], "%d.%02d", v / 100, v % 100);
        }
        logPrintln("\033[2K\rFWVer|%7s|%7s|%7s|%7s|",
                fw[0], fw[1], fw[2], fw[3]);
    }
        logPrintln("\033[2K\rHWVer|  %4d  |  %4d  |  %4d  |  %4d  |\r\n"
                "\033[2K\rRes  |%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rInd  |%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rOpt  |  %04X |  %04X |  %04X |  %04X |\r\n"
                "\033[2K\rLockL|  %4d  |  %4d  |  %4d  |  %4d  |",
                g_motor_status->motors[0].hardware_version,
                g_motor_status->motors[1].hardware_version,
                g_motor_status->motors[2].hardware_version,
                g_motor_status->motors[3].hardware_version,
                g_motor_status->motors[0].phase_resistance,
                g_motor_status->motors[1].phase_resistance,
                g_motor_status->motors[2].phase_resistance,
                g_motor_status->motors[3].phase_resistance,
                g_motor_status->motors[0].phase_inductance,
                g_motor_status->motors[1].phase_inductance,
                g_motor_status->motors[2].phase_inductance,
                g_motor_status->motors[3].phase_inductance,
                g_motor_status->motors[0].option_params,
                g_motor_status->motors[1].option_params,
                g_motor_status->motors[2].option_params,
                g_motor_status->motors[3].option_params,
                g_motor_status->motors[0].lock_level,
                g_motor_status->motors[1].lock_level,
                g_motor_status->motors[2].lock_level,
                g_motor_status->motors[3].lock_level);
#endif
#if MOTOR_CONTROL
        logPrintln("\033[2K\rKp   |%7d|%7d|%7d|%7d|\r\n"
            "\033[2K\rKi   |%7d|%7d|%7d|%7d|\r\n"
            "\033[2K\rKd   |%7d|%7d|%7d|%7d|\r\n"
            "\033[2K\rPosW |%7d|%7d|%7d|%7d|\r\n"
            "\033[2K\rIntL |%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].kp,
                g_motor_status->motors[1].kp,
                g_motor_status->motors[2].kp,
                g_motor_status->motors[3].kp,
                g_motor_status->motors[0].ki,
                g_motor_status->motors[1].ki,
                g_motor_status->motors[2].ki,
                g_motor_status->motors[3].ki,
                g_motor_status->motors[0].kd,
                g_motor_status->motors[1].kd,
                g_motor_status->motors[2].kd,
                g_motor_status->motors[3].kd,
                g_motor_status->motors[0].pos_window,
                g_motor_status->motors[1].pos_window,
                g_motor_status->motors[2].pos_window,
                g_motor_status->motors[3].pos_window,
                g_motor_status->motors[0].integral_limit,
                g_motor_status->motors[1].integral_limit,
                g_motor_status->motors[2].integral_limit,
                g_motor_status->motors[3].integral_limit);
#endif
#if MOTOR_PROTECTION
        logPrintln("\033[2K\rTempT|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rCurrT|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rProtT|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rHearT|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rColA |%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].temp_threshold,
                g_motor_status->motors[1].temp_threshold,
                g_motor_status->motors[2].temp_threshold,
                g_motor_status->motors[3].temp_threshold,
                g_motor_status->motors[0].current_threshold,
                g_motor_status->motors[1].current_threshold,
                g_motor_status->motors[2].current_threshold,
                g_motor_status->motors[3].current_threshold,
                g_motor_status->motors[0].protect_time,
                g_motor_status->motors[1].protect_time,
                g_motor_status->motors[2].protect_time,
                g_motor_status->motors[3].protect_time,
                g_motor_status->motors[0].heartbeat_time,
                g_motor_status->motors[1].heartbeat_time,
                g_motor_status->motors[2].heartbeat_time,
                g_motor_status->motors[3].heartbeat_time,
                g_motor_status->motors[0].collision_angle,
                g_motor_status->motors[1].collision_angle,
                g_motor_status->motors[2].collision_angle,
                g_motor_status->motors[3].collision_angle);
#endif
#if MOTOR_CLOG
        logPrintln("\033[2K\rClogE| %-5s | %-5s | %-5s | %-5s |\r\n",
                "\033[2K\rClogR|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rClogC|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rClogT|%7d|%7d|%7d|%7d|",
                onoff_str(g_motor_status->motors[0].clog_enable),
                onoff_str(g_motor_status->motors[1].clog_enable),
                onoff_str(g_motor_status->motors[2].clog_enable),
                onoff_str(g_motor_status->motors[3].clog_enable),
                g_motor_status->motors[0].clog_rpm,
                g_motor_status->motors[1].clog_rpm,
                g_motor_status->motors[2].clog_rpm,
                g_motor_status->motors[3].clog_rpm,
                g_motor_status->motors[0].clog_current,
                g_motor_status->motors[1].clog_current,
                g_motor_status->motors[2].clog_current,
                g_motor_status->motors[3].clog_current,
                g_motor_status->motors[0].clog_time,
                g_motor_status->motors[1].clog_time,
                g_motor_status->motors[2].clog_time,
                g_motor_status->motors[3].clog_time);
#endif
#if MOTOR_HOME
        logPrintln("\033[2K\rHomeM| %-4s  | %-4s  | %-4s  | %-4s  |\r\n",
                "\033[2K\rHomeD| %-4s | %-4s | %-4s | %-4s |\r\n"
                "\033[2K\rHomeS|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rHomeT|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rHmAEn| %-4s  | %-4s  | %-4s  | %-4s  |\r\n"
                "\033[2K\rColR |%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rColC |%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rColT |%7d|%7d|%7d|%7d|",
                home_mode_str(g_motor_status->motors[0].home_mode),
                home_mode_str(g_motor_status->motors[1].home_mode),
                home_mode_str(g_motor_status->motors[2].home_mode),
                home_mode_str(g_motor_status->motors[3].home_mode),
                dir_str(g_motor_status->motors[0].home_dir),
                dir_str(g_motor_status->motors[1].home_dir),
                dir_str(g_motor_status->motors[2].home_dir),
                dir_str(g_motor_status->motors[3].home_dir),
                g_motor_status->motors[0].home_speed,
                g_motor_status->motors[1].home_speed,
                g_motor_status->motors[2].home_speed,
                g_motor_status->motors[3].home_speed,
                g_motor_status->motors[0].home_timeout,
                g_motor_status->motors[1].home_timeout,
                g_motor_status->motors[2].home_timeout,
                g_motor_status->motors[3].home_timeout,
                onoff_str(g_motor_status->motors[0].home_auto_enable),
                onoff_str(g_motor_status->motors[1].home_auto_enable),
                onoff_str(g_motor_status->motors[2].home_auto_enable),
                onoff_str(g_motor_status->motors[3].home_auto_enable),
                g_motor_status->motors[0].collision_rpm,
                g_motor_status->motors[1].collision_rpm,
                g_motor_status->motors[2].collision_rpm,
                g_motor_status->motors[3].collision_rpm,
                g_motor_status->motors[0].collision_current,
                g_motor_status->motors[1].collision_current,
                g_motor_status->motors[2].collision_current,
                g_motor_status->motors[3].collision_current,
                g_motor_status->motors[0].collision_time,
                g_motor_status->motors[1].collision_time,
                g_motor_status->motors[2].collision_time,
                g_motor_status->motors[3].collision_time);
#endif
#if MOTOR_DRIVER
        logPrintln("\033[2K\rCtrlM| %-4s  | %-4s  | %-4s  | %-4s  |\r\n"
                "\033[2K\rMotTp| %-4s | %-4s | %-4s | %-4s |\r\n"
                "\033[2K\rMotD | %-4s | %-4s | %-4s | %-4s |",
                ctrl_str(g_motor_status->motors[0].control_mode),
                ctrl_str(g_motor_status->motors[1].control_mode),
                ctrl_str(g_motor_status->motors[2].control_mode),
                ctrl_str(g_motor_status->motors[3].control_mode),
                motor_type_str(g_motor_status->motors[0].motor_type),
                motor_type_str(g_motor_status->motors[1].motor_type),
                motor_type_str(g_motor_status->motors[2].motor_type),
                motor_type_str(g_motor_status->motors[3].motor_type),
                dir_str(g_motor_status->motors[0].motor_direction),
                dir_str(g_motor_status->motors[1].motor_direction),
                dir_str(g_motor_status->motors[2].motor_direction),
                dir_str(g_motor_status->motors[3].motor_direction));
    {   static char ms[4][6];
        for (int _i = 0; _i < 4; _i++) {
            uint8_t v = g_motor_status->motors[_i].micro_step;
            sprintf(ms[_i], v == 0 ? "256" : "%d", v);
        }
        logPrintln("\033[2K\rMicro| %-4s | %-4s | %-4s | %-4s |",
                ms[0], ms[1], ms[2], ms[3]);
    }
        logPrintln("\033[2K\rInter| %-4s | %-4s | %-4s | %-4s |\r\n",
                "\033[2K\rOpenI|%7d|%7d|%7d|%7d|\r\n"
                "\033[2K\rClosI|%7d|%7d|%7d|%7d|",
                onoff_str(g_motor_status->motors[0].interpolation),
                onoff_str(g_motor_status->motors[1].interpolation),
                onoff_str(g_motor_status->motors[2].interpolation),
                onoff_str(g_motor_status->motors[3].interpolation),
                g_motor_status->motors[0].open_current,
                g_motor_status->motors[1].open_current,
                g_motor_status->motors[2].open_current,
                g_motor_status->motors[3].open_current,
                g_motor_status->motors[0].close_current,
                g_motor_status->motors[1].close_current,
                g_motor_status->motors[2].close_current,
                g_motor_status->motors[3].close_current);
#if CURRENT_FIRMWARE == FIRMWARE_EMM
        logPrintln("\033[2K\rMaxV |%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].max_output_voltage,
                g_motor_status->motors[1].max_output_voltage,
                g_motor_status->motors[2].max_output_voltage,
                g_motor_status->motors[3].max_output_voltage);
#elif CURRENT_FIRMWARE == FIRMWARE_X
        logPrintln("\033[2K\rMaxS |%7d|%7d|%7d|%7d|",
                g_motor_status->motors[0].max_speed,
                g_motor_status->motors[1].max_speed,
                g_motor_status->motors[2].max_speed,
                g_motor_status->motors[3].max_speed);
#endif
#endif /* MOTOR_DRIVER */
#if MOTOR_COMM
        logPrintln("\033[2K\rUart | %-6s | %-6s | %-6s | %-6s |\r\n"
                "\033[2K\rCan  | %-6s | %-6s | %-6s | %-6s |\r\n"
                "\033[2K\rVrfy | %-5s | %-5s | %-5s | %-5s |\r\n"
                "\033[2K\rResp | %-6s | %-6s | %-6s | %-6s |"
                uart_baud_str(g_motor_status->motors[0].uart_baudrate),
                uart_baud_str(g_motor_status->motors[1].uart_baudrate),
                uart_baud_str(g_motor_status->motors[2].uart_baudrate),
                uart_baud_str(g_motor_status->motors[3].uart_baudrate),
                can_baud_str(g_motor_status->motors[0].can_baudrate),
                can_baud_str(g_motor_status->motors[1].can_baudrate),
                can_baud_str(g_motor_status->motors[2].can_baudrate),
                can_baud_str(g_motor_status->motors[3].can_baudrate),
                verify_str(g_motor_status->motors[0].verify_mode),
                verify_str(g_motor_status->motors[1].verify_mode),
                verify_str(g_motor_status->motors[2].verify_mode),
                verify_str(g_motor_status->motors[3].verify_mode),
                respond_str(g_motor_status->motors[0].response_mode),
                respond_str(g_motor_status->motors[1].response_mode),
                respond_str(g_motor_status->motors[2].response_mode),
                respond_str(g_motor_status->motors[3].response_mode));
#if CURRENT_FIRMWARE == FIRMWARE_X
        logPrintln("\033[2K\rPScl | %-4s | %-4s | %-4s | %-4s |",
                onoff_str(g_motor_status->motors[0].pos_scale),
                onoff_str(g_motor_status->motors[1].pos_scale),
                onoff_str(g_motor_status->motors[2].pos_scale),
                onoff_str(g_motor_status->motors[3].pos_scale));
#endif
#endif /* MOTOR_COMM */
        osDelay(update_time);
        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break;
        }
    }
    logPrintln("\033[%dA\033[J\033[2A", ++line_count);
}
#endif /* USE_VIEW */
