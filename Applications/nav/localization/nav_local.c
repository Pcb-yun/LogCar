/**
 * @file nav_local.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航定位源文件
 */

#include "FreeRTOS.h"
#include "nav_local.h"
#include "cmsis_os2.h"
#include "Events.h"
#include <string.h>
#include "track.h"

static Pose_t *g_pose = NULL;
static void Loc_Update(void);


/**
 * @brief 导航定位初始化
 * @return 初始化结果
 */
bool Loc_Init(void) {
    g_pose = pvPortMalloc(sizeof(Pose_t));
    if(g_pose == NULL) {
        return false;
    }
    memset(g_pose, 0, sizeof(Pose_t));



    return true;
}

/**
 * @brief 导航定位任务
 */
void Loc_Update_Task(void *argument) {

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for(;;) {
		Loc_Update();
        osDelay(1000);
    }
}

/**
 * @brief 导航定位更新
 */
static void Loc_Update(void) {



}

/**
 * @brief 导航定位设置
 * @param pose 导航位姿指针
 */
void Loc_Set(Pose_t *pose) {
    extern osMutexId_t Pose_MutexHandle;

    if (osMutexAcquire(Pose_MutexHandle, osWaitForever) == osOK) {
        *g_pose = *pose;
        g_pose->timestamp = osKernelGetTickCount();
        osMutexRelease(Pose_MutexHandle);
    }
}
