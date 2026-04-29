/**
 * @file servo_port.c
 * @author MIKE
 * @brief 总线舵机驱动 FreeRTOS 适配层源文件
 */

#include "servo_port.h"
#include "usart.h"
#include "log.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "freeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "Events.h"
#include <string.h>
#include <stdlib.h>

const uint8_t ServoIDList[SERVO_COUNT] = {1, 2, 3, 4, 5, 6};

/**
 * @brief 舵机单圈角度控制接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @param angle 目标角度，范围-180°-180°（单圈绝对位置）
 * @param interval_ms 运动时间，单位ms（0-65535）
 * @param power_mW 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令
 */
bool Servo_ANGLE(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_SET_ANGLE_SHELL,
        .servoId = id,
        .angle = angle,
        .interval = interval_ms,
        .power = power_mW
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机紧急停止接口（立即停止运动） （非阻塞式API）
 * @param id 舵机ID (1-254)
 * @return 命令发送结果，true=发送成功，false=发送失败
 * @note 通过消息队列发送命令
 */
bool Servo_STOP(uint8_t id) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_STOP_SHELL,
        .servoId = id
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

#if SERVO_PING
/**
 * @brief 舵机Ping接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @return 命令发送结果，true=发送成功，false=发送失败
 * @note 通过消息队列发送命令
 */
bool Servo_PING(uint8_t id) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_PING_SHELL,
        .servoId = id
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}
#endif

/**
 * @brief 所有舵机紧急停止接口（批量停止）
 * @note 遍历所有舵机ID列表，循环调用单舵机停止接口，实现全部舵机立即停止
 */
void Servo_STOP_ALL(void) {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        Servo_STOP(ServoIDList[i]);
    }
}

/**
 * @brief 舵机单圈角度控制命令（Shell接口）
 * @param argc 参数个数，必须为3或4
 * @param argv 参数数组
 *            argv[1]: 舵机ID (1~254)
 *            argv[2]: 目标角度 (-180°~180°，单圈范围)
 *            argv[3]: 运动时间（可选，单位ms，默认1000ms）
 * @note 控制舵机转动到指定角度，通过消息队列发送命令给舵机控制任务
 */
static void Servo_Angle_Shell(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        logPrintln("Usage: angle <id> <angle> [time_ms]");
        logPrintln("  id: 1~254, angle: -180~180 (single turn)");
        return;
    }
    uint8_t id = (uint8_t)atoi(argv[1]);
    if (id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
        return;
    }
    Servo_ANGLE(id, (float)atof(argv[2]), 
                   (argc == 4) ? (uint16_t)atoi(argv[3]) : 1000, 1000);
}

/**
 * @brief 舵机停止命令(Shell接口)
 * @param argc 参数个数，必须为2
 * @param argv 参数数组，argv[1]为舵机ID
 * @note 停止指定舵机的当前运动，舵机进入停止状态
 */
static void Servo_Stop_Shell(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: stop <id>");
        return;
    }
    uint8_t id = (uint8_t)atoi(argv[1]);
    if (id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
        return;
    }
    Servo_STOP(id);
}

/**
 * @brief 舵机全部停止命令(Shell接口)
 * @note 停止指定舵机的当前运动，舵机进入停止状态
 */
static void Servo_StopAll_Shell(void) {
    Servo_STOP_ALL();
    logPrintln("All servos stopped");
}

#if SERVO_PING
/**
 * @brief 舵机连通性测试命令(Shell接口)
 * @param argc 参数个数
 * @param argv 参数数组，argv[1]为舵机ID
 * @note 向指定舵机发送Ping命令，检测舵机是否在线响应
 */
static void Servo_Ping_Shell(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: ping <id>");
        return;
    }
    uint8_t id = (uint8_t)atoi(argv[1]);
    if (id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
        return;
    }
    Servo_PING(id);
}
#endif

/**
 * @brief 舵机模块Shell命令
 * @param argc 参数个数，必须为2
 * @param argv 参数数组，argv[1]为命令
 */
static void Servo_Shell(int argc, char *argv[]){
    if(argc < 2) {
        logPrintln(SERVO_CMD_HELP);
        return;
    }

    int sub_argc = argc - 1;
    char **sub_argv = &argv[1];
    
    if(strcmp(argv[1], "angle") == 0) {
        Servo_Angle_Shell(sub_argc, sub_argv);
    } else if(strcmp(argv[1], "stop") == 0) {
        Servo_Stop_Shell(sub_argc, sub_argv);
    } else if(strcmp(argv[1], "stopall") == 0) {
        Servo_StopAll_Shell();
    } else if(strcmp(argv[1], "ping") == 0) {
        Servo_Ping_Shell(sub_argc, sub_argv);
    } else {
        logPrintln("Invalid command: %s", argv[1]);
        logPrintln(SERVO_CMD_HELP);
    }
}

/**
 * @brief 导出舵机模块Shell命令
 */
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
servo, Servo_Shell, servo control commands);

/**
 * @brief 执行舵机控制命令
 * @param cmd 舵机命令结构体指针，包含命令类型、舵机ID、角度、速度等参数
 * @note 支持多种命令类型：角度控制、多圈角度控制、速度控制、阻尼模式、停止、复位等
 */
static void Servo_ExecuteCommand(const ServoCmd_t *cmd) {
    switch (cmd->cmdType) {
        case SERVO_CMD_SET_ANGLE_SHELL:
            if(Servo_SetServoAngle(cmd->servoId, cmd->angle,
                               cmd->interval, cmd->power)==SERVO_STATUS_SUCCESS){
                logPrintln("Servo %d angle set to %.2f", cmd->servoId, cmd->angle);
            }
            break;
        case SERVO_CMD_STOP_SHELL:
            if(Servo_StopOnControlMode(cmd->servoId, 0, 0)==SERVO_STATUS_SUCCESS){
                logPrintln("Servo %d stop", cmd->servoId);
            } else {
                logPrintln("Servo %d stop failed (error %d)", cmd->servoId, cmd->servoId);
            }
            break;
        #if SERVO_PING
        case SERVO_CMD_PING_SHELL:
            if(Servo_Ping(cmd->servoId)==SERVO_STATUS_SUCCESS){
                logPrintln("Servo %d responded", cmd->servoId);
            } else {
                logPrintln("Servo %d no response (error %d)", cmd->servoId, cmd->servoId);
            }
            break;
        #endif
        default:
            break;
    }
}

/**
 * @brief 舵机模块控制任务
 * @param argument 任务参数
 */
void Servo_Ctrl_Task(void *argument) {
    (void)argument;
    ServoCmd_t cmd;
    
    // 等待系统初始化完成
    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    
    for (;;) {
        // 阻塞等待命令
        if (osMessageQueueGet(Servo_CmdHandle, &cmd, NULL, osWaitForever) == osOK) {
            Servo_ExecuteCommand(&cmd);
        }
    }
}

void Servo_Init(void) {
    MX_USART3_UART_Init();
    osEventFlagsClear(System_StatusHandle, UART3_TX_IDLE);
    osEventFlagsClear(System_StatusHandle, UART3_RX_IDLE);
}
