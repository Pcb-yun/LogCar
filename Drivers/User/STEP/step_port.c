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
#include "can.h"
#include "Events.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief 初始化电机模块
 */
bool Motor_Init(void) {
    MX_CAN1_Init();

    CAN_FilterTypeDef canfilter = {0};
    canfilter.FilterActivation = CAN_FILTER_ENABLE;
    canfilter.FilterBank = 0;
    canfilter.FilterMode = CAN_FILTERMODE_IDMASK;
    canfilter.FilterScale = CAN_FILTERSCALE_32BIT;
    canfilter.FilterIdHigh = 0x0000;
    canfilter.FilterIdLow  = 0x0000;
    canfilter.FilterMaskIdHigh = 0x0000;
    canfilter.FilterMaskIdLow  = 0x0000;
    canfilter.FilterFIFOAssignment = CAN_RX_FIFO0;

    if (HAL_CAN_ConfigFilter(&hcan1, &canfilter) != HAL_OK) return false;
    if (HAL_CAN_Start(&hcan1) != HAL_OK) return false;
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) return false;

    for (uint8_t i = 0; i < 4; i++) {
        g_motor_status.motors[i].motor_id = i + 1;
    }

    g_motor_status.is_init = true;
    return true;
}

/**
 * @brief 发送电机控制命令到队列
 * @param cmd 电机控制命令指针
 * @return true 成功发送，false 失败
 */
bool Motor_Send_Cmd(MotorCmd_t *cmd) {
    extern osMessageQueueId_t MotorCmdsHandle;

    osStatus_t status = osMessageQueuePut(MotorCmdsHandle, cmd, 0, 100);
    return (status == osOK);
}

/**
 * @brief 电机控制任务
 * @param argument 任务参数
 */
void Motor_Ctrl_Task(void *argument) {
    (void)argument;
    extern osMessageQueueId_t MotorCmdsHandle;
    MotorCmd_t cmd;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!g_motor_status.is_init)  vTaskDelete(NULL);

    for(;;) {
        if (osMessageQueueGet(MotorCmdsHandle, &cmd, NULL, osWaitForever) == osOK) {
            Motor_Process_Cmd(&cmd);
        }
    }
}

/**
 * @brief 电机状态任务
 * @param argument 任务参数
 */
void Motor_Get_Sta_Task(void *argument) {
    (void)argument;
    CAN_Rx_Message_t msg;
    extern osMessageQueueId_t Can1_Rx_DataHandle;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!g_motor_status.is_init)  vTaskDelete(NULL);

    for(;;) {
        if (osMessageQueueGet(Can1_Rx_DataHandle, &msg, NULL, osWaitForever) == osOK) {
            if (process_multi_packet(&msg) != true) {
                logWarning("Failed to process multi packet");
            }
        }
    }
}

/**
 * @brief 测试电机控制命令
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
static void Motor_cmd_tset(int argc, char *argv[]) {
    if (!g_motor_status.is_init) {
        logPrintln("Motor not initialized");
        return;
    }

    if (argc != 2) {
        logPrintln("Usage: cmd [cmd]" \
                    "Example: cmd \"01 F3 AB 00 00 6B\"");
        return;
    }

    char *cmd_str = argv[1];
    uint8_t cmd_buffer[64];
    uint8_t cmd_len = 0;

    char *token = strtok(cmd_str, " ");
    while (token != NULL && cmd_len < sizeof(cmd_buffer)) {
        uint8_t byte = 0;
        sscanf(token, "%2hhx", &byte);
        cmd_buffer[cmd_len++] = byte;
        token = strtok(NULL, " ");
    }

    extern osThreadId_t Motor_CtrlHandle;
    extern osThreadId_t Motor_Get_StaHandle;
    osThreadSuspend(Motor_CtrlHandle);
    osThreadSuspend(Motor_Get_StaHandle);

    ZDT_V5_SEND_CMD(cmd_buffer, cmd_len);

    CAN_Rx_Message_t rx_msg;

    logPrintln("Waiting for motor response...");
    extern osMessageQueueId_t Can1_Rx_DataHandle;

    if (osMessageQueueGet(Can1_Rx_DataHandle, &rx_msg, 0, 500) == osOK) {
        char buffer[64];
        uint8_t pos = 0;

        for (uint8_t i = 0; i < rx_msg.DLC; i++) {
            if (i > 0) {
                buffer[pos++] = ' ';
            }
            pos += sprintf(&buffer[pos], "%02X", rx_msg.data[i]);
        }
        buffer[pos] = '\0';

        logPrintln("ExtId: %08X, DLC: %d, Data: %s", rx_msg.ExtId, rx_msg.DLC, buffer);
    } else {
        logPrintln("Timeout waiting for motor response");
    }

    osThreadResume(Motor_CtrlHandle);
    osThreadResume(Motor_Get_StaHandle);
}

/**
 * @brief 查看电机状态
 */
static void Motor_View(void) {
    char ch;
    extern Shell shell;

    uint8_t line_count = 0;

    if (!g_motor_status.is_init) {
        logPrintln("Motor not initialized");
        return;
    }

    logPrintln("Motor Status Viewer - Press ^C to exit");
    logPrintln("  ID |  -  |  -  |  -  |  -  |");
    line_count += 1;
#if MOTOR_STATUS_ELECTRICAL
    logPrintln("V(mV)|-----|-----|-----|-----|");
    logPrintln("I(mA)|-----|-----|-----|-----|");
    logPrintln("PhI  |-----|-----|-----|-----|");
    logPrintln("Temp |-----|-----|-----|-----|");
    line_count += 4;
    #if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
    logPrintln("BatV |-----|-----|-----|-----|");
    line_count++;
    #endif
#endif
#if MOTOR_STATUS_MOTION
    logPrintln("Vel  |-----|-----|-----|-----|");
    logPrintln("Pos  |-----|-----|-----|-----|");
    logPrintln("TPos |-----|-----|-----|-----|");
    logPrintln("Err  |-----|-----|-----|-----|");
    line_count += 4;
#endif
#if MOTOR_STATUS_ENCODER
    logPrintln("Enc  |-----|-----|-----|-----|");
    line_count++;
#endif
#if MOTOR_STATUS_STATUS
    logPrintln("Sta  |-----|-----|-----|-----|");
    logPrintln("Hom  |-----|-----|-----|-----|");
    line_count += 2;
#endif

    for(;;) {
        logPrintln("\033[%dA\033[2K\r  ID |  %d  |  %d  |  %d  |  %d  |",
                line_count,
                g_motor_status.motors[0].motor_id,
                g_motor_status.motors[1].motor_id,
                g_motor_status.motors[2].motor_id,
                g_motor_status.motors[3].motor_id);
#if MOTOR_STATUS_ELECTRICAL
        logPrintln("\033[2K\rV(mV)|%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].voltage,
                g_motor_status.motors[1].voltage,
                g_motor_status.motors[2].voltage,
                g_motor_status.motors[3].voltage);
        logPrintln("\033[2K\rI(mA)|%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].current,
                g_motor_status.motors[1].current,
                g_motor_status.motors[2].current,
                g_motor_status.motors[3].current);
        logPrintln("\033[2K\rPhI  |%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].phase_current,
                g_motor_status.motors[1].phase_current,
                g_motor_status.motors[2].phase_current,
                g_motor_status.motors[3].phase_current);
        logPrintln("\033[2K\rTemp | %3d | %3d | %3d | %3d |",
                g_motor_status.motors[0].temp,
                g_motor_status.motors[1].temp,
                g_motor_status.motors[2].temp,
                g_motor_status.motors[3].temp);
        #if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
        logPrintln("\033[2K\rBatV |%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].battery_voltage,
                g_motor_status.motors[1].battery_voltage,
                g_motor_status.motors[2].battery_voltage,
                g_motor_status.motors[3].battery_voltage);
        #endif
#endif
#if MOTOR_STATUS_MOTION
        logPrintln("\033[2K\rVel  |%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].vel,
                g_motor_status.motors[1].vel,
                g_motor_status.motors[2].vel,
                g_motor_status.motors[3].vel);
        logPrintln("\033[2K\rPos  |%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].pos,
                g_motor_status.motors[1].pos,
                g_motor_status.motors[2].pos,
                g_motor_status.motors[3].pos);
        logPrintln("\033[2K\rTPos |%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].target_pos,
                g_motor_status.motors[1].target_pos,
                g_motor_status.motors[2].target_pos,
                g_motor_status.motors[3].target_pos);
        logPrintln("\033[2K\rErr  |%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].pos_error,
                g_motor_status.motors[1].pos_error,
                g_motor_status.motors[2].pos_error,
                g_motor_status.motors[3].pos_error);
#endif
#if MOTOR_STATUS_ENCODER
        logPrintln("\033[2K\rEnc  |%5d|%5d|%5d|%5d|",
                g_motor_status.motors[0].encoder_value,
                g_motor_status.motors[1].encoder_value,
                g_motor_status.motors[2].encoder_value,
                g_motor_status.motors[3].encoder_value);
#endif
#if MOTOR_STATUS_STATUS
        logPrintln("\033[2K\rSta  | 0x%02X| 0x%02X| 0x%02X| 0x%02X|",
                g_motor_status.motors[0].status,
                g_motor_status.motors[1].status,
                g_motor_status.motors[2].status,
                g_motor_status.motors[3].status);
        logPrintln("\033[2K\rHom  | 0x%02X| 0x%02X| 0x%02X| 0x%02X|",
                g_motor_status.motors[0].home_status,
                g_motor_status.motors[1].home_status,
                g_motor_status.motors[2].home_status,
                g_motor_status.motors[3].home_status);
#endif
        osDelay(50);

        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break;
        }
    }
    logPrintln("\033[%dA\033[J\033[2A", ++line_count);
}


ShellCommand StepGroup[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, cmd, Motor_cmd_tset, Test Motor Control Command),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, view, Motor_View, View Motor Status),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
step, StepGroup, command group step);