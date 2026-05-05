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

/**
 * @brief 定位数据结构体
 */
typedef struct {
#if OPS_USE_POS
    float x;
    float y;
#endif
#if OPS_USE_YAW
    float yaw;
#endif
#if OPS_USE_PITCH
    float pitch;
#endif
#if OPS_USE_ROLL
    float roll;
#endif
#if OPS_USE_ANG_VEL
    float w_z;
#endif
} OPSData_t;


void OPS_Init(void);



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
