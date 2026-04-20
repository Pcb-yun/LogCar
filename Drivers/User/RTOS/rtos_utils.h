/**
 * @file rtos_utils.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief RTOS实用命令集头文件
 */

#ifndef __RTOS_UTILS_H__
#define __RTOS_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// OS命令帮助信息
#define OS_HELP \
    "Usage: os COMMAND\r\n" \
    "\r\n" \
    "commands:\r\n" \
    "  mem      Show memory status\r\n" \
    "  task     Show task status\r\n" \
    "  time     Show system time status"



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RTOS_UTILS_H__ */
