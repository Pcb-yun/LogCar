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

static Track_t track;
static uint8_t trackBuffer[TRACK_ALL_LEN];

static void Track_Reset(void);
static void Track_Key(void);
static uint8_t Track_Get_Size(void);
static void Track_Parse(uint8_t *buffer, TrackData_t *data);


/**
 * @brief 巡线模块获取任务
 * @param argument 任务参数
 */
void Track_Get_Task(void *argument) {
    (void)argument;
    extern osMessageQueueId_t Track_DataHandle;
    TrackData_t trackData;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for(;;) {
        osDelay(track.time);
        switch(track.mode) {
            case TRACK_STOP:
                HAL_UART_Transmit(&huart2, TRACK_CMD_STOP, 7, TRACK_TIMEOUT); continue;
            case TRACK_DIGITAL:
                HAL_UART_Transmit(&huart2, TRACK_CMD_DIGITAL, 7, TRACK_TIMEOUT); break;
            case TRACK_ANALOG:
                HAL_UART_Transmit(&huart2, TRACK_CMD_ANALOG, 7, TRACK_TIMEOUT); break;
            case TRACK_ALL:
                HAL_UART_Transmit(&huart2, TRACK_CMD_ALL, 7, TRACK_TIMEOUT); break;
            default: continue;
        }

        HAL_UART_Receive_DMA(&huart2, trackBuffer, Track_Get_Size());
        osEventFlagsWait(System_StatusHandle, UART2_RX_CPLT, osFlagsWaitAny, osWaitForever);
        HAL_UART_Transmit(&huart2, TRACK_CMD_STOP, 7, TRACK_TIMEOUT);
        osEventFlagsClear(System_StatusHandle, UART2_RX_CPLT);

        Track_Parse(trackBuffer, &trackData);
        osMessageQueueReset(Track_DataHandle);
        osMessageQueuePut(Track_DataHandle, &trackData, 0, 0);
    }
}

/**
 * @brief 设置巡线模块模式
 * @param mode 巡线模块模式枚举值
 */
static void Track_Set_Mode(TrackSet_t mode) {
    char ch;
    extern Shell shell;
    track.mode = mode;

    if(mode == TRACK_CAL) {
        HAL_UART_Transmit(&huart2, TRACK_CMD_CAL, 7, TRACK_TIMEOUT);
        logPrintln(TRACK_CAL_HELP_1);

        do {
            while (shell.read(&ch, 1) == 0) {
                osDelay(100);
            }
        } while(ch != '\r');

        Track_Key();
        logPrintln(TRACK_CAL_HELP_2);

        do {
            while (shell.read(&ch, 1) == 0) {
                osDelay(100);
            }
        } while(ch != '\r');

        Track_Key();
        logPrintln(TRACK_CAL_HELP_3);
    }
}

/**
 * @brief 设置巡线模块模式
 * @param argc 参数数量
 * @param argv 参数列表
 */
static void Track_Mode_Shell(int argc, char *argv[]) {
    if(argc != 2) {
        logPrintln(TRACK_MODE_HELP);
        return;
    }
    const char *mode_str;

    if(strcmp(argv[1], "cal") == 0) {
        Track_Set_Mode(TRACK_CAL);
    } else if(strcmp(argv[1], "d") == 0) {
        Track_Set_Mode(TRACK_DIGITAL);
    } else if(strcmp(argv[1], "a") == 0) {
        Track_Set_Mode(TRACK_ANALOG);
    } else if(strcmp(argv[1], "all") == 0) {
        Track_Set_Mode(TRACK_ALL);
    } else if(strcmp(argv[1], "stop") == 0) {
        Track_Set_Mode(TRACK_STOP);
    } else if(strcmp(argv[1], "rst") == 0) {
        Track_Reset();
    } else if(strcmp(argv[1], "sta") == 0) {
        switch(track.mode) {
            case TRACK_CAL: mode_str = "Calibration"; break;
            case TRACK_ANALOG: mode_str = "Analog"; break;
            case TRACK_DIGITAL: mode_str = "Digital"; break;
            case TRACK_ALL: mode_str = "All"; break;
            case TRACK_STOP: mode_str = "Stop"; break;
            default: mode_str = "Unknown"; break;
        }
        logPrintln("Status: %s , Time: %d", mode_str, track.time);
    } else {
        logPrintln("invalid choice: %s", argv[1]);
        logPrintln(TRACK_MODE_HELP);
    }
}

/**
 * @brief 设置巡线模块发送时间间隔
 * @param argc 参数数量
 * @param argv 参数列表
 */
static void Track_Time_Sell(int argc, char *argv[]) {
    if(argc != 2) {
        logPrintln(TRACK_TIME_HELP);
        return;
    }

    // 判断参数是否为数字
    char *endptr;
    long val = strtol(argv[1], &endptr, 10);
    if(*endptr != '\0') {
        logPrintln("invalid time value: %s", argv[1]);
        logPrintln(TRACK_TIME_HELP);
        return;
    } else {
        track.time = (uint16_t)val;
    }
}

/**
 * @brief 实时刷新巡线模块数据
 */
static void Track_View_Sell(void) {
    extern osMessageQueueId_t Track_DataHandle;
    TrackData_t trackData;
    char ch;
    extern Shell shell;

    logPrintln("Track Data Viewer - Press ^C to exit");
    logPrintln("Analog : - - - - - - - -\r\nDigital: - - - - - - - -");

    for(;;) {
        if (osMessageQueueGet(Track_DataHandle, &trackData, NULL, 50) == osOK) {
            switch(trackData.mode) {
                case TRACK_DIGITAL:
                    logPrintln("\033[1A\033[2K\rDigital: %d %d %d %d %d %d %d %d",
                            (trackData.digitalData & 0x80) ? 1 : 0,
                            (trackData.digitalData & 0x40) ? 1 : 0,
                            (trackData.digitalData & 0x20) ? 1 : 0,
                            (trackData.digitalData & 0x10) ? 1 : 0,
                            (trackData.digitalData & 0x08) ? 1 : 0,
                            (trackData.digitalData & 0x04) ? 1 : 0,
                            (trackData.digitalData & 0x02) ? 1 : 0,
                            (trackData.digitalData & 0x01) ? 1 : 0);
                    break;
                case TRACK_ANALOG:
                    logPrintln("\033[2A\033[2K\rAnalog : %d %d %d %d %d %d %d %d\r\n",
                            trackData.analogData[0], trackData.analogData[1],
                            trackData.analogData[2], trackData.analogData[3],
                            trackData.analogData[4], trackData.analogData[5],
                            trackData.analogData[6], trackData.analogData[7]);
                    break;
                case TRACK_ALL:
                    logPrintln("\033[2A\033[2K\rAnalog : %d %d %d %d %d %d %d %d",
                        trackData.analogData[0], trackData.analogData[1],
                        trackData.analogData[2], trackData.analogData[3],
                        trackData.analogData[4], trackData.analogData[5],
                        trackData.analogData[6], trackData.analogData[7]);
                    logPrintln("\033[2K\rDigital: %d %d %d %d %d %d %d %d",
                        (trackData.digitalData & 0x80) ? 1 : 0,
                        (trackData.digitalData & 0x40) ? 1 : 0,
                        (trackData.digitalData & 0x20) ? 1 : 0,
                        (trackData.digitalData & 0x10) ? 1 : 0,
                        (trackData.digitalData & 0x08) ? 1 : 0,
                        (trackData.digitalData & 0x04) ? 1 : 0,
                        (trackData.digitalData & 0x02) ? 1 : 0,
                        (trackData.digitalData & 0x01) ? 1 : 0);
                    break;
            }
        }

        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break; // ^C
        }
    }
    logPrintln("\033[3A\033[J\033[2A");
}

ShellCommand TrackGroup[] =
{
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, mode, Track_Mode_Shell, set track mode),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, time, Track_Time_Sell, set track time),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, view, Track_View_Sell, view track data),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
track, TrackGroup, command group track);

/**
 * @brief 初始化巡线模块
 */
void Track_Init(void) {
    MX_USART2_UART_Init();
    osEventFlagsClear(System_StatusHandle, UART2_RX_CPLT);

    track.mode = TRACK_STOP;
    track.time = 500;
}

/**
 * @brief 获取巡线模块当前模式的发送数据长度
 * @return 发送数据长度
 */
static uint8_t Track_Get_Size(void) {
    switch(track.mode) {
        case TRACK_DIGITAL: return TRACK_DIGITAL_LEN;
        case TRACK_ANALOG: return TRACK_ANALOG_LEN;
        case TRACK_ALL: return TRACK_ALL_LEN;
        default: return TRACK_DIGITAL_LEN;
    }
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
 * @brief 解析巡线模块数据
 * @param buffer 接收缓冲区
 * @param data 解析后的数据结构体
 */
static void Track_Parse(uint8_t *buffer, TrackData_t *data) {
    char *ptr = (char *)buffer;
    uint8_t index;
    data->mode = track.mode;

    switch(track.mode) {
        case TRACK_DIGITAL:
            if (ptr[0] == '$' && ptr[1] == 'D') {
                data->digitalData = 0;
                if (ptr[6 ] == '1') data->digitalData |= (1 << 7);
                if (ptr[11] == '1') data->digitalData |= (1 << 6);
                if (ptr[16] == '1') data->digitalData |= (1 << 5);
                if (ptr[21] == '1') data->digitalData |= (1 << 4);
                if (ptr[26] == '1') data->digitalData |= (1 << 3);
                if (ptr[31] == '1') data->digitalData |= (1 << 2);
                if (ptr[36] == '1') data->digitalData |= (1 << 1);
                if (ptr[41] == '1') data->digitalData |= (1 << 0);
            } break;
        case TRACK_ANALOG:
            if (ptr[0] == '$' && ptr[1] == 'A') {
                ptr += 3;
                for (index = 0; index < 8; index++) {
                    while (*ptr != ':') ptr++;
                    ptr++;
                    data->analogData[index] = 0;
                    while (*ptr >= '0' && *ptr <= '9') {
                        data->analogData[index] = data->analogData[index] * 10 + (*ptr - '0');
                        ptr++;
                    }
                    if (*ptr == ',') ptr++;
                }
            } break;
        case TRACK_ALL:
            if (ptr[0] == '$' && ptr[1] == 'A') {
                ptr += 3;
                for (index = 0; index < 8; index++) {
                    while (*ptr != ':') ptr++;
                    ptr++;
                    data->analogData[index] = 0;
                    while (*ptr >= '0' && *ptr <= '9') {
                        data->analogData[index] = data->analogData[index] * 10 + (*ptr - '0');
                        ptr++;
                    }
                    if (*ptr == ',') ptr++;
                }
                while (*ptr != '$' && *ptr != '\0') ptr++;
            }
            if (*ptr == '$' && *(ptr + 1) == 'D') {
                data->digitalData = 0;
                if (ptr[6 ] == '1') data->digitalData |= (1 << 7);
                if (ptr[11] == '1') data->digitalData |= (1 << 6);
                if (ptr[16] == '1') data->digitalData |= (1 << 5);
                if (ptr[21] == '1') data->digitalData |= (1 << 4);
                if (ptr[26] == '1') data->digitalData |= (1 << 3);
                if (ptr[31] == '1') data->digitalData |= (1 << 2);
                if (ptr[36] == '1') data->digitalData |= (1 << 1);
                if (ptr[41] == '1') data->digitalData |= (1 << 0);
            } break;
    }
}
