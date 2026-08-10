/**
 * @file rpi_sensor_port.h
 * @brief 树莓派通信接口
 */

#ifndef __RPI_SENSOR_PORT_H__
#define __RPI_SENSOR_PORT_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "sensor.h"     /* SENSOR_ColorResult_t */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief 读取物料颜色识别结果
 * @return 颜色识别结果; 失败时 color_name 为 NULL, confidence 为 0
 */
SENSOR_ColorResult_t RPI_Read_Color(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RPI_PORT_H__ */
