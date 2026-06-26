/**
 * @file step_port_def.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 端口层宏定义头文件
 */

#ifndef __STEP_PORT_DEF_H__
#define __STEP_PORT_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

void Motor_TTL_Send(uint8_t *cmd, uint8_t len);

// 发送命令宏
#define ZDT_V5_SEND_CMD(cmd, len) Motor_TTL_Send(cmd, len)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_PORT_DEF_H__ */
