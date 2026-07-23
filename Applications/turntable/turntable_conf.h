#ifndef __TURNTABLE_CONF_H__
#define __TURNTABLE_CONF_H__

/**
 * @brief 转盘模块初始角度
 */
#define TURNABLE_INIT -10.0f

/**
 * @brief 转盘模块ID0角度
 */
#define TURNABLE_ID_0_ANGLE 26.0f

/**
 * @brief 转盘模块角度间隔
 */
#define TURNABLE_ANGLE 36.0f

/**
 * @brief 转盘模块关闭角度偏移量
 */
#define TURNABLE_CLOSE_OFFSET 18.0f

/**
 * @brief 转盘模块ID最大数量
 */
#define TURNABLE_ID_MAX 5

float turntable_id[TURNABLE_ID_MAX] = {TURNABLE_ID_0_ANGLE,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*2,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*4,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*6,
     TURNABLE_ID_0_ANGLE + TURNABLE_ANGLE*8};

#endif