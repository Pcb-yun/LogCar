/**
 * @file ops.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 平面定位模块源文件
 */

#include "ops.h"
#include "usart.h"
#include "Events.h"


/**
 * @brief 初始化定位模块
 */
void OPS_Init(void) {
    MX_UART4_Init();


}

/**
 * @brief 定位更新任务
 */
void OPS_Update_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for(;;) {
        osDelay(1000);


    }
}
