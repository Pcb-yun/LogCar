/**
 * @file Events.h
 * @brief 事件定义
 * @author Pcb-yun (pcbyinyun@163.com)
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "cmsis_os2.h"

/*                  系统状态                    */
extern osEventFlagsId_t System_StatusHandle;
#define SYS_INIT_COMPLETE (1UL << 0)    // 系统初始化完成
#define APP_NEED_USART (1UL << 1)       // 需要使用串口
#define UART1_TX_IDLE (1UL << 2)        // UART1发送空闲
#define UART2_RX_CPLT (1UL << 3)        // UART2接收完成
#define UART6_TX_IDLE (1UL << 4)        // UART6发送空闲
#define UART3_TX_IDLE (1UL << 5)        // UART3发送空闲
#define UART3_RX_IDLE (1UL << 6)        // UART3接收空闲
#define ADC1_CONVCPLT (1UL << 7)        // ADC1转换完成
#define SHELL_ONLINE (1UL << 8)         // shell在线

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __EVENTS_H__
