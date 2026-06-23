/**
 * @file track.c
 * @brief 巡线模块源文件
 */

#include "track.h"
#include "shell.h"
#include "log.h"
#include "freeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "shell_cmd_group.h"
#include <string.h>
#include <stdlib.h>
#include "Events.h"
#include "i2c.h"

TrackData_t *g_track = NULL;
static bool is_init = false;

static void Track_Reset(void);
static void Track_Key(void);
static int16_t I2C_ReadDigital(void);


/**
 * @brief 初始化巡线模块
 */
bool Track_Init(void) {
    MX_I2C1_Init();

    g_track = pvPortMalloc(sizeof(TrackData_t));
    if (g_track == NULL) {
        return false;
    }
    memset(g_track, 0, sizeof(TrackData_t));

    g_track->mode = TRACK_STOP;
    g_track->time = 5;
    is_init = true;

    return is_init;
}

/**
 * @brief 巡线模块获取任务
 */
void Track_Get_Task(void *argument) {
    (void)argument;

    extern osMutexId_t Track_MutexHandle;
    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    if (!is_init) vTaskDelete(NULL);

    for(;;) {
        osDelay(g_track->time);

        if (g_track->mode != TRACK_DIGITAL) {
            continue;
        }

        // 通过I2C读取数字量
        int16_t digital = I2C_ReadDigital();
        if (digital != -1) {
            if (osMutexAcquire(Track_MutexHandle, osWaitForever) == osOK) {
                g_track->digitalData = digital;
                g_track->timestamp = xTaskGetTickCount();
                osMutexRelease(Track_MutexHandle);
            }
        }
    }
}

/**
 * @brief 设置巡线模块模式
 * @param mode 模式枚举值
 */
static void Track_Set_Mode(TrackMode_t mode) {
    char ch;
    extern Shell shell;
    uint8_t value;

    if (mode == TRACK_CAL) {
        // 1. 通过I2C进入校准模式
        value = 0x01;
        HAL_I2C_Mem_Write(TRACK_I2C_HANDLE, TRACK_I2C_ADDR << 1, 0x01, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
        logPrintln(TRACK_CAL_HELP_1);

        do {
            while (shell.read(&ch, 1) == 0) { osDelay(100); }
        } while(ch != '\r');
        Track_Key();          // 模拟按下按键，采样黑线

        logPrintln(TRACK_CAL_HELP_2);
        do {
            while (shell.read(&ch, 1) == 0) { osDelay(100); }
        } while(ch != '\r');
        Track_Key();          // 模拟按下按键，采样白线

        // 退出校准模式（实际校准成功后模块会自动退出，但此处确保退出）
        value = 0x00;
        HAL_I2C_Mem_Write(TRACK_I2C_HANDLE, TRACK_I2C_ADDR << 1, 0x01, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
        logPrintln(TRACK_CAL_HELP_3);
        g_track->mode = TRACK_STOP;   // 校准完成后停止数据发送
    } else {
        g_track->mode = mode;
    }
}

/**
 * @brief 设置巡线模块模式
 */
static void Track_Mode_Shell(int argc, char *argv[]) {
    if (!is_init) {
        logWarning("Track module not initialized"); return;
    }

    if(argc != 2) {
        logPrintln(TRACK_MODE_HELP);
        return;
    }
    const char *mode_str;

    if(strcmp(argv[1], "cal") == 0) {
        Track_Set_Mode(TRACK_CAL);
    } else if(strcmp(argv[1], "d") == 0) {
        Track_Set_Mode(TRACK_DIGITAL);
    } else if(strcmp(argv[1], "stop") == 0) {
        Track_Set_Mode(TRACK_STOP);
    } else if(strcmp(argv[1], "rst") == 0) {
        Track_Reset();
    } else if(strcmp(argv[1], "sta") == 0) {
        switch(g_track->mode) {
            case TRACK_CAL: mode_str = "Calibration"; break;
            case TRACK_DIGITAL: mode_str = "Digital"; break;
            case TRACK_STOP: mode_str = "Stop"; break;
            default: mode_str = "Unknown"; break;
        }
        logPrintln("Status: %s , Time: %d ms", mode_str, g_track->time);
    } else {
        logPrintln("invalid command: %s\n%s", argv[1], TRACK_MODE_HELP);
    }
}

/**
 * @brief 查看或设置巡线模块发送时间间隔
 */
static void Track_Time_Shell(int argc, char *argv[]) {
    if (!is_init) {
        logWarning("Track module not initialized"); return;
    }

    if(argc > 2) {
        logPrintln(TRACK_TIME_HELP); return;
    } else if(argc == 1) {
        logPrintln("current time: %d ms", g_track->time); return;
    }

    char *endptr;
    long val = strtol(argv[1], &endptr, 10);
    if(*endptr != '\0') {
        logPrintln("invalid time value: %s", argv[1]);
        return;
    } else {
        g_track->time = (uint16_t)val;
    }
}

/**
 * @brief 实时刷新巡线模块数据
 */
static void Track_View_Shell(void) {
    char ch;
    extern Shell shell;

    if (!is_init) {
        logWarning("Track module not initialized"); return;
    }

    logPrintln("Track Data Viewer - Press ^C to exit\r\n"
               "Digital: - - - - - - - -\r\n"
               "Timestamp:");

    for(;;) {
        logPrintln("\033[2A\033[2K\rDigital: %d %d %d %d %d %d %d %d\r\n"
                   "Timestamp: %d",
                    (g_track->digitalData & 0x80) ? 0 : 1,
                    (g_track->digitalData & 0x40) ? 0 : 1,
                    (g_track->digitalData & 0x20) ? 0 : 1,
                    (g_track->digitalData & 0x10) ? 0 : 1,
                    (g_track->digitalData & 0x08) ? 0 : 1,
                    (g_track->digitalData & 0x04) ? 0 : 1,
                    (g_track->digitalData & 0x02) ? 0 : 1,
                    (g_track->digitalData & 0x01) ? 0 : 1,
                    g_track->timestamp);
        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break; // ^C
        }
        osDelay(100);
    }
    logPrintln("\033[3A\033[J\033[2A");
}

ShellCommand TrackGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, mode, Track_Mode_Shell, set track mode),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, time, Track_Time_Shell, view or set track time),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, view, Track_View_Shell, view track data),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       track, TrackGroup, Track Tool Group);

/**
 * @brief 重置巡线模块
 */
static void Track_Reset(void) {
    HAL_GPIO_WritePin(TRACK_RST_Port, TRACK_RST_Pin, GPIO_PIN_RESET);
    osDelay(500);
    HAL_GPIO_WritePin(TRACK_RST_Port, TRACK_RST_Pin, GPIO_PIN_SET);
    osDelay(100);
}

/**
 * @brief 模拟按下巡线模块按键（KEY1）
 */
static void Track_Key(void) {
    HAL_GPIO_WritePin(TRACK_KEY_Port, TRACK_KEY_Pin, GPIO_PIN_RESET);
    osDelay(100);
    HAL_GPIO_WritePin(TRACK_KEY_Port, TRACK_KEY_Pin,    GPIO_PIN_SET);
    osDelay(100);
}

/**
 * @brief 通过I2C读取数字量寄存器（0x30）
 * @return 读取到的数字值，失败返回-1
 */
static int16_t I2C_ReadDigital(void) {
    int16_t value = 0;
    if (HAL_I2C_Mem_Read(TRACK_I2C_HANDLE, TRACK_I2C_ADDR << 1, 0x30, I2C_MEMADD_SIZE_8BIT, (uint8_t*)&value, 1, 30) != HAL_OK) {
        return -1;
    }
    return value;
}
