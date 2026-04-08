#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "cmsis_os2.h"

/*                  系统状态                    */
extern osEventFlagsId_t System_StatusHandle;
#define SYS_INIT_COMPLETE (1UL << 0)  // 系统初始化完成
#define APP_NEED_USART (1UL << 1)  // 需要使用串口



#endif // __EVENTS_H__
