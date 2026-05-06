/**
 * @file rtos_utils.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief RTOS实用命令集源文件
 */

#include "rtos_utils.h"
#include "shell.h"
#include "log.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"
#include <string.h>


/**
 * @brief 显示内存分配信息
 */
static void Memory_Info(void) {
    HeapStats_t xHeapStats;

    vPortGetHeapStats(&xHeapStats);

    logPrintln("Free heap: %u bytes", xHeapStats.xAvailableHeapSpaceInBytes);
    logPrintln("Min ever free: %u bytes", xHeapStats.xMinimumEverFreeBytesRemaining);
    logPrintln("Largest free block: %u bytes", xHeapStats.xSizeOfLargestFreeBlockInBytes);
    logPrintln("Smallest free block: %u bytes", xHeapStats.xSizeOfSmallestFreeBlockInBytes);
    logPrintln("Number of free blocks: %u", xHeapStats.xNumberOfFreeBlocks);
    logPrintln("Successful allocations: %u", xHeapStats.xNumberOfSuccessfulAllocations);
    logPrintln("Successful frees: %u", xHeapStats.xNumberOfSuccessfulFrees);
}

/**
 * @brief 显示任务状态信息
 */
static void Task_Info(void) {
    osThreadId_t task_ids[32];
    uint32_t task_count = osThreadEnumerate(task_ids, 32);

    logPrintln("ID        Name          State       Priority    Stack Space    High Water    Runtime");
    logPrintln("------------------------------------------------------------------------------------");

    // 获取总运行时间
    uint32_t ulTotalRunTime;
    TaskStatus_t *pxTaskStatusArray;
    uint32_t ulArraySize;

    // 获取任务数量
    ulArraySize = uxTaskGetNumberOfTasks();

    pxTaskStatusArray = pvPortMalloc(ulArraySize * sizeof(TaskStatus_t));

    if(pxTaskStatusArray != NULL) {
        ulArraySize = uxTaskGetSystemState(pxTaskStatusArray, ulArraySize, &ulTotalRunTime);

        for(uint32_t i = 0; i < task_count; i++) {
            const char *task_name = osThreadGetName(task_ids[i]);
            osThreadState_t state = osThreadGetState(task_ids[i]);
            osPriority_t priority = osThreadGetPriority(task_ids[i]);
            uint32_t stack_space = osThreadGetStackSpace(task_ids[i]);

            // 获取任务高水位线和运行时间
            TaskStatus_t xTaskStatus;
            TaskHandle_t xTask = (TaskHandle_t)task_ids[i];
            vTaskGetInfo(xTask, &xTaskStatus, pdTRUE, eInvalid);
            uint32_t high_water = xTaskStatus.usStackHighWaterMark;
            unsigned long ulRunTimeCounter = xTaskStatus.ulRunTimeCounter;

            // 计算CPU使用率
            float cpu_usage = 0.0f;
            if(ulTotalRunTime > 0) {
                cpu_usage = (float)ulRunTimeCounter / (float)ulTotalRunTime * 100.0f;
            }

            const char *state_str;
            switch(state) {
                case osThreadInactive: state_str = "Inactive"; break;
                case osThreadReady: state_str = "Ready"; break;
                case osThreadRunning: state_str = "Running"; break;
                case osThreadBlocked: state_str = "Blocked"; break;
                case osThreadTerminated: state_str = "Terminated"; break;
                default: state_str = "Unknown"; break;
            }
            logPrintln("%p  %-12s  %-10s  %-10lu  %-10lu     %-12lu  %-4.2f %%",
                      task_ids[i], task_name ? task_name : "<unknown>",
                      state_str, priority, stack_space, high_water, cpu_usage);
        }

        vPortFree(pxTaskStatusArray);
    } else {
        logPrintln("Failed to allocate memory for task status array");
    }
}

/**
 * @brief 显示系统时间信息
 */
static void Time_Info(void) {
    uint32_t tick_count = osKernelGetTickCount();
    uint32_t tick_freq = osKernelGetTickFreq();

    uint32_t hours = tick_count / (tick_freq * 3600);
    uint32_t minutes = (tick_count % (tick_freq * 3600)) / (tick_freq * 60);
    uint32_t seconds = (tick_count % (tick_freq * 60)) / tick_freq;
    uint32_t milliseconds = (tick_count % tick_freq) * 1000 / tick_freq;

    logPrintln("Tick Frequency: %lu Hz", tick_freq);
    logPrintln("System Tick: %lu", tick_count);
    logPrintln("Uptime: %02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, milliseconds);

    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    uint32_t hclk = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

    logPrintln("System Clock: %lu Hz", sysclk);
    logPrintln("HCLK: %lu Hz", hclk);
    logPrintln("PCLK1: %lu Hz", pclk1);
    logPrintln("PCLK2: %lu Hz", pclk2);
}

/**
 * @brief OS命令处理函数
 * @param argc 参数数量
 * @param argv 参数列表
 */
static void OS_Tool_Shell(int argc, char *argv[]) {
    if(argc < 2) {
        logPrintln(OS_HELP);
        return;
    }

    if(strcmp(argv[1], "mem") == 0) {
        Memory_Info();
    } else if(strcmp(argv[1], "task") == 0) {
        Task_Info();
    } else if(strcmp(argv[1], "time") == 0) {
        Time_Info();
    } else {
        logPrintln("Invalid os command: %s", argv[1]);
        logPrintln(OS_HELP);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
os, OS_Tool_Shell, View system information);
