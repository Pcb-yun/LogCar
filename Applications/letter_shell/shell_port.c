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
#include "shell_port.h"
#include "log.h"
#include "usart.h"
#include "string.h"


Shell shell;
char shellBuffer[SHELL_BUFFER_SIZE];

/**
 * @brief 用户shell写
 * @param data 数据
 * @param len 数据长度
 * @return 实际写入的数据长度
 */
static short userShellWrite(char *data, uint32_t len) {
    uint32_t total_sent = 0;
    const uint16_t MAX_DMA_LEN = 0xFFFF;  // DMA单次最大发送长度

    while (len > 0) {
        // 计算本次发送长度，不超过DMA限制
        uint16_t send_len = len > MAX_DMA_LEN ? MAX_DMA_LEN : (uint16_t)len;

        // 发送数据
        osEventFlagsClear(System_StatusHandle, UART1_TX_IDLE);
        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)data, send_len);
        osEventFlagsWait(System_StatusHandle, UART1_TX_IDLE, osFlagsWaitAny, osWaitForever);

        // 更新进度
        total_sent += send_len;
        data += send_len;
        len -= send_len;
    }

    return total_sent;
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

   if (osMessageQueueGet(Usart1_Rx_DataHandle, data, NULL, 0) == osOK) {
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
         osDelay(10);
         continue;
      }
      if(osMessageQueueGet(Usart1_Rx_DataHandle, &data, NULL, 50) == osOK) {
         shellHandler(&shell, data);
      }
   }
}

/**
 * @brief 在线检查任务
 */
void Online_Check_Task(void *argument) {
    extern osMessageQueueId_t Usart1_Rx_DataHandle;
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for (;;) {
        osDelay(SHELL_ONLINE_CHECK_TIME);
        if((osEventFlagsGet(System_StatusHandle) & APP_NEED_USART)) continue;
        osEventFlagsSet(System_StatusHandle, APP_NEED_USART);

        shell.write((char[]){0x05}, 1);    // 查询指令(ENQ)0x05

        uint8_t read_count = 0;
        uint8_t read_buf[5] = {0};

        // 尝试读取4个字节
        while(read_count < 4) {
           if(osMessageQueueGet(Usart1_Rx_DataHandle, &read_buf[read_count], NULL, 50) == osOK) {
               read_count++;
           } else {
               osEventFlagsClear(System_StatusHandle, SHELL_ONLINE);
               break; // 读取超时，退出循环
           }
        }

       if(read_count == 4) {
           if(strcmp((char*)read_buf, "Wind") == 0) {
               if((osEventFlagsGet(System_StatusHandle) & SHELL_ONLINE) == 0) {
                   osEventFlagsSet(System_StatusHandle, SHELL_ONLINE);
                   Shell_New_Convo(&shell);
               }
           } else {
               osEventFlagsClear(System_StatusHandle, SHELL_ONLINE);
           }
       }
       osEventFlagsClear(System_StatusHandle, APP_NEED_USART);
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
