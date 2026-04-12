/**
 * @file shell_port.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-02-22
 *
 * @copyright (c) 2019 Letter
 *
 */

#include "FreeRTOS.h"
#include "task.h"
#include "Events.h"
#include "shell.h"
#include "log.h"
#include "usart.h"
#include "string.h"

#define SHELL_BUFFER_SIZE 1024

Shell shell;
char shellBuffer[SHELL_BUFFER_SIZE];

/**
* @brief 用户shell写
* @param data 数据
* @param len 数据长度
* @return 实际写入的数据长度
*/
static short userShellWrite(char *data, unsigned short len) {
   osEventFlagsClear(System_StatusHandle, UART1_TX_IDLE);
   HAL_UART_Transmit_DMA(&huart1, (uint8_t *)data, len);
   osEventFlagsWait(System_StatusHandle, UART1_TX_IDLE, osFlagsWaitAny, osWaitForever);
   return len;
}

/**
* @brief 用户shell读
* @param data 数据缓冲区
* @param len 数据长度
* @return 实际读取的数据长度
*/
static short userShellRead(char *data, unsigned short len) {
   extern osMessageQueueId_t Usart1_Rx_DataHandle;
   if (len == 0) return 0;

   if (osMessageQueueGet(Usart1_Rx_DataHandle, data, NULL, 100) == osOK) {
       return 1;
   }
   return 0;
}

/**
* @brief 用户shell初始化
*/
void userShellInit(void) {
   shell.write = userShellWrite;
   shell.read = userShellRead;
   shellInit(&shell, shellBuffer, SHELL_BUFFER_SIZE);
}

/**
* @brief 用户shell任务
* @param argument 任务参数
*/
void Shell_Task(void *argument) {
   extern osMessageQueueId_t Usart1_Rx_DataHandle;
   (void)argument;

   osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

   char data;
   for(;;) {
      // 优先将数据发送给需要使用串口的应用程序
      if(osEventFlagsGet(System_StatusHandle) & APP_NEED_USART) {
         osDelay(100);
         continue;
      }
      if(osMessageQueueGet(Usart1_Rx_DataHandle, &data, NULL, 200) == osOK) {
         shellHandler(&shell, data);
      }
   }
}

/**
 * @brief 软件重置
 * @param argc 参数数量
 * @param argv 参数列表
 */
static void Sys_Reset(int argc, char *argv[]) {
    char ch = 0;

    if (argc > 1 && strcmp(argv[1], "-y") == 0) {
        logPrintln("system will reset after 1 seconds");
        osDelay(1000);
        HAL_NVIC_SystemReset();
    }

    logPrintln("WARNING: System will be reset, Would you like to proceed? (y/n)");

    while (1) {
        if (shell.read(&ch, 1) == 1) {
            if (ch == 'y') {
                logPrintln("system will reset after 1 seconds");
                osDelay(1000);
                HAL_NVIC_SystemReset();
            } else {
               break;
            }
        }
        osDelay(100);
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
Reset, Sys_Reset, Rest System);
