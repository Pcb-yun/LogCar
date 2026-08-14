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
 * TIM2 (1MHz, 32-bit) 用于非阻塞超时计时。
 *
 * === 非阻塞状态机 ===
 * 通过 SENSOR_StartReadAll() 启动测量，SENSOR_Process() 驱动状态机，
 * 每个通道经历: SET_FILTER → SETTLE → DISCARD → SAMPLE(5) → COMPUTE
 * 完成后通过回调或轮询通知调用者。
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

/**
 * @brief 白色平衡参数
 */
TCS230_RGBC_t rgbc_wb = {0};

/**
 * @brief 颜色参数
 */
TCS230_RGBC_t rgb_color = {0};

/**
 * @brief 单通道测量阶段
 */
typedef enum {
    PHASE_IDLE = 0,
    PHASE_SET_FILTER,       // 切换滤波器 GPIO
    PHASE_SETTLE,           // 等待信号稳定 (TCS230_SETTLE_US)
    PHASE_DISCARD,          // 丢弃第一个捕获
    PHASE_SAMPLE,           // 采集 FREQ_NSAMPLES 个样本
    PHASE_COMPUTE,          // 计算频率
} SENSOR_Phase_t;

/**
 * @brief 单通道测量上下文
 */
typedef struct {
    TCS230_Filter_t channels[4];    // 4 个通道按顺序
    int channel_idx;                // 当前通道 (0~3)
    SENSOR_Phase_t phase;           // 当前阶段
    int sample_idx;                 // 当前样本索引
    uint32_t samples[FREQ_NSAMPLES];
    uint32_t phase_start;           // 当前阶段开始时的 TIM2->CNT
    volatile bool capture_ready;
    TCS230_RGBC_t result;
    bool busy;
    bool complete;
    SENSOR_ReadAll_Callback_t callback;
} SENSOR_Context_t;

/**
 * @brief 单通道测量上下文
 */
static SENSOR_Context_t s_ctx;

/**
 * @brief 初始化 TCS230 传感器
 * - 设置默认白色平衡参数
 */
void TCS230_Init(void) {
    rgbc_wb.red = TCS230_WB_R;
    rgbc_wb.green = TCS230_WB_G;
    rgbc_wb.blue = TCS230_WB_B;
    rgbc_wb.clear = TCS230_WB_C;
}

/**
 * @brief 初始化 TCS230 传感器接口
 * - 设置默认滤波器为 Clear (S2=H, S3=L)
 */
void SENSOR_Init(void) {
    SENSOR_SetFilter(TCS230_FILTER_CLEAR);
}

/**
 * @brief 选择颜色滤波器
 * @param filter 颜色滤波器选择
 */
void SENSOR_SetFilter(TCS230_Filter_t filter) {
    HAL_GPIO_WritePin(SENSOR_S2_GPIO_Port, SENSOR_S2_Pin,
                      (filter & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SENSOR_S3_GPIO_Port, SENSOR_S3_Pin,
                      (filter & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief TIM IC 捕获回调（由 TIM4_IRQHandler → HAL_TIM_IRQHandler 调用）
 *
 * 覆盖 HAL 弱定义，在 ISR 上下文中运行，仅设置标志并读取 CCR1。
 * CCR1 在 PWM 输入模式下记录周期计数值。
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4 && s_ctx.busy) {
        s_ctx.capture_ready = true;
    }
}

/**
 * @brief 计算样本缓冲区中有效周期的均值频率
 * @param samples  原始周期样本
 * @return 频率 Hz，0 表示无效
 */
static uint32_t compute_frequency(const uint32_t *samples) {
    /* 排序 */
    uint32_t buf[FREQ_NSAMPLES];
    memcpy(buf, samples, sizeof(buf));
    for (int i = 0; i < FREQ_NSAMPLES - 1; i++) {
        for (int j = i + 1; j < FREQ_NSAMPLES; j++) {
            if (buf[i] > buf[j]) {
                uint32_t t = buf[i];
                buf[i] = buf[j];
                buf[j] = t;
            }
        }
    }
    /* 去掉最小和最大的各 1 个，取中间 3 个平均 */
    uint32_t sum = 0;
    for (int i = 1; i < FREQ_NSAMPLES - 1; i++) {
        sum += buf[i];
    }
    uint32_t period = sum / (FREQ_NSAMPLES - 2);
    if (period == 0) return 0;
    return TCS230_TIM_CLOCK_HZ / period;
}

/**
 * @brief 获取当前通道对应的 result 字段指针
 */
static uint32_t* channel_result_ptr(int channel_idx) {
    switch (s_ctx.channels[channel_idx]) {
        case TCS230_FILTER_CLEAR: return &s_ctx.result.clear;
        case TCS230_FILTER_RED:   return &s_ctx.result.red;
        case TCS230_FILTER_GREEN: return &s_ctx.result.green;
        case TCS230_FILTER_BLUE:  return &s_ctx.result.blue;
        default:                  return NULL;
    }
}

/**
 * @brief 进入下一个通道
 * @return true 还有更多通道，false 所有通道已完成
 */
static bool start_next_channel(void) {
    s_ctx.channel_idx++;
    if (s_ctx.channel_idx >= 4) {
        return false;  // 所有通道完成
    }
    s_ctx.phase = PHASE_SET_FILTER;
    s_ctx.sample_idx = 0;
    return true;
}

/**
 * @brief 启动指定阶段的计时
 */
static inline void start_phase_timer(void) {
    s_ctx.phase_start = TIM2->CNT;
}

/**
 * @brief 终止当前测量（超时或取消）
 */
static void abort_measurement(void) {
    s_ctx.busy = false;
    s_ctx.complete = false;
    s_ctx.phase = PHASE_IDLE;
    TIM4->DIER &= ~TIM_DIER_CC1IE;  // 关闭 CC1 中断
    if (s_ctx.callback) {
        s_ctx.callback(&s_ctx.result, false);
    }
}

void SENSOR_StartReadAll(SENSOR_ReadAll_Callback_t callback) {
    if (s_ctx.busy) return;

    /* 初始化上下文 */
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.channels[0] = TCS230_FILTER_CLEAR;
    s_ctx.channels[1] = TCS230_FILTER_RED;
    s_ctx.channels[2] = TCS230_FILTER_GREEN;
    s_ctx.channels[3] = TCS230_FILTER_BLUE;
    s_ctx.channel_idx = 0;
    s_ctx.phase = PHASE_SET_FILTER;
    s_ctx.busy = true;
    s_ctx.complete = false;
    s_ctx.callback = callback;

    /* 启用 TIM4 CC1 中断（PWM 输入模式下捕获完成时触发） */
    TIM4->DIER |= TIM_DIER_CC1IE;
}

/**
 * @brief 检查所有通道是否已完成读取
 * @return true 所有通道已完成读取，false 有通道未完成
 */
bool SENSOR_ReadAll_IsComplete(void) {
    return s_ctx.complete;
}

/**
 * @brief 获取所有通道的读取结果
 * @return 指向读取结果的指针（仅在 SENSOR_ReadAll_IsComplete 为 true 时有效）
 */
const TCS230_RGBC_t* SENSOR_GetResult(void) {
    return &s_ctx.result;
}

/**
 * @brief 取消当前测量（超时或取消）
 */
void SENSOR_Cancel(void) {
    if (!s_ctx.busy) return;
    abort_measurement();
}

/**
 * @brief 检查传感器是否正在忙于读取
 * @return true 正在读取，false 未读取
 */
bool SENSOR_IsBusy(void) {
    return s_ctx.busy;
}

/**
 * @brief 驱动非阻塞状态机（必须周期性调用）
 *
 * 每次调用最多执行一个阶段的非阻塞转移。
 * 建议 1ms 周期调用。
 */
void SENSOR_Process(void) {
    if (!s_ctx.busy) return;

    uint32_t now = TIM2->CNT;

    switch (s_ctx.phase) {

    /* ========== 1. 设置滤波器 ========== */
    case PHASE_SET_FILTER: {
        TCS230_Filter_t filter = s_ctx.channels[s_ctx.channel_idx];
        SENSOR_SetFilter(filter);
        /* 确保 TIM4 PSC = 13 (6MHz) */
        TIM4->PSC = 13;
        TIM4->EGR = TIM_EGR_UG;
        s_ctx.phase = PHASE_SETTLE;
        start_phase_timer();
        break;
    }

    /* ========== 2. 等待稳定 ========== */
    case PHASE_SETTLE: {
        if ((now - s_ctx.phase_start) >= TCS230_SETTLE_US) {
            /* 清除捕获标志，准备丢弃第一个捕获 */
            TIM4->SR = (uint32_t)~TIM_SR_CC1IF;
            s_ctx.capture_ready = false;
            s_ctx.phase = PHASE_DISCARD;
            start_phase_timer();
        }
        break;
    }

    /* ========== 3. 丢弃第一个捕获 ========== */
    case PHASE_DISCARD: {
        if ((now - s_ctx.phase_start) > TCS230_TIMEOUT_US) {
            /* 整个通道超时 */
            *channel_result_ptr(s_ctx.channel_idx) = 0;
            if (!start_next_channel()) {
                goto done;
            }
            break;
        }
        if (s_ctx.capture_ready) {
            s_ctx.capture_ready = false;
            /* 丢弃第一个捕获值，清除标志开始采样 */
            TIM4->SR = (uint32_t)~TIM_SR_CC1IF;
            s_ctx.sample_idx = 0;
            s_ctx.phase = PHASE_SAMPLE;
            start_phase_timer();
        }
        break;
    }

    /* ========== 4. 采集样本 ========== */
    case PHASE_SAMPLE: {
        if ((now - s_ctx.phase_start) > TCS230_TIMEOUT_US) {
            *channel_result_ptr(s_ctx.channel_idx) = 0;
            if (!start_next_channel()) {
                goto done;
            }
            break;
        }
        if (s_ctx.capture_ready) {
            s_ctx.capture_ready = false;
            s_ctx.samples[s_ctx.sample_idx++] = TIM4->CCR1;
            /* 在最后一次采集中已经清除标志，不需要再次清除 */
            TIM4->SR = (uint32_t)~TIM_SR_CC1IF;

            if (s_ctx.sample_idx >= FREQ_NSAMPLES) {
                s_ctx.phase = PHASE_COMPUTE;
            }
        }
        break;
    }

    /* ========== 5. 计算频率 ========== */
    case PHASE_COMPUTE: {
        uint32_t *result = channel_result_ptr(s_ctx.channel_idx);
        *result = compute_frequency(s_ctx.samples);
        /* 进入下一通道 */
        if (!start_next_channel()) {
            goto done;
        }
        break;
    }

    default:
        break;
    }
    return;

done:
    /* 所有 4 个通道读取完成 */
    s_ctx.busy = false;
    s_ctx.complete = true;
    TIM4->DIER &= ~TIM_DIER_CC1IE;  // 关闭 CC1 中断
    if (s_ctx.callback) {
        s_ctx.callback(&s_ctx.result, true);
    }
}

/**
 * @brief 将原始 RGBC 频率转换为色度 RGB (0-255) + 亮度因子
 */
bool sensor_rgbc_to_rgb(const TCS230_RGBC_t *raw, const TCS230_RGBC_t *wb,
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

    float bri = (float)raw->clear / (float)wb->clear;
    if (bri > 1.0f) bri = 1.0f;
    if (brightness) *brightness = bri;

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
 * @brief 读取所有四个 RGBC 通道的频率（阻塞）
 * @param out RGBC 读取结构体指针
 *
 * 依次读取 Clear → Red → Green → Blue，每次阻塞。
 */
static void SENSOR_Read_Shell(void) {
    TCS230_RGBC_t rgbc;

    Shell *shell;
    uint8_t byte;
    shell = shellGetCurrent();
    if (shell == NULL) return;

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    SENSOR_Init();
    logPrintln("TCS230 Reader - Press ^C to exit\r\n"
               "  R       G       B       C");

    for (;;) {
        /* 非阻塞读取 */
        SENSOR_StartReadAll(NULL);
        while (!SENSOR_ReadAll_IsComplete()) {
            SENSOR_Process();
            osDelay(1);
        }
        const TCS230_RGBC_t *p = SENSOR_GetResult();
        rgbc = *p;  // copy

        logPrintln("\033[1A\033[2K\r%5lu  %5lu  %5lu  %5lu Hz",
                   rgbc.red, rgbc.green, rgbc.blue, rgbc.clear);

        if (shell->read((char*)&byte, 1)) {
            if (byte == 0x03) break;
        }
        osDelay(200);
    }
    logPrintln("\033[1A\033[2K\r");
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
}

/**
 * @brief 读取所有四个 RGBC 通道的频率（阻塞）
 * @param out RGBC 读取结构体指针
 *
 * 依次读取 Clear → Red → Green → Blue，每次阻塞。
 */
static void SENSOR_Color_Shell(void) {
    Shell *shell;
    uint8_t byte;
    shell = shellGetCurrent();
    if (shell == NULL) return;

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    for (;;) {
        /* 非阻塞读取 */
        SENSOR_StartReadAll(NULL);
        while (!SENSOR_ReadAll_IsComplete()) {
            SENSOR_Process();
            osDelay(1);
        }
        const TCS230_RGBC_t *rgbc = SENSOR_GetResult();

        uint8_t cr, cg, cb;
        float bri;
        if (!sensor_rgbc_to_rgb(rgbc, &rgbc_wb, &cr, &cg, &cb, &bri)) {
            osDelay(200);
            continue;
        }

        rgb_color.red = cr;
        rgb_color.green = cg;
        rgb_color.blue = cb;

        SENSOR_ColorResult_t inferred = SENSOR_InferColor(cr, cg, cb, bri);

        logPrintln("\r\033[1A\033[2K\rR: %3u  G: %3u  B: %3u  |  %s (%u%%)",
                   cr, cg, cb,
                   inferred.color_name, inferred.confidence);

        if (shell->read((char*)&byte, 1)) {
            if (byte == 0x03) break;
        }
        osDelay(200);
    }
    logPrintln("\033[1A\033[2K\r");
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
}

/**
 * @brief 读取所有四个 RGBC 通道的频率（阻塞）
 * @param out RGBC 读取结构体指针
 *
 * 依次读取 Clear → Red → Green → Blue，每次阻塞。
 */
static void SENSOR_WB_Shell(void) {
    TCS230_RGBC_t rgbc;

    /* 非阻塞读取 */
    SENSOR_StartReadAll(NULL);
    while (!SENSOR_ReadAll_IsComplete()) {
        SENSOR_Process();
        osDelay(1);
    }
    const TCS230_RGBC_t *p = SENSOR_GetResult();
    rgbc = *p;

    rgbc_wb.red = rgbc.red;
    rgbc_wb.green = rgbc.green;
    rgbc_wb.blue = rgbc.blue;
    rgbc_wb.clear = rgbc.clear;

    logPrintln("R: %u, G: %u, B: %u", rgbc_wb.red, rgbc_wb.green, rgbc_wb.blue);
    logPrintln("Exposure: %u", rgbc_wb.clear);
    logPrintln("Wr: %f", (float)rgbc_wb.clear / rgbc_wb.red);
    logPrintln("Wg: %f", (float)rgbc_wb.clear / rgbc_wb.green);
    logPrintln("Wb: %f", (float)rgbc_wb.clear / rgbc_wb.blue);

    logPrintln("White Balance Set");
}

/**
 * @brief 读取所有四个 RGBC 通道的频率（阻塞）
 * @param out RGBC 读取结构体指针
 *
 * 依次读取 Clear → Red → Green → Blue，每次阻塞。
 */
static void SENSOR_Detect_Shell(void) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    uint8_t byte;
    TCS230_RGBC_t rgbc;

    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);
    logPrintln("Color Detection - Press ^C to exit");

    for (;;) {
        osDelay(200);

        /* 非阻塞读取 */
        SENSOR_StartReadAll(NULL);
        while (!SENSOR_ReadAll_IsComplete()) {
            SENSOR_Process();
            osDelay(1);
        }
        const TCS230_RGBC_t *p = SENSOR_GetResult();
        rgbc = *p;

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

/**
 * @brief 颜色推理
 * @param r 红色频率
 * @param g 绿色频率
 * @param b 蓝色频率
 * @param brightness 亮度因子
 * @return 颜色识别结果
 */
SENSOR_ColorResult_t SENSOR_InferColor(uint8_t r, uint8_t g, uint8_t b,
                                        float brightness) {
    SENSOR_ColorResult_t result = {"Unknown", COLOR_UNKNOWN, 0};

    float fr = r / 255.0f;
    float fg = g / 255.0f;
    float fb = b / 255.0f;

    float maxC = fmaxf(fr, fmaxf(fg, fb));
    float minC = fminf(fr, fminf(fg, fb));
    float range = maxC - minC;
    float saturation = (maxC > 0.001f) ? (range / maxC) : 0.0f;

    /* 黑色：亮度过低 */
    if (brightness < 0.15f) {
        result.color_name = "Black";
        result.color = COLOR_BLACK;
        result.confidence = (uint8_t)((1.0f - brightness / 0.15f) * 100.0f);
        return result;
    }

    /* 白色：亮度高且饱和度低 */
    if (brightness > 0.4f && saturation < 0.25f) {
        float conf = (1.0f - saturation / 0.25f) * fminf(brightness / 0.6f, 1.0f);
        result.color_name = "White";
        result.color = COLOR_WHITE;
        result.confidence = (uint8_t)(conf * 100.0f);
        return result;
    }

    /* 彩色：基于通道主导性判断 */
    if (saturation > 0.10f && brightness > 0.08f) {
        /* 找出次强分量 */
        float top2;
        if (maxC == fr)          top2 = fmaxf(fg, fb);
        else if (maxC == fg)     top2 = fmaxf(fr, fb);
        else                     top2 = fmaxf(fr, fg);

        /* 主导度 = (最强 - 次强) / 最强，衡量颜色的纯度 */
        float dominance = (maxC - top2) / maxC;
        float bf = fminf(brightness / 0.15f, 1.0f);
        float score = dominance * bf;

        if (score > 0.05f) {
            if (maxC == fr) {
                result.color_name = "Red";
                result.color = COLOR_RED;
            } else if (maxC == fg) {
                result.color_name = "Green";
                result.color = COLOR_GREEN;
            } else {
                result.color_name = "Blue";
                result.color = COLOR_BLUE;
            }
            result.confidence = (uint8_t)fminf(score * 100.0f, 100.0f);
            return result;
        }
    }

    /* 无法识别的颜色 */
    result.color_name = "Unknown";
    result.color = COLOR_BLACK; // 默认黑色
    result.confidence = 0;
    return result;
}

/**
 * @brief 颜色传感器颜色检测
 * @return 颜色枚举值
 */
SENSOR_Color_t SENSOR_DetectColor(void) {
	if (SENSOR_IsBusy()) {
		return COLOR_UNKNOWN;
	}

	SENSOR_StartReadAll(NULL);

	while (!SENSOR_ReadAll_IsComplete()) {
		SENSOR_Process();
		osDelay(1);
	}

	const TCS230_RGBC_t *raw = SENSOR_GetResult();
	uint8_t r, g, b;
	float bri;
	if (!sensor_rgbc_to_rgb(raw, &rgbc_wb, &r, &g, &b, &bri)) {
		return COLOR_UNKNOWN;
	}

	SENSOR_ColorResult_t res = SENSOR_InferColor(r, g, b, bri);
	return res.color;
}

/**
 * @brief Shell 命令注册
 */
ShellCommand SensorGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, read, SENSOR_Read_Shell, read TCS230 RGBC frequency continuously),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, color, SENSOR_Color_Shell, read TCS230 RGBC frequency and apply white balance),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, wb, SENSOR_WB_Shell, set white balance),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, detect, SENSOR_Detect_Shell, detect color and show confidence),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       sensor, SensorGroup, TCS230 Color Sensor Tool Group);
