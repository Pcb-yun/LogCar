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
#define SHELL_ONLINE_CHECK_TIME 2000     // shell在线检查时间间隔，单位：ms

void userShellInit(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
