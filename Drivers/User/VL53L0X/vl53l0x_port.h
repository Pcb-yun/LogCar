/**
 * @file vl53l0x_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief VL53L0X模块用户层头文件
 */

#ifndef __VL53L0X_PORT_H__
#define __VL53L0X_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "vl53l0x_api.h"
#include <stdbool.h>

bool VL53L0X_Init(void);
void Dist_Get_Task(void *argument);














#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VL53L0X_PORT_H__ */
