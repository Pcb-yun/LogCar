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
#include "Events.h"
#include <string.h>


/**
 * @brief 显示内存分配信息
 */
static void Memory_Info(void) {
    HeapStats_t xHeapStats;

    vPortGetHeapStats(&xHeapStats);

    logPrintln("Free heap: %u bytes\r\n"
               "Min ever free: %u bytes\r\n"
               "Largest free block: %u bytes\r\n"
               "Smallest free block: %u bytes\r\n"
               "Number of free blocks: %u\r\n"
               "Successful allocations: %u\r\n"
               "Successful frees: %u",
               xHeapStats.xAvailableHeapSpaceInBytes,
               xHeapStats.xMinimumEverFreeBytesRemaining,
               xHeapStats.xSizeOfLargestFreeBlockInBytes,
               xHeapStats.xSizeOfSmallestFreeBlockInBytes,
               xHeapStats.xNumberOfFreeBlocks,
               xHeapStats.xNumberOfSuccessfulAllocations,
               xHeapStats.xNumberOfSuccessfulFrees);
}

/**
 * @brief 显示任务状态信息
 */
static void Task_Info(void) {
    osThreadId_t task_ids[32];
    uint32_t task_count = osThreadEnumerate(task_ids, 32);

    logPrintln("ID        Name          State       Priority    Stack Space    High Water    Runtime\r\n"
               "------------------------------------------------------------------------------------");

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

    logPrintln("Tick Frequency: %lu Hz\r\n"
            "System Tick: %lu\r\n"
            "Uptime: %02lu:%02lu:%02lu.%03lu",
            tick_freq, tick_count, hours,
            minutes, seconds, milliseconds);

    uint32_t sysclk = HAL_RCC_GetSysClockFreq();
    uint32_t hclk = HAL_RCC_GetHCLKFreq();
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

    logPrintln("System Clock: %lu Hz\r\n"
            "HCLK: %lu Hz\r\n"
            "PCLK1: %lu Hz\r\n"
            "PCLK2: %lu Hz",
            sysclk, hclk, pclk1, pclk2);
}

/**
 * @brief 显示消息队列信息
 */
static void Queue_Info(void) {
    logPrintln("Message Queue information:");

    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    extern osMessageQueueId_t MotorCmdsHandle;
    extern osMessageQueueId_t Usart6_Rx_DataHandle;
    extern osMessageQueueId_t Uart4_Rx_DataHandle;
    extern osMessageQueueId_t Servo_CmdHandle;
    extern osMessageQueueId_t Servo_Tx_DataHandle;

    logPrintln("ID        Name                  Count/Max    MsgSize");
    logPrintln("----------------------------------------------------");

    if (Usart1_Rx_DataHandle != NULL) {
        uint32_t count = osMessageQueueGetCount(Usart1_Rx_DataHandle);
        uint32_t capacity = osMessageQueueGetCapacity(Usart1_Rx_DataHandle);
        uint32_t msg_size = osMessageQueueGetMsgSize(Usart1_Rx_DataHandle);
        logPrintln("%p  %-20s %3lu/%-3lu   %4lu bytes",
                  Usart1_Rx_DataHandle, "Usart1_Rx_Data",
                  count, capacity, msg_size);
    }

    if (MotorCmdsHandle != NULL) {
        uint32_t count = osMessageQueueGetCount(MotorCmdsHandle);
        uint32_t capacity = osMessageQueueGetCapacity(MotorCmdsHandle);
        uint32_t msg_size = osMessageQueueGetMsgSize(MotorCmdsHandle);
        logPrintln("%p  %-20s %3lu/%-3lu   %4lu bytes",
                  MotorCmdsHandle, "MotorCmds",
                  count, capacity, msg_size);
    }

    if (Usart6_Rx_DataHandle != NULL) {
        uint32_t count = osMessageQueueGetCount(Usart6_Rx_DataHandle);
        uint32_t capacity = osMessageQueueGetCapacity(Usart6_Rx_DataHandle);
        uint32_t msg_size = osMessageQueueGetMsgSize(Usart6_Rx_DataHandle);
        logPrintln("%p  %-20s %3lu/%-3lu   %4lu bytes",
                  Usart6_Rx_DataHandle, "Usart6_Rx_Data",
                  count, capacity, msg_size);
    }

    if (Uart4_Rx_DataHandle != NULL) {
        uint32_t count = osMessageQueueGetCount(Uart4_Rx_DataHandle);
        uint32_t capacity = osMessageQueueGetCapacity(Uart4_Rx_DataHandle);
        uint32_t msg_size = osMessageQueueGetMsgSize(Uart4_Rx_DataHandle);
        logPrintln("%p  %-20s %3lu/%-3lu   %4lu bytes",
                  Uart4_Rx_DataHandle, "Uart4_Rx_Data",
                  count, capacity, msg_size);
    }

    if (Servo_CmdHandle != NULL) {
        uint32_t count = osMessageQueueGetCount(Servo_CmdHandle);
        uint32_t capacity = osMessageQueueGetCapacity(Servo_CmdHandle);
        uint32_t msg_size = osMessageQueueGetMsgSize(Servo_CmdHandle);
        logPrintln("%p  %-20s %3lu/%-3lu   %4lu bytes",
                  Servo_CmdHandle, "Servo_Cmd",
                  count, capacity, msg_size);
    }

    if (Servo_Tx_DataHandle != NULL) {
        uint32_t count = osMessageQueueGetCount(Servo_Tx_DataHandle);
        uint32_t capacity = osMessageQueueGetCapacity(Servo_Tx_DataHandle);
        uint32_t msg_size = osMessageQueueGetMsgSize(Servo_Tx_DataHandle);
        logPrintln("%p  %-20s %3lu/%-3lu   %4lu bytes",
                  Servo_Tx_DataHandle, "Servo_Tx_Data",
                  count, capacity, msg_size);
    }
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
    } else if(strcmp(argv[1], "queue") == 0) {
        Queue_Info();
    } else {
        logPrintln("Invalid command: %s\r\n"
                OS_HELP, argv[1]);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
os, OS_Tool_Shell, View system information);
