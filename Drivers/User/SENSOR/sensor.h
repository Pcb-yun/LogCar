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
 * @brief 颜色枚举
 */
typedef enum {
	COLOR_UNKNOWN = 0,	// 未知
	COLOR_BLACK,		// 黑色
	COLOR_WHITE,		// 白色
	COLOR_RED,			// 红色
	COLOR_GREEN,		// 绿色
	COLOR_BLUE,			// 蓝色
} SENSOR_Color_t;

/**
 * @brief 颜色识别结果
 */
typedef struct {
	const char *color_name;		// 颜色名称
	SENSOR_Color_t color;		// 颜色枚举
	uint8_t confidence;			// 可信度 0-100
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
void SENSOR_Init(void);
bool sensor_rgbc_to_rgb(const TCS230_RGBC_t *raw, const TCS230_RGBC_t *wb,
                                uint8_t *r, uint8_t *g, uint8_t *b,
                                float *brightness);
void SENSOR_SetFilter(TCS230_Filter_t filter);
void SENSOR_StartReadAll(SENSOR_ReadAll_Callback_t callback);
bool SENSOR_ReadAll_IsComplete(void);
const TCS230_RGBC_t* SENSOR_GetResult(void);
void SENSOR_Cancel(void);
bool SENSOR_IsBusy(void);
void SENSOR_Process(void);
SENSOR_ColorResult_t SENSOR_InferColor(uint8_t r, uint8_t g, uint8_t b, float brightness);
SENSOR_Color_t SENSOR_DetectColor(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_H__ */
