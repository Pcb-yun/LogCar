/**
 * @file nav_tracker.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航轨迹跟踪源文件
 */

#include "nav_tracker.h"
#include "cmsis_os2.h"
#include "Events.h"


/**
 * @brief 导航轨迹跟踪初始化
 * @return 初始化结果
 */
bool Nav_Track_Init(void) {



    return true;
}

/**
 * @brief 导航轨迹跟踪任务
 */
void Nav_Track_Task(void *argument){

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for(;;) {



        osDelay(10);
    }
}
