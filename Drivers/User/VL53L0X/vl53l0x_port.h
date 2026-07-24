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


#define DIST_TimeBudget 33000     // 测量时间预算(µs)

// 手动校准值（运行dist cal all后将输出值填入此处）
#define CAL_REF_SPAD_COUNT      4
#define CAL_IS_APERTURE_SPADS   1
#define CAL_VHV_SETTINGS        33
#define CAL_PHASE_CAL           1
#define CAL_OFFSET_UM           20000


bool VL53L0X_Init(void);
void Dist_Get_Task(void *argument);
uint16_t Dist_Get(void);





#define DIST_CAL_ALL_HELP \
    "Usage: dist cal TYPE [param]\r\n" \
    "Types:\r\n" \
    "  ref         - Reference calibration (VHV/Phase, auto)\r\n" \
    "  spad        - SPAD management (auto)\r\n" \
    "  offset [mm] - Offset calibration at given distance\r\n" \
    "  xwalk [mm]  - XTalk calibration at given distance\r\n" \
    "  all         - Full: spad->ref->offset->xwalk (UM2039)\r\n" \
    "  show        - Show current calibration values"


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VL53L0X_PORT_H__ */
