#ifndef SENSOR_CFG_H
#define SENSOR_CFG_H

/**
 * @brief 频率测量超时 (TIM2 ticks, 1 tick = 1us)
 */
#define TCS230_TIMEOUT_US  1000000UL

/**
 * @brief 滤波器切换后稳定等待时间 (us)
 */
#define TCS230_SETTLE_US   2000UL

/**
 * @brief TIM4 计数器频率 (6MHz)
 * 预分频器 84MHz/14=6MHz，提高高频测量分辨率。
 */
#define TCS230_TIM_CLOCK_HZ  6000000UL

/**
 * @brief 频率测量采样数 (取奇数，便于去极值后取中段均值)
 */
#define FREQ_NSAMPLES  5

#endif 