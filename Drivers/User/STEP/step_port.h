/**
 * @file step_port.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机端口层头文件
 */

#ifndef __STEP_PORT_H__
#define __STEP_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "step_cmd.h"
#include "step_can.h"


bool Motor_Init(void);
bool Motor_Send_Cmd(MotorCmd_t *cmd);




#define ZDT_V5_SEND_CMD(cmd, len) Motor_CAN_Send(cmd, len)

#define MOTOR_SET_TIME_HELP \
    "Usage: time COMMAND [value] (ms)" \
    "\r\n" \
    "commands:\r\n" \
    "  send      Set Send time\r\n" \
    "  get       Set Get time"


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_PORT_H__ */
