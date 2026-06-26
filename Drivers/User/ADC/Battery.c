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
#include "shell.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief 电池电压监控数据结构体
 */
typedef struct {
    uint16_t voltage;           // 实际电压
    uint16_t interval_ms;       // 任务间隔
    uint32_t last_warn_tick;  // 上次警告时间戳
} BatteryData_t;

static BatteryData_t *g_battery = NULL;
static bool is_init = false;

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

    g_battery->interval_ms = 200;

    float raw_voltage = (float)g_battery->voltage * ADC_VREF_MV / ADC_RESOLUTION;
    g_battery->voltage = (uint16_t)(raw_voltage * BATTERY_DIVIDER_RATIO + BATTERY_OFFSET);

    if (g_battery->voltage >= 3300) {
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
    if (!is_init) vTaskDelete(NULL);

    for (;;) {
        uint32_t sum = 0;
        for (int i = 0; i < ADC_SAMPLE_COUNT; i++) {
            HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&voltage, 1);
            osEventFlagsWait(System_StatusHandle, ADC1_CONVCPLT, osFlagsWaitAny, osWaitForever);
            sum += voltage;
        }
        float avg_raw = (float)sum / ADC_SAMPLE_COUNT;
        float voltage_row = avg_raw * ADC_VREF_MV / ADC_RESOLUTION;
        g_battery->voltage = (uint16_t)(voltage_row * BATTERY_DIVIDER_RATIO + BATTERY_OFFSET);
        if (g_battery->voltage <= BATTERY_THRESHOLD) {
            uint32_t now = xTaskGetTickCount();
            if (now - g_battery->last_warn_tick >= 30000) {
                g_battery->last_warn_tick = now;
                logWarning("Low battery voltage: %d mV", g_battery->voltage);
            }
        }
        osDelay(g_battery->interval_ms);
    }
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
 * @brief 电池工具
 */
static void Battery_Tool(int argc, char *argv[]) {
    if (!is_init) {
        logWarning("Battery module not initialized"); return;
    }

    if (argc == 1) {
        logPrintln("Battery Voltage: %d mV", g_battery->voltage);
    } else if (argc <= 3) {
        if (strcmp(argv[1], "time") == 0){
            if (argc == 2) {
                logPrintln("Current interval: %d ms", g_battery->interval_ms);
            } else if (argc == 3) {
                char *endptr;
                long val = strtol(argv[2], &endptr, 10);
                if (*endptr != '\0') {
                    logPrintln("Invalid time: %s", argv[2]);
                } else {
                    g_battery->interval_ms = (uint16_t)val;
                    logPrintln("time set to %d ms", g_battery->interval_ms);
                }
            } else {
                logPrintln("Usage: battery time [ms]");
            }
        }
        if (strcmp(argv[1], "view") == 0){
            Battery_ViewRealtime();
        }
    } else {
        logPrintln("Usage: battery time|view");
    }
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
battery, Battery_Tool, Battery Tool);
