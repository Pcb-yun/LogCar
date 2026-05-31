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
#include <stdbool.h>


static uint16_t battery_voltage;          // 实际电压
static uint16_t battery_interval_ms;      // 任务间隔
static bool is_init = false;


/**
 * @brief 初始化电池电压监控模块
 */
bool Battery_Init(void) {
    MX_ADC1_Init();
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    battery_voltage = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    battery_interval_ms = 5000;

    float raw_voltage = (float)battery_voltage * ADC_VREF_MV / ADC_RESOLUTION;
    battery_voltage = (uint16_t)(raw_voltage * BATTERY_DIVIDER_RATIO);

    if (battery_voltage != 0) {
        is_init = true;
    }
    return is_init;
}

/**
 * @brief 电池电压更新任务
 */
void Battery_Get_Task(void *argument) {
    (void)argument;
    uint16_t voltage = 0;
    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    if (!is_init) {
        vTaskDelete(NULL);
        return;
    }

    for (;;) {
        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&voltage, 1);
        osEventFlagsWait(System_StatusHandle, BATTERY_UPDATE, osFlagsWaitAny, osWaitForever);
        float voltage_row = (float)voltage * ADC_VREF_MV / ADC_RESOLUTION;
        battery_voltage = (uint16_t)(voltage_row * BATTERY_DIVIDER_RATIO);
        osDelay(battery_interval_ms);
    }
}

/**
 * @brief 显示当前电池电压
 */
static void Battery_ShowVoltage(void) {
    logPrintln("Battery Voltage: %d mV", battery_voltage);
}

/**
 * @brief 实时刷新显示电池电压
 */
static void Battery_ViewRealtime(void) {
    extern Shell shell;
    char ch;

    logPrintln("Battery Voltage Viewer - Press ^C to exit\r\n"
               "Voltage: --- mV");

    while (1) {
        logPrintln("\033[1A\033[2K\rVoltage: %d mV", battery_voltage);
        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break;
        }

        osDelay(100);
    }

    logPrintln("\033[3A\033[J\033[2A");
}

/**
 * @brief 获取或设置电池电压采样间隔
 */
static void Battery_Time_Shell(int argc, char *argv[]) {
    if (argc == 1) {
        logPrintln("Current interval: %d ms", battery_interval_ms);
    } else if (argc == 2) {
        char *endptr;
        long val = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0') {
            logPrintln("Invalid time: %s", argv[1]);
        } else {
            battery_interval_ms = (uint16_t)val;
            logPrintln("time set to %d ms", battery_interval_ms);
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
