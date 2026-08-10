/**
 * @file rpi_sensor_port.c
 * @brief 树莓派通信接口
 */

#include "rpi_sensor_port.h"

/**
 * @brief 读取物料颜色识别结果
 * @return 颜色识别结果; 失败时 color_name 为 NULL, confidence 为 0
 */
SENSOR_ColorResult_t RPI_Read_Color(void) {
    SENSOR_ColorResult_t res = {NULL, 0};
    return res;
}
