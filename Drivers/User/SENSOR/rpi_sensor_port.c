/**
 * @file rpi_sensor_port.c
 * @brief 树莓派通信接口（用户实现）
 */

#include "rpi_sensor_port.h"

/**
 * @brief 读取奖杯上印制的字母
 * @return 识别到的字母 ('A'/'B'/'C'); 读取失败返回 0
 */
char RPI_Read_Letter(void) {
    return 0;
}

/**
 * @brief 读取物料颜色识别结果
 * @return 颜色识别结果; 失败时 color_name 为 NULL, confidence 为 0
 */
SENSOR_ColorResult_t RPI_Read_Color(void) {
    SENSOR_ColorResult_t res = {NULL, 0};
    return res;
}
