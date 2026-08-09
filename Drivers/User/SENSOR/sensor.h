#ifndef __SENSOR_H__
#define __SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "sensor_cfg.h"

/**
 * TCS230 颜色滤波器选择
 * Bit1=S2, Bit0=S3
 */
typedef enum {
    TCS230_FILTER_RED   = 0,  // S2=L, S3=L
    TCS230_FILTER_BLUE  = 1,  // S2=L, S3=H
    TCS230_FILTER_CLEAR = 2,  // S2=H, S3=L
    TCS230_FILTER_GREEN = 3,  // S2=H, S3=H
    TCS230_FILTER_NUM
} TCS230_Filter_t;

/**
 * RGBC 读取结构体
 */
typedef struct {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t clear;
} TCS230_RGBC_t;

/**
 * @brief 颜色识别结果
 */
typedef struct {
    const char *color_name;    /**< 颜色名称 */
    uint8_t confidence;        /**< 可信度 0-100 */
} SENSOR_ColorResult_t;

/**
 * @brief 白色平衡参数
 */
extern TCS230_RGBC_t rgbc_wb;

/**
 * @brief 异步读取完成回调
 * @param result  指向读取结果的指针（仅回调期间有效）
 * @param success true=读取成功, false=超时/失败
 */
typedef void (*SENSOR_ReadAll_Callback_t)(const TCS230_RGBC_t *result, bool success);

/**
 * @brief 初始化 TCS230 传感器接口
 * - 设置默认滤波器为 Clear (S2=H, S3=L)
 */
void SENSOR_Init(void);

/**
 * @brief 将原始 RGBC 频率转换为色度 RGB (0-255) + 亮度因子
 */
bool sensor_rgbc_to_rgb(const TCS230_RGBC_t *raw, const TCS230_RGBC_t *wb,
                                uint8_t *r, uint8_t *g, uint8_t *b,
                                float *brightness);

/**
 * @brief 选择颜色滤波器
 * @param filter 颜色滤波器选择
 */
void SENSOR_SetFilter(TCS230_Filter_t filter);

/**
 * @brief 启动异步 RGBC 读取
 * @param callback 完成回调（在 SENSOR_Process 上下文中调用，可为 NULL）
 *
 * 调用后立即返回。必须周期性地调用 SENSOR_Process() 驱动状态机。
 * 当所有 4 通道读取完成时，若 callback 非 NULL 则调用之。
 */
void SENSOR_StartReadAll(SENSOR_ReadAll_Callback_t callback);

/**
 * @brief 轮询检查异步读取是否完成
 * @return true 表示读取完成，结果可通过 SENSOR_GetResult() 获取
 */
bool SENSOR_ReadAll_IsComplete(void);

/**
 * @brief 获取最近一次异步读取的结果
 * @return 指向 RGBC 结果结构体的指针
 *
 * 可在 SENSOR_ReadAll_IsComplete() 返回 true 后或回调中调用。
 */
const TCS230_RGBC_t* SENSOR_GetResult(void);

/**
 * @brief 取消正在进行的异步测量
 */
void SENSOR_Cancel(void);

/**
 * @brief 检查是否有测量正在进行
 * @return true 忙碌中
 */
bool SENSOR_IsBusy(void);

/**
 * @brief 驱动非阻塞状态机（必须周期性调用）
 *
 * 建议调用频率至少 1ms 一次，以保证超时检测精度。
 * 可在定时器中断、RTOS 任务或主循环中调用。
 */
void SENSOR_Process(void);

/**
 * @brief 根据色度 RGB + 亮度因子推断当前颜色
 * @param r 红色色度 (0-255)
 * @param g 绿色色度 (0-255)
 * @param b 蓝色色度 (0-255)
 * @param brightness  亮度因子 0.0~1.0 (fC/fC_wb)
 * @return 颜色识别结果
 *
 * 可识别颜色: Black, White, Red, Green, Blue
 */
SENSOR_ColorResult_t SENSOR_InferColor(uint8_t r, uint8_t g, uint8_t b, float brightness);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_H__ */
