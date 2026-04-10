/**
 * @file track.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 巡线模块源文件
 */

#include "track.h"
#include "shell.h"
#include "log.h"
#include "freeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "usart.h"
#include "shell_cmd_group.h"
#include <string.h>
#include <stdlib.h>
#include "Events.h"

Track_t track;
uint8_t trackBuffer[ALL_LEN];

static void Track_Key(void);



/**
 * @brief 初始化巡线模块
 */
static void Track_Init(void) {
    MX_USART2_UART_Init();
    osEventFlagsClear(System_StatusHandle, UART2_RX_CPLT);

    track.mode = TRACK_STOP;
    track.time = 1000;
}

/**
 * @brief 巡线模块获取任务
 * @param argument 任务参数
 */
void Track_Get_Task(void *argument) {
    (void)argument;

    // 等待系统初始化完成
    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    Track_Init();

    for(;;) {
        osDelay(track.time);
        switch(track.mode) {
            case TRACK_STOP:
                HAL_UART_Transmit(&huart2, CMD_STOP, 7, 500); continue;
            case TRACK_DIGITAL:
                HAL_UART_Transmit(&huart2, CMD_DIGITAL, 7, 500); break;
            case TRACK_ANALOG:
                HAL_UART_Transmit(&huart2, CMD_ANALOG, 7, 500); break;
            case TRACK_ALL:
                HAL_UART_Transmit(&huart2, CMD_ALL, 7, 500); break;
            default: continue;
        }

        HAL_UART_Receive_DMA(&huart2, trackBuffer, Track_Get_Size());
        osEventFlagsWait(System_StatusHandle, UART2_RX_CPLT, osFlagsWaitAny, osWaitForever);
        HAL_UART_Transmit(&huart2, CMD_STOP, 7, 500);
        osEventFlagsClear(System_StatusHandle, UART2_RX_CPLT);

        // 临时调试用输出
       logPrintln("\033[1A\033[2K\r%s", trackBuffer);
    }
}

/**
 * @brief 获取巡线模块当前模式的发送数据长度
 * @return 发送数据长度
 */
uint8_t Track_Get_Size(void) {
    switch(track.mode) {
        case TRACK_DIGITAL: return DIGITAL_LEN;
        case TRACK_ANALOG: return ANALOG_LEN;
        case TRACK_ALL: return ALL_LEN;
        default: return DIGITAL_LEN;
    }
}

/**
 * @brief 设置巡线模块模式
 * @param mode 巡线模块模式枚举值
 */
void Track_Set_Mode(TrackSet_t mode) {
    char ch;
    extern Shell shell;
    track.mode = mode;

    if(mode == TRACK_CAL) {
        HAL_UART_Transmit(&huart2, CMD_CAL, strlen(CMD_CAL), 500);

        logPrintln("Calibration Mode Started\r\n" \
                    "1. When red light is on, place all sensors on black line for 3s, then press Enter");

        do {
            while (shell.read(&ch, 1) == 0) {
                osDelay(100);
            }
        } while(ch != '\r');

        Track_Key();

        logPrintln("2. Place all sensors on white line for 3s, then press Enter");

        do {
            while (shell.read(&ch, 1) == 0) {
                osDelay(100);
            }
        } while(ch != '\r');

        Track_Key();

        logPrintln("Calibration completed! Check red light status:\r\n" \
                    "- Red light off: Calibration success\r\n" \
                    "- Red light slow blink: Need recalibration");
    }
}

/**
 * @brief 设置巡线模块发送时间间隔
 * @param time 时间间隔(ms)
 */
static void Track_Set_Time(uint16_t time) {
    track.time = time;
}

/**
 * @brief 重置巡线模块
 */
static void Track_Reset(void) {
    HAL_GPIO_WritePin(TRACK_RST_Port, TRACK_RST_PIN, GPIO_PIN_RESET);
    osDelay(500);
    HAL_GPIO_WritePin(TRACK_RST_Port, TRACK_RST_PIN, GPIO_PIN_SET);
}

/**
 * @brief 按下巡线模块按键
 */
static void Track_Key(void) {
    HAL_GPIO_WritePin(TRACK_KEY_Port, TRACK_KEY_PIN, GPIO_PIN_RESET);
    osDelay(100);
    HAL_GPIO_WritePin(TRACK_KEY_Port, TRACK_KEY_PIN, GPIO_PIN_SET);
}

/**
 * @brief 设置巡线模块模式
 * @param argc 参数数量
 * @param argv 参数列表
 */
static void Track_Set_Sell(int argc, char *argv[]) {
    if(argc != 3) {
        logPrintln(TRACK_SET_HELP);
        return;
    }

    if(strcmp(argv[1], "mode") == 0) {
        if(strcmp(argv[2], "cal") == 0) {
            Track_Set_Mode(TRACK_CAL);
        } else if(strcmp(argv[2], "d") == 0) {
            Track_Set_Mode(TRACK_DIGITAL);
        } else if(strcmp(argv[2], "a") == 0) {
            Track_Set_Mode(TRACK_ANALOG);
        } else if(strcmp(argv[2], "all") == 0) {
            Track_Set_Mode(TRACK_ALL);
        } else if(strcmp(argv[2], "stop") == 0) {
            Track_Set_Mode(TRACK_STOP);
        } else if(strcmp(argv[2], "rst") == 0) {
            Track_Reset();
        } else {
            logPrintln("invalid choice: %s", argv[2]);
            logPrintln(TRACK_SET_HELP);
        }
    } else if(strcmp(argv[1], "time") == 0) {
        // 判断第二个值是否为数字
        char *endptr;
        long val = strtol(argv[2], &endptr, 10);
        if(*endptr != '\0') {
            logPrintln("invalid time value: %s", argv[2]);
            logPrintln(TRACK_SET_HELP);
        } else {
            Track_Set_Time((uint16_t)val);
        }
    }
}

/**
 * @brief 实时刷新巡线模块数据
 */
static void Track_View_Sell(void) {
    logPrintln("Not implemented yet");
}

ShellCommand TrackGroup[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, view, Track_View_Sell, view track data),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, set, Track_Set_Sell, set track mode or time),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
track, TrackGroup, command group track);
