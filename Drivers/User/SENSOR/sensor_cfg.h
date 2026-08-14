/**
 * @file sensor_cfg.h
 * @brief 颜色传感器配置头文件
 */

#ifndef __SENSOR_CFG_H__
#define __SENSOR_CFG_H__


#define TCS230_TIMEOUT_US  1000000UL    // 频率测量超时
#define TCS230_SETTLE_US   2000UL        // 滤波器切换后稳定等待时间
#define TCS230_TIM_CLOCK_HZ  6000000UL    // TIM4 计数器频率
#define FREQ_NSAMPLES  5            // 频率测量采样数

// 默认白色平衡
#define TCS230_WB_R 673
#define TCS230_WB_G 651
#define TCS230_WB_B 750
#define TCS230_WB_C 2011

#endif /* __SENSOR_CFG_H__ */
