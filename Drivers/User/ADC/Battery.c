#include "Battery.h"
#include "adc.h"               // 包含 hadc1, MX_ADC1_Init
#include "shell.h"
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "shell_cmd_group.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------- 静态变量 -------------------------- */
static uint16_t battery_adc_raw = 0;          // DMA 实时更新的原始值 (0~4095)
static float battery_voltage = 0.0f;          // 实际电压（已考虑分压比）
static uint16_t battery_interval_ms = 500;    // 任务间隔（ms），可动态修改

/**
 * @brief 电池电压监控模块的配置宏
 */
// 分压比：若电池通过电阻分压后接入 ADC，则设为 (R1+R2)/R2；否则为 1.0
#define BATTERY_DIVIDER_RATIO   1.0f

// ADC 参考电压 (VREF+)，通常为 3.3V
#define ADC_VREF                3.3f

// ADC 分辨率 (12位)
#define ADC_RESOLUTION          4095.0f

/* -------------------------- 函数声明 -------------------------- */
static void Battery_UpdateVoltage(void);       // 根据原始值计算实际电压
static void Battery_ShowVoltage(void);         // Shell 输出一次电压
static void Battery_ViewRealtime(void);        // Shell 实时刷新视图


static void Battery_Cmd_Show(int argc, char *argv[]);
static void Battery_Cmd_Interval(int argc, char *argv[]);
static void Battery_Cmd_View(int argc, char *argv[]);

/**
 * @brief 电池电压监控模块的 Shell 命令组
 */
ShellCommand BatteryGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN | SHELL_CMD_DISABLE_RETURN,
                         show, Battery_Cmd_Show,
                         show current battery voltage),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN | SHELL_CMD_DISABLE_RETURN,
                         interval, Battery_Cmd_Interval,
                         get/set sampling interval (ms)),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC | SHELL_CMD_DISABLE_RETURN,
                         view, Battery_Cmd_View,
                         realtime view voltage (press ^C to exit)),
    SHELL_CMD_GROUP_END()
};

// 导出命令组，在 Shell 中输入 "battery" 即可看到子命令
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
                       battery, BatteryGroup, Battery Voltage Monitor);

/**
 * @brief 初始化电池电压监控模块
 */
void Battery_Init(void)
{
    // 1. 初始化 ADC 硬件（时钟、GPIO、DMA 等，由 CubeMX 生成）
    MX_ADC1_Init();

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&battery_adc_raw, 1);

    Battery_UpdateVoltage();
}

/**
 * @brief 电池电压监控模块的 FreeRTOS 任务
 */
void Battery_Get_Task(void *argument)
{
    (void)argument;

    TickType_t last_wake = xTaskGetTickCount();
    for (;;)
    {
        Battery_UpdateVoltage();

        // 按设置的间隔周期性运行（释放 CPU）
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(battery_interval_ms));
    }
}

/* -------------------------- 内部函数 -------------------------- */
static void Battery_UpdateVoltage(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    battery_adc_raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    float raw_voltage = (float)battery_adc_raw * ADC_VREF / ADC_RESOLUTION;
    battery_voltage = raw_voltage * BATTERY_DIVIDER_RATIO;
}

static void Battery_ShowVoltage(void)
{
    logPrintln("Battery Voltage: %.2f V", battery_voltage);
}

/* 实时刷新显示（类似 top 命令，按 Ctrl+C 退出） */
static void Battery_ViewRealtime(void)
{
    extern Shell shell;
    char ch;

    logPrintln("Battery Voltage Viewer - Press ^C to exit\r\n"
               "Voltage: --- V");

    while (1)
    {
        Battery_UpdateVoltage();
        // 覆盖上一行并重绘
        logPrintln("\033[1A\033[2K\rVoltage: %.2f V", battery_voltage);
        // 检测按键，若收到 Ctrl+C (0x03) 则退出
        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break;
        }

        osDelay(100);
    }

    logPrintln("\033[1A\033[J"); // 清除最后一行，恢复提示符
}

/* -------------------------- Shell 命令实现 -------------------------- */
static void Battery_Cmd_Show(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    Battery_ShowVoltage();
}

static void Battery_Cmd_Interval(int argc, char *argv[])
{
    if (argc == 1) {
        logPrintln("Current interval: %d ms", battery_interval_ms);
    } else if (argc == 2) {
        char *endptr;
        long val = strtol(argv[1], &endptr, 10);
        if (*endptr != '\0' || val <= 0 || val > 10000) {
            logPrintln("Invalid interval (must be 1~10000 ms)");
        } else {
            battery_interval_ms = (uint16_t)val;
            logPrintln("Interval set to %d ms", battery_interval_ms);
        }
    } else {
        logPrintln("Usage: battery interval [ms]");
    }
}

static void Battery_Cmd_View(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    Battery_ViewRealtime();
}

/* -------------------------- 对外接口 -------------------------- */
float Battery_GetVoltage(void)
{
    return battery_voltage;
}

void Battery_SetInterval(uint16_t ms)
{
    battery_interval_ms = ms;
}

uint16_t Battery_GetInterval(void)
{
    return battery_interval_ms;
}