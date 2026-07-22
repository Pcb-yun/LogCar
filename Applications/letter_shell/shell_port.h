/**
 * @file shell_port.h
 * @author Letter (NevermindZZT@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-02-22
 *
 * @copyright (c) 2019 Letter
 *
 */

#ifndef __SHELL_PORT_H__
#define __SHELL_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "shell.h"

extern Shell shell;

#define SHELL_BUFFER_SIZE 1024          // shell缓冲区大小

#define ONLINE_CHECK_RESPONSE "Wind"    // shell在线检查终端响应信息
#define ONLINE_CHECK_TIME 1000           // shell在线检查时间间隔(ms)
#define ONLINE_CHECK_TIMEOUT 100         // shell在线检查超时时间(ms)
#define ONLINE_CHECK_RETRY 3            // shell在线检查重试次数

void userShellInit(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
