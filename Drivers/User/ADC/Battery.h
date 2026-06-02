/**
 * @file Battery.h
 * @brief 电池电压监控模块头文件
 */

#ifndef __BATTERY_H__
#define __BATTERY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>

/**
 * @brief 电池电压监控模块的配置宏
 */
#define BATTERY_DIVIDER_RATIO   5.0f        // 分压比
#define ADC_VREF_MV             3300        // ADC 参考电压 (VREF+)，通常为 3300mV
#define ADC_RESOLUTION          4096.0f     // ADC 分辨率


bool Battery_Init(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BATTERY_H__ */
