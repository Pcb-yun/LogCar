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
#include "step_ttl.h"


bool Motor_Init(void);
bool Motor_Send_Cmd(MotorCmd_t *cmd);


// 是否使用查看任务
// 当前程序架构存在问题，除调试阶段否则不建议使用
// 如果使能且启用了大量的功能开关，会严重影响控制性能
#define USE_VIEW 0

// 发送命令宏
#define ZDT_V5_SEND_CMD(cmd, len) Motor_TTL_Send(cmd, len)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_PORT_H__ */
