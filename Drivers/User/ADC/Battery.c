/**
 * @file Battery.c
 * @brief 电池电压监控模块源文件
 */

#include "battery.h"
#include "adc.h"
#include "shell.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "Events.h"
#include "shell_cmd_group.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief 电池电压监控数据结构体
 */
typedef struct {
    uint16_t voltage;           // 实际电压
    uint16_t interval_ms;       // 任务间隔
    bool is_init;               // 初始化标志
} BatteryData_t;

static BatteryData_t *g_battery = NULL;

/**
 * @brief 初始化电池电压监控模块
 * @return 初始化结果
 */
bool Battery_Init(void) {
    g_battery = pvPortMalloc(sizeof(BatteryData_t));
    if (g_battery == NULL) {
        return false;
    }
    memset(g_battery, 0, sizeof(BatteryData_t));

    MX_ADC1_Init();
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    g_battery->voltage = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    g_battery->interval_ms = 5000;

    float raw_voltage = (float)g_battery->voltage * ADC_VREF_MV / ADC_RESOLUTION;
    g_battery->voltage = (uint16_t)(raw_voltage * BATTERY_DIVIDER_RATIO);

    if (g_battery->voltage != 0) {
        g_battery->is_init = true;
    }
    return g_battery->is_init;
}

/**
 * @brief 电池电压更新任务
 */
void Battery_Get_Task(void *argument) {
    (void)argument;
    uint16_t voltage = 0;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    if (!g_battery->is_init) vTaskDelete(NULL);

    for (;;) {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&voltage, 1);
        osEventFlagsWait(System_StatusHandle, ADC1_CONVCPLT, osFlagsWaitAny, osWaitForever);
        float voltage_row = (float)voltage * ADC_VREF_MV / ADC_RESOLUTION;
        g_battery->voltage = (uint16_t)(voltage_row * BATTERY_DIVIDER_RATIO);
        osDelay(g_battery->interval_ms);
    }
}

/**
 * @brief 显示当前电池电压
 */
static void Battery_ShowVoltage(void) {
    logPrintln("Battery Voltage: %d mV", g_battery->voltage);
}

/**
 * @brief 实时刷新显示电池电压
 */
static void Battery_ViewRealtime(void) {
    extern Shell shell;
    char ch;

    logPrintln("Battery Voltage Viewer - Press ^C to exit\r\n"
               "Voltage: ----- mV");

    while (1) {
        logPrintln("\033[1A\033[2K\rVoltage: %d mV", g_battery->voltage);
        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break;
        }

        osDelay(100);
    }

    logPrintln("\033[2A\033[J\033[2A");
}

/**
 * @brief 获取或设置电池电压采样间隔
 */
static void Battery_Time_Shell(int argc, char *argv[]) {
    if (argc == 1) {
        logPrintln("Current interval: %d ms", g_battery->interval_ms);
    } else if (argc == 2) {
        char *endptr;
        long val = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0') {
            logPrintln("Invalid time: %s", argv[1]);
        } else {
            g_battery->interval_ms = (uint16_t)val;
            logPrintln("time set to %d ms", g_battery->interval_ms);
        }
    } else {
        logPrintln("Usage: battery time [ms]");
    }
}

/**
 * @brief 电池电压监控模块的 Shell 命令组
 */
ShellCommand BatteryGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN | SHELL_CMD_DISABLE_RETURN, show, Battery_ShowVoltage,
                         show current battery voltage),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN | SHELL_CMD_DISABLE_RETURN, time, Battery_Time_Shell,
                         get/set sampling time (ms)),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC | SHELL_CMD_DISABLE_RETURN, view, Battery_ViewRealtime,
                         realtime view voltage),
    SHELL_CMD_GROUP_END()
};

SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
                       battery, BatteryGroup, Battery Voltage Monitor);
