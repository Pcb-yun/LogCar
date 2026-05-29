#ifndef __BATTERY_H__
#define __BATTERY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 初始化电池管理模块（会自动创建 FreeRTOS 任务） */
void Battery_Init(void);

/* 获取当前电池电压（单位：伏特） */
float Battery_GetVoltage(void);

/* 动态修改采样间隔（毫秒） */
void Battery_SetInterval(uint16_t ms);

/* 获取当前采样间隔（毫秒） */
uint16_t Battery_GetInterval(void);

#ifdef __cplusplus
}
#endif

#endif