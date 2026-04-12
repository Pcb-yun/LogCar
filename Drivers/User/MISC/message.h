/**
 * @file message.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 拓展消息显示模块头文件
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief 消息状态枚举
 */
typedef enum {
    dmesg_wait = 0,     // 等待
    dmesg_ok,           // 成功
    dmesg_fail,         // 失败
} dmesg_status;

// 是否启用拓展消息显示模块
#define MESSAGE_ENABLE 1
#if MESSAGE_ENABLE

#include "usart.h"

void Show_dmesg(const dmesg_status status, const char *string);
#define SHOW_DMESG(status, string) Show_dmesg(status, string)









#else /* MESSAGE_ENABLE */
#define SHOW_DMESG(status, string)

#endif /* MESSAGE_ENABLE */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MESSAGE_H__ */
