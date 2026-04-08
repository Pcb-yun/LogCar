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

Shell shell;
char shellBuffer[512];

/**
* @brief 用户shell写
* @param data 数据
* @param len 数据长度
* @return short 实际写入的数据长度
*/
short userShellWrite(char *data, unsigned short len) {
   HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 500);
   return len;
}

/**
* @brief 用户shell初始化
*/
void userShellInit(void) {
   shell.write = userShellWrite;
   shellInit(&shell, shellBuffer, 512);
}

/**
* @brief 用户shell任务
* @param argument 任务参数
*/
void Shell_Task(void *argument) {
   extern osSemaphoreId_t Usart1_Rx_DataHandle;
   (void)argument;

   // 等待系统初始化完成
   osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

   char data;
   for(;;) {
      // 优先将数据发送给需要使用串口的应用程序
      if(osEventFlagsGet(System_StatusHandle) & APP_NEED_USART) {
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
void Sys_Reset(int argc, char *argv[]) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    uint8_t bytes_read = 0;
    osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

    // 检查是否有-y参数，如果有则直接执行重置
    if (argc > 1 && strcmp(argv[1], "-y") == 0) {
        logPrintln("system will reset after 1 seconds");
        osDelay(1000);
        HAL_NVIC_SystemReset();
    }

    logPrintln("WARNING: System will be reset, Would you like to proceed? (y/n)");

    if(osMessageQueueGet(Usart1_Rx_DataHandle, &bytes_read, NULL, 5000) == osOK) {
        if (bytes_read == 'y') {
            logPrintln("system will reset after 1 seconds");
            osDelay(1000);
            HAL_NVIC_SystemReset();
        }
    }
    osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
Reset, Sys_Reset, Rest System);

/**
 * @brief 设置红灯状态
 * @param argc 参数数量
 * @param argv 参数列表
 */
void Led(int argc, char *argv[]) {
   if(argc != 2) {
      logError("invalid arguments");
      return;
   }

   if(argv[1][0] == '0') {
      HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET);  // 熄灭红灯
   } else if(argv[1][0] == '1') {
      HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);  // 点亮红灯
   }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
Led, Led, Set Red LED);
