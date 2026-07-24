/**
 * @file oled_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief OLED模块用户层源文件
 */

#include "oled_port.h"
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Events.h"
#include "shell.h"
#include "log.h"
#include "spi.h"


/**
 * @brief OLED初始化函数
 */
void OLED_Init(void) {
    MX_SPI1_Init();
    ssd1306_Init();
}

/**
 * @brief OLED刷新任务
 */
void Oled_Refresh_Task(void *argument) {
    (void) argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    extern osMutexId_t OLED_MutexHandle;

    for (;;) {
        osDelay(33);
        osMutexAcquire(OLED_MutexHandle, osWaitForever);
        ssd1306_UpdateScreen();
        osMutexRelease(OLED_MutexHandle);
    }
}

/**
 * @brief OLED测试函数
 */
static void OLED_test(void) {
    extern TaskHandle_t Oled_RefreshHandle;
    vTaskSuspend(Oled_RefreshHandle);
    ssd1306_TestAll();
    vTaskResume(Oled_RefreshHandle);
}
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    oled_test, OLED_test, Test oled driver);
