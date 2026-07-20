/**
 * @file sensor.c
 * @brief TCS230 颜色传感器驱动
 *
 * 引脚连接:
 *   SENSOR_OUT (PE1) — TCS230 OUT (频率输出)
 *   SENSOR_S2  (PE2) — TCS230 S2 (滤波器选择)
 *   SENSOR_S3  (PE3) — TCS230 S3 (滤波器选择)
 *   S0, S1 外部硬件固定为 H (100% 频率标定)
 *
 * 使用 TIM2 (1MHz, 32-bit 自由运行计数器) 测量输出频率.
 */

#include "sensor.h"
#include "gpio.h"
#include "shell.h"
#include "log.h"
#include "shell_cmd_group.h"
#include <string.h>
#include "Events.h"
#include "tim.h"
#include "main.h"
#include <math.h>

/** 频率测量超时 (TIM2 ticks, 1 tick = 1us) */
#define TCS230_TIMEOUT_US  1000000UL

/** 滤波器切换后稳定等待时间 (us) */
#define TCS230_SETTLE_US   2000UL

/**
 * @brief 白色平衡参数
 */
TCS230_RGBC_t rgbc_wb = {0};

/**
 * @brief 颜色参数
 */
TCS230_RGBC_t rgb_color = {0};

bool isWB = false;


/**
 * @brief 使用 TIM2 微秒忙等待
 */
static inline void delay_us(uint32_t us) {
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us);
}

/**
 * @brief 测量 SENSOR_OUT 引脚的频率
 * @return 频率，单位为 Hz，0 表示超时
 *
 * TIM2 在 1 MHz, 32-bit
 */
static uint32_t measure_frequency(void) {
    uint32_t t1, t2;
    uint32_t start;

    /* 同步：等待引脚低电平 */
    start = TIM2->CNT;
    while (HAL_GPIO_ReadPin(SENSOR_OUT_GPIO_Port, SENSOR_OUT_Pin) != GPIO_PIN_RESET) {
        if ((TIM2->CNT - start) > TCS230_TIMEOUT_US) return 0;
    }

    /* 上升沿边：等待引脚高电平（周期开始） */
    start = TIM2->CNT;
    while (HAL_GPIO_ReadPin(SENSOR_OUT_GPIO_Port, SENSOR_OUT_Pin) == GPIO_PIN_RESET) {
        if ((TIM2->CNT - start) > TCS230_TIMEOUT_US) return 0;
    }
    t1 = TIM2->CNT;

    /* 下降沿边：等待引脚低电平（周期结束） */
    start = TIM2->CNT;
    while (HAL_GPIO_ReadPin(SENSOR_OUT_GPIO_Port, SENSOR_OUT_Pin) != GPIO_PIN_RESET) {
        if ((TIM2->CNT - start) > TCS230_TIMEOUT_US) return 0;
    }

    /* 上升沿边：等待引脚高电平（周期结束） */
    start = TIM2->CNT;
    while (HAL_GPIO_ReadPin(SENSOR_OUT_GPIO_Port, SENSOR_OUT_Pin) == GPIO_PIN_RESET) {
        if ((TIM2->CNT - start) > TCS230_TIMEOUT_US) return 0;
    }
    t2 = TIM2->CNT;

    uint32_t period = t2 - t1;
    if (period == 0) return 0;

    return 1000000UL / period;
}

/**
 * @brief 初始化 TCS230 颜色传感器
 */
void SENSOR_Init(void) {
    /* 默认滤波器：清除 */
    SENSOR_SetFilter(TCS230_FILTER_CLEAR);
}

/**
 * @brief 设置 TCS230 颜色传感器的滤波器
 * @param filter 滤波器选择
 */
void SENSOR_SetFilter(TCS230_Filter_t filter) {
    HAL_GPIO_WritePin(SENSOR_S2_GPIO_Port, SENSOR_S2_Pin,
                      (filter & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SENSOR_S3_GPIO_Port, SENSOR_S3_Pin,
                      (filter & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief 读取 TCS230 颜色传感器的指定通道频率
 * @param filter 滤波器选择
 * @return 频率，单位为 Hz，0 表示超时
 */
uint32_t SENSOR_ReadChannel(TCS230_Filter_t filter) {
    SENSOR_SetFilter(filter);
    delay_us(TCS230_SETTLE_US);
    return measure_frequency();
}

/**
 * @brief 读取 TCS230 颜色传感器的所有通道频率
 * @param out 输出结构体指针，用于存储读取到的频率
 */
void SENSOR_ReadAll(TCS230_RGBC_t *out) {
    out->clear = SENSOR_ReadChannel(TCS230_FILTER_CLEAR);
    out->red   = SENSOR_ReadChannel(TCS230_FILTER_RED);
    out->green = SENSOR_ReadChannel(TCS230_FILTER_GREEN);
    out->blue  = SENSOR_ReadChannel(TCS230_FILTER_BLUE);
}

/**
 * @brief 读取 TCS230 颜色传感器的所有通道频率并打印到串口
 */
static void SENSOR_Read_Shell(void) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    uint8_t byte;
    TCS230_RGBC_t rgbc;

    SENSOR_Init();

    logPrintln("TCS230 Reader - Press ^C to exit\r\n"
               "  R       G       B       C");

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    for (;;) {
        osDelay(200);
        SENSOR_ReadAll(&rgbc);
        logPrintln("\033[1A\033[2K\r%5lu  %5lu  %5lu  %5lu Hz",
                   rgbc.red, rgbc.green, rgbc.blue, rgbc.clear);

        if (osMessageQueueGet(Usart1_Rx_DataHandle, &byte, NULL, 0) == osOK) {
            if (byte == 0x03) break;
        }
    }

    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("\033[1A\033[2K\r");
}

/**
 * @brief 读取 TCS230 颜色传感器的所有通道频率并打印到串口
 */
static void SENSOR_Color_Shell(void) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    uint8_t byte;
    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    TCS230_RGBC_t rgbc;
    SENSOR_ReadAll(&rgbc);

    if (!isWB) {
        logPrintln("White Balance Not Set");
        return;
    }

    /*
    *Rn = (fR / fC) × (fC0 / fR0)
    *Gn = (fG / fC) × (fC0 / fG0)
    *Bn = (fB / fC) × (fC0 / fB0)
    */

    for (;;) {
        SENSOR_ReadAll(&rgbc);

        float Rn =   (float)rgbc.red / (float)rgbc.clear * (float)rgbc_wb.clear / (float)rgbc_wb.red;
        float Gn =   (float)rgbc.green / (float)rgbc.clear * (float)rgbc_wb.clear / (float)rgbc_wb.green;
        float Bn =   (float)rgbc.blue / (float)rgbc.clear * (float)rgbc_wb.clear / (float)rgbc_wb.blue;

        float maxVal = fmaxf(fmaxf(Rn, Gn), Bn);

        if (maxVal > 0) {
            rgb_color.red = (uint8_t)((Rn / maxVal) * 255.0f);
            rgb_color.green = (uint8_t)((Gn / maxVal) * 255.0f);
            rgb_color.blue = (uint8_t)((Bn / maxVal) * 255.0f);
            // 打印 R,G,B 或使用这些数值
        }

        osDelay(200);
        logPrintln("\033[2K\rR: %f, G: %f, B: %f",
                 rgb_color.red, rgb_color.green, rgb_color.blue);

        if (osMessageQueueGet(Usart1_Rx_DataHandle, &byte, NULL, 0) == osOK) {
            if (byte == 0x03) break;
        }

    }
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("\033[1A\033[2K\r");
}

static void SENSOR_WB_Shell(void) {
    TCS230_RGBC_t rgbc;
    SENSOR_ReadAll(&rgbc);

    rgbc_wb.red = rgbc.red;
    rgbc_wb.green = rgbc.green;
    rgbc_wb.blue = rgbc.blue;
    rgbc_wb.clear = rgbc.clear;

    logPrintln("W: %f", rgbc_wb.clear / rgbc_wb.red); 
    logPrintln("W: %f", rgbc_wb.clear / rgbc_wb.green);
    logPrintln("W: %f", rgbc_wb.clear / rgbc_wb.blue);
    
    isWB = true;
    logPrintln("White Balance Set");
}

ShellCommand SensorGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, read, SENSOR_Read_Shell, read TCS230 RGBC frequency continuously),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, color, SENSOR_Color_Shell, read TCS230 RGBC frequency and apply white balance),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, wb, SENSOR_WB_Shell, set white balance),
    SHELL_CMD_GROUP_END()
};

SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       sensor, SensorGroup, TCS230 Color Sensor Tool Group);
