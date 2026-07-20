#ifndef __SENSOR_H__
#define __SENSOR_H__

#include <stdbool.h>
#include <stdint.h>


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
 * @brief 初始化 TCS230 传感器接口
 * - 重新配置 SENSOR_OUT 引脚为输入无拉电阻
 * - 设置默认滤波器为 Clear (S2=H, S3=L)
 */
void SENSOR_Init(void);

/**
 * @brief 选择颜色滤波器
 * @param filter 颜色滤波器选择
 */
void SENSOR_SetFilter(TCS230_Filter_t filter);

/**
 * @brief 读取指定滤波器通道的频率
 * @param filter 读取前要选择的滤波器
 * @return 频率，单位为 Hz，0 表示超时
 *
 * 选择滤波器，等待 ~2ms 以稳定，然后测量频率。
 */
uint32_t SENSOR_ReadChannel(TCS230_Filter_t filter);

/**
 * @brief 读取所有四个 RGBC 通道的频率
 * @param out RGBC 读取结构体指针
 *
 * 读取通道顺序为：Clear → Red → Green → Blue
 * (优化为最小化 S2/S3 切换次数)
 */
void SENSOR_ReadAll(TCS230_RGBC_t *out);

#endif /* __SENSOR_H__ */
