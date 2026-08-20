/**
 * @file ops.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 平面定位模块头文件
 */

#ifndef __OPS_H__
#define __OPS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ops_cfg.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 定位数据结构体
 */
typedef struct {
#if OPS_USE_POS
    float x;        // X坐标 (mm)
    float y;        // Y坐标 (mm)
#endif
#if OPS_USE_YAW
    float yaw;      // 航向 (deg)
#endif
#if OPS_USE_PITCH
    float pitch;    // 俯仰
#endif
#if OPS_USE_ROLL
    float roll;     // 滚转
#endif
#if OPS_USE_ANG_VEL
    float w_z;      // 角速度
#endif
    uint32_t timestamp; // 时间戳
} OPSData_t;


bool OPS_Init(void);
bool OPS_Get(OPSData_t *pose);
void OPS_Zero(void);
bool OPS_Is_Ready(void);


#define OPS_SET_HELP \
    "Usage: set COMMAND [value]\r\n" \
    "\r\n" \
    "commands:\r\n" \
    "  j         Set yaw angle\r\n" \
    "  x         Set X coordinate\r\n" \
    "  y         Set Y coordinate"



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OPS_H__ */
