/**
 * @file rpi_sensor_port.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 树莓派通信接口
 */

#ifndef __RPI_SENSOR_PORT_H__
#define __RPI_SENSOR_PORT_H__

#include <stdbool.h>
#include <stdint.h>
#include "sensor.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    RPI_CAL_TYPE_MATL = 0,      // 物料
    RPI_CAL_TYPE_TROP1,         // 冠军
    RPI_CAL_TYPE_TROP2,         // 亚军
    RPI_CAL_TYPE_TROP3          // 季军
} RPI_CalType_t;

SENSOR_Color_t RPI_DetectColor(void);
bool RPI_Calibrate(int16_t *err_x, int16_t *err_y, RPI_CalType_t type);
void RPI_Detect_IDE(void);
void RPI_Calibrate_IDE(void);


#define RPI_HELP \
    "Usage: rpi COMMAND\r\n" \
    "commands:\r\n" \
    "  det         Detect color color\r\n" \
    "  cal         Calibrate offset"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RPI_SENSOR_ PORT_H__ */
