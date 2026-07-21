/**
 * @file sensor.c
 * @brief TCS230 颜色传感器驱动
 *
 * 引脚连接:
 *   SENSOR_OUT (PD12) — TCS230 OUT, TIM4_CH1 (输入捕获)
 *   SENSOR_S2  (PE2)  — TCS230 S2 (滤波器选择)
 *   SENSOR_S3  (PE3)  — TCS230 S3 (滤波器选择)
 *   S0, S1 外部硬件固定为 H (100% 频率标定)
 *
 * 使用 TIM4 PWM 输入模式 (6 MHz) 硬件捕获输出信号周期，
 * TIM2 (1MHz, 32-bit) 仅用于超时计时。
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
 * TIM4 计数器频率
 * SENSOR_Init 中将 TIM4 预分频器从 83(1MHz) 改为 13(6MHz)，
 * 提高高频测量分辨率。原始值保留在 tim.c 中避免 CubeMX 冲突。
 */
#define TCS230_TIM_CLOCK_HZ  6000000UL

/** 频率测量采样数 (取奇数，便于去极值后取中段均值) */
#define FREQ_NSAMPLES  5

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
 * @brief 使用 TIM4 输入捕获（PWM 输入模式）测量 SENSOR_OUT 引脚的频率
 * @return 频率，单位为 Hz，0 表示超时
 *
 * TIM4 已配置为 PWM 输入模式 (6 MHz, 16-bit):
 *   - 每个上升沿计数器复位，CCR1 记录周期（μs 当量）
 *   - CCR2 记录高电平脉宽（μs 当量）（目前未使用）
 *
 * 测量策略：
 *   1. 丢弃第一个捕获（滤波器切换后信号可能不稳定）
 *   2. 采集 5 个周期样本
 *   3. 去掉最大值和最小值，取中间 3 个的平均值
 *   4. 用平均周期计算频率
 */
static uint32_t measure_frequency(void) {
    uint32_t start = TIM2->CNT;

    /* 丢弃第一个捕获 — 滤波器切换后信号可能尚未稳定 */
    TIM4->SR = (uint32_t)~TIM_SR_CC1IF;
    while ((TIM4->SR & TIM_SR_CC1IF) == 0) {
        if ((TIM2->CNT - start) > TCS230_TIMEOUT_US) return 0;
    }

    /* 采集 FREQ_NSAMPLES 个周期的样本 */
    uint32_t samples[FREQ_NSAMPLES];
    for (int i = 0; i < FREQ_NSAMPLES; i++) {
        TIM4->SR = (uint32_t)~TIM_SR_CC1IF;
        while ((TIM4->SR & TIM_SR_CC1IF) == 0) {
            if ((TIM2->CNT - start) > TCS230_TIMEOUT_US) return 0;
        }
        samples[i] = TIM4->CCR1;
    }

    /* 冒泡排序，方便去极值 */
    for (int i = 0; i < FREQ_NSAMPLES - 1; i++) {
        for (int j = i + 1; j < FREQ_NSAMPLES; j++) {
            if (samples[i] > samples[j]) {
                uint32_t t = samples[i];
                samples[i] = samples[j];
                samples[j] = t;
            }
        }
    }

    /* 去掉最小和最大的各 1 个，取中间值平均 */
    uint32_t sum = 0;
    for (int i = 1; i < FREQ_NSAMPLES - 1; i++) {
        sum += samples[i];
    }
    uint32_t period = sum / (FREQ_NSAMPLES - 2);

    if (period == 0) return 0;

    return TCS230_TIM_CLOCK_HZ / period;
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
    /* 确保 TIM4 计数器时钟为 6MHz（预分频器 84MHz/14=6MHz） */
    TIM4->PSC = 13;
    TIM4->EGR = TIM_EGR_UG;

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
 * @brief 将原始 RGBC 频率转换为色度 RGB (0-255) + 亮度因子
 * @param raw       原始频率读数
 * @param wb        白平衡参考
 * @param r         输出红色色度 (0-255)
 * @param g         输出绿色色度 (0-255)
 * @param b         输出蓝色色度 (0-255)
 * @param brightness 输出亮度因子 0.0~1.0 (fC / fC_wb)
 * @return true 转换成功, false 数据无效
 *
 * 色度 = 通道占比 / 白平衡校正, 归一化到峰值 = 1.0 (纯色度, 与亮度无关)
 * 亮度 = fC / fC_wb, 用于 InferColor 区分黑/白/灰/彩色
 */
static bool sensor_rgbc_to_rgb(const TCS230_RGBC_t *raw, const TCS230_RGBC_t *wb,
                                uint8_t *r, uint8_t *g, uint8_t *b,
                                float *brightness) {
    if (raw->clear == 0 || wb->clear == 0 ||
        wb->red == 0 || wb->green == 0 || wb->blue == 0) {
        *r = *g = *b = 0;
        if (brightness) *brightness = 0.0f;
        return false;
    }

    float Rn = (float)raw->red   / (float)raw->clear * (float)wb->clear / (float)wb->red;
    float Gn = (float)raw->green / (float)raw->clear * (float)wb->clear / (float)wb->green;
    float Bn = (float)raw->blue  / (float)raw->clear * (float)wb->clear / (float)wb->blue;

    /* 亮度因子：用于 InferColor 区分黑/白 */
    float bri = (float)raw->clear / (float)wb->clear;
    if (bri > 1.0f) bri = 1.0f;
    if (brightness) *brightness = bri;

    /* 色度归一化：峰值通道 = 1.0，与亮度无关 */
    float maxRGB = fmaxf(Rn, fmaxf(Gn, Bn));
    if (maxRGB > 0.0f) {
        float inv = 1.0f / maxRGB;
        *r = (uint8_t)(fminf(Rn * inv * 255.0f, 255.0f));
        *g = (uint8_t)(fminf(Gn * inv * 255.0f, 255.0f));
        *b = (uint8_t)(fminf(Bn * inv * 255.0f, 255.0f));
    } else {
        *r = *g = *b = 0;
    }
    return true;
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

    TCS230_RGBC_t rgbc;
    SENSOR_ReadAll(&rgbc);

    if (!isWB) {
        logPrintln("White Balance Not Set");
        return;
    }

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    /*
    *Rn = (fR / fC) × (fC0 / fR0)
    *Gn = (fG / fC) × (fC0 / fG0)
    *Bn = (fB / fC) × (fC0 / fB0)
    */

    for (;;) {
        SENSOR_ReadAll(&rgbc);

        uint8_t cr, cg, cb;
        float bri;
        if (!sensor_rgbc_to_rgb(&rgbc, &rgbc_wb, &cr, &cg, &cb, &bri)) {
            osDelay(200);
            continue;
        }

        rgb_color.red = cr;
        rgb_color.green = cg;
        rgb_color.blue = cb;

        SENSOR_ColorResult_t inferred = SENSOR_InferColor(cr, cg, cb, bri);

        osDelay(200);
        logPrintln("\r\033[1A\033[2K\rR: %3u  G: %3u  B: %3u  |  %s (%u%%)",
                   cr, cg, cb,
                   inferred.color_name, inferred.confidence);

        if (osMessageQueueGet(Usart1_Rx_DataHandle, &byte, NULL, 0) == osOK) {
            if (byte == 0x03) break;
        }

    }
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("\033[1A\033[2K\r");
}

static void SENSOR_WB_Shell(void) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    TCS230_RGBC_t rgbc;

    SENSOR_ReadAll(&rgbc);

    // if(rgbc.clear > 600000 ||
    //      rgbc.red > 60000 ||
    //      rgbc.green > 60000 ||
    //      rgbc.blue > 60000) {
    //     logPrintln("Exposure Too High  R: %u  G: %u  B: %u  C: %u",
    //                rgbc.red, rgbc.green, rgbc.blue, rgbc.clear);
    //     return;
    // }else if(rgbc.clear < 1000 ||
    //      rgbc.red < 1000 ||
    //      rgbc.green < 1000 ||
    //      rgbc.blue < 1000) {
    //     logPrintln("Exposure Too Low  R: %u  G: %u  B: %u  C: %u",
    //                rgbc.red, rgbc.green, rgbc.blue, rgbc.clear);
    //     return;
    // }

    rgbc_wb.red = rgbc.red;
    rgbc_wb.green = rgbc.green;
    rgbc_wb.blue = rgbc.blue;
    rgbc_wb.clear = rgbc.clear;

    logPrintln("R: %u, G: %u, B: %u", rgbc_wb.red, rgbc_wb.green, rgbc_wb.blue);
    logPrintln("Exposure: %u", rgbc_wb.clear);
    logPrintln("W: %f", (float)rgbc_wb.clear / rgbc_wb.red);
    logPrintln("W: %f", (float)rgbc_wb.clear / rgbc_wb.green);
    logPrintln("W: %f", (float)rgbc_wb.clear / rgbc_wb.blue);

    isWB = true;
    logPrintln("White Balance Set");

}

/**
 * @brief 根据色度 RGB + 亮度因子推断颜色
 * @param r 红色色度 (0-255)
 * @param g 绿色色度 (0-255)
 * @param b 蓝色色度 (0-255)
 * @param brightness  亮度因子 0.0~1.0 (fC/fC_wb)
 * @return 颜色识别结果
 *
 * 算法:
 *   1. brightness < 0.15 → Black (极暗)
 *   2. brightness > 0.4 && range < 0.3 → White (高亮 + 通道高度均匀)
 *   3. 否则按通道占比偏离均匀基准 (1/3) 判定 Red/Green/Blue
 *   4. 亮度过低时对彩色评分施加惩罚
 *   5. 最佳评分 < 5% 时视为 Unknown（防止噪声误判）
 */
SENSOR_ColorResult_t SENSOR_InferColor(uint8_t r, uint8_t g, uint8_t b,
                                        float brightness) {
    SENSOR_ColorResult_t result = {"Unknown", 0};

    float fr = r / 255.0f;
    float fg = g / 255.0f;
    float fb = b / 255.0f;

    float maxC = fmaxf(fr, fmaxf(fg, fb));
    float minC = fminf(fr, fminf(fg, fb));
    float range = maxC - minC;
    float sum  = fr + fg + fb;

    const char *names[] = {"Black", "White", "Red", "Green", "Blue"};
    float scores[5] = {0};

    /* --- Black: 亮度极低 --- */
    if (brightness < 0.15f) {
        scores[0] = 1.0f - brightness / 0.15f;
    }

    /* --- White: 较高亮度 + 各通道高度均匀 --- */
    if (brightness > 0.4f && range < 0.3f) {
        float uniformity = 1.0f - range / 0.3f;          /* range=0 → 1.0, range=0.3 → 0.0 */
        float bri_factor = fminf(brightness / 0.5f, 1.0f);
        scores[1] = bri_factor * uniformity;
    }

    /* --- 彩色: 有效光照下按主导色判断 --- */
    if (brightness > 0.08f && sum > 0.05f) {
        float r_ratio = fr / sum;
        float g_ratio = fg / sum;
        float b_ratio = fb / sum;

        /* 相对均匀基准 (1/3) 的偏离 × 2.0 */
        scores[2] = fmaxf(0.0f, (r_ratio - 1.0f / 3.0f) * 2.0f);
        scores[3] = fmaxf(0.0f, (g_ratio - 1.0f / 3.0f) * 2.0f);
        scores[4] = fmaxf(0.0f, (b_ratio - 1.0f / 3.0f) * 2.0f);

        /* 亮度惩罚: brightness < 0.25 时线性衰减 */
        float bp = fminf(brightness / 0.25f, 1.0f);
        for (int i = 2; i < 5; i++) scores[i] *= bp;
    }

    /* 选择最高分 */
    int best = 0;
    for (int i = 1; i < 5; i++) {
        if (scores[i] > scores[best]) best = i;
    }

    /* 评分过低 → Black */
    if (scores[best] < 0.05f) {
        result.color_name = "Black";
        result.confidence = (uint8_t)(scores[best] * 100.0f);
        return result;
    }

    result.color_name = names[best];
    result.confidence = (uint8_t)fminf(scores[best] * 100.0f, 100.0f);
    return result;
}

static void SENSOR_Detect_Shell(void) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    uint8_t byte;
    TCS230_RGBC_t rgbc;

    if (!isWB) {
        logPrintln("White Balance Not Set");
        return;
    }

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);
    logPrintln("Color Detection - Press ^C to exit");

    for (;;) {
        osDelay(200);
        SENSOR_ReadAll(&rgbc);

        uint8_t cr, cg, cb;
        float bri;
        if (!sensor_rgbc_to_rgb(&rgbc, &rgbc_wb, &cr, &cg, &cb, &bri)) {
            continue;
        }

        SENSOR_ColorResult_t inferred = SENSOR_InferColor(cr, cg, cb, bri);

        logPrintln("\033[1A\033[2K\rR: %3u  G: %3u  B: %3u  =>  %s (%u%%)",
                   cr, cg, cb, inferred.color_name, inferred.confidence);

        if (osMessageQueueGet(Usart1_Rx_DataHandle, &byte, NULL, 0) == osOK) {
            if (byte == 0x03) break;
        }
    }

    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
    logPrintln("");
}

ShellCommand SensorGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, read, SENSOR_Read_Shell, read TCS230 RGBC frequency continuously),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, color, SENSOR_Color_Shell, read TCS230 RGBC frequency and apply white balance),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, wb, SENSOR_WB_Shell, set white balance),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, detect, SENSOR_Detect_Shell, detect color and show confidence),
    SHELL_CMD_GROUP_END()
};

SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       sensor, SensorGroup, TCS230 Color Sensor Tool Group);
