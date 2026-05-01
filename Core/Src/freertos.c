/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Events.h"
#include "tim.h"
#include "shell.h"
#include "message.h"
#include "track.h"
#include "usart.h"
#include "step_port.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for Sys_Init */
osThreadId_t Sys_InitHandle;
const osThreadAttr_t Sys_Init_attributes = {
  .name = "Sys_Init",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime7,
};
/* Definitions for Shell */
osThreadId_t ShellHandle;
const osThreadAttr_t Shell_attributes = {
  .name = "Shell",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};
/* Definitions for Track_Get */
osThreadId_t Track_GetHandle;
const osThreadAttr_t Track_Get_attributes = {
  .name = "Track_Get",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};
/* Definitions for Motor_Get_Sta */
osThreadId_t Motor_Get_StaHandle;
const osThreadAttr_t Motor_Get_Sta_attributes = {
  .name = "Motor_Get_Sta",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for Motor_Ctrl */
osThreadId_t Motor_CtrlHandle;
const osThreadAttr_t Motor_Ctrl_attributes = {
  .name = "Motor_Ctrl",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for Motor_Update */
osThreadId_t Motor_UpdateHandle;
const osThreadAttr_t Motor_Update_attributes = {
  .name = "Motor_Update",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for Usart1_Rx_Data */
osMessageQueueId_t Usart1_Rx_DataHandle;
const osMessageQueueAttr_t Usart1_Rx_Data_attributes = {
  .name = "Usart1_Rx_Data"
};
/* Definitions for Usart2_Rx_Data */
osMessageQueueId_t Usart2_Rx_DataHandle;
const osMessageQueueAttr_t Usart2_Rx_Data_attributes = {
  .name = "Usart2_Rx_Data"
};
/* Definitions for Track_Data */
osMessageQueueId_t Track_DataHandle;
const osMessageQueueAttr_t Track_Data_attributes = {
  .name = "Track_Data"
};
/* Definitions for MotorCmds */
osMessageQueueId_t MotorCmdsHandle;
const osMessageQueueAttr_t MotorCmds_attributes = {
  .name = "MotorCmds"
};
/* Definitions for Usart6_Rx_Data */
osMessageQueueId_t Usart6_Rx_DataHandle;
const osMessageQueueAttr_t Usart6_Rx_Data_attributes = {
  .name = "Usart6_Rx_Data"
};
/* Definitions for System_Status */
osEventFlagsId_t System_StatusHandle;
const osEventFlagsAttr_t System_Status_attributes = {
  .name = "System_Status"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Sys_Init_Task(void *argument);
extern void Shell_Task(void *argument);
extern void Track_Get_Task(void *argument);
extern void Motor_Get_Sta_Task(void *argument);
extern void Motor_Ctrl_Task(void *argument);
extern void Motor_Update_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
  MX_TIM2_Init();
}

__weak unsigned long getRunTimeCounterValue(void)
{
  return __HAL_TIM_GET_COUNTER(&htim2);
}
/* USER CODE END 1 */

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */

  // 空闲时绿灯闪烁
  HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_10);
}
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
   my_printf("\r\n[ERROR] Stack overflow detected!\r\n");

   my_printf("[ERROR] Task name: %s\r\n", pcTaskName);
   my_printf("[ERROR] Task ID: 0x%p\r\n", xTask);

   Error_Handler();
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
   taskDISABLE_INTERRUPTS();
   my_printf("\r\n[ERROR] Memory allocation failed!\r\n");

   // 获取当前任务信息
   TaskHandle_t xCurrentTask = xTaskGetCurrentTaskHandle();
   if(xCurrentTask != NULL) {
     const char *pcTaskName = pcTaskGetName(xCurrentTask);
     my_printf("Current task: %s\r\n", pcTaskName);
     my_printf("Task handle: 0x%p\r\n", xCurrentTask);
   }

   // 打印空闲堆信息
   size_t xFreeHeapSize = xPortGetFreeHeapSize();
   size_t xMinimumEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
   my_printf("Free heap size: %u bytes\r\n", xFreeHeapSize);
   my_printf("Min ever free heap: %u bytes\r\n", xMinimumEverFreeHeapSize);

   Error_Handler();
 }
/* USER CODE END 5 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of Usart1_Rx_Data */
  Usart1_Rx_DataHandle = osMessageQueueNew (32, sizeof(uint8_t), &Usart1_Rx_Data_attributes);

  /* creation of Usart2_Rx_Data */
  Usart2_Rx_DataHandle = osMessageQueueNew (64, sizeof(uint8_t), &Usart2_Rx_Data_attributes);

  /* creation of Track_Data */
  Track_DataHandle = osMessageQueueNew (1, sizeof(TrackData_t), &Track_Data_attributes);

  /* creation of MotorCmds */
  MotorCmdsHandle = osMessageQueueNew (8, sizeof(MotorCmd_t), &MotorCmds_attributes);

  /* creation of Usart6_Rx_Data */
  Usart6_Rx_DataHandle = osMessageQueueNew (5, sizeof(Usart6_RxBuf_t), &Usart6_Rx_Data_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Sys_Init */
  Sys_InitHandle = osThreadNew(Sys_Init_Task, NULL, &Sys_Init_attributes);

  /* creation of Shell */
  ShellHandle = osThreadNew(Shell_Task, NULL, &Shell_attributes);

  /* creation of Track_Get */
  Track_GetHandle = osThreadNew(Track_Get_Task, NULL, &Track_Get_attributes);

  /* creation of Motor_Get_Sta */
  Motor_Get_StaHandle = osThreadNew(Motor_Get_Sta_Task, NULL, &Motor_Get_Sta_attributes);

  /* creation of Motor_Ctrl */
  Motor_CtrlHandle = osThreadNew(Motor_Ctrl_Task, NULL, &Motor_Ctrl_attributes);

  /* creation of Motor_Update */
  Motor_UpdateHandle = osThreadNew(Motor_Update_Task, NULL, &Motor_Update_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of System_Status */
  System_StatusHandle = osEventFlagsNew(&System_Status_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Sys_Init_Task */
/**
  * @brief  Function implementing the Sys_Init thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Sys_Init_Task */
void Sys_Init_Task(void *argument)
{
  /* USER CODE BEGIN Sys_Init_Task */
  (void)argument;

  SHOW_DMESG(dmesg_wait, "Initialize Shell");
  extern void userShellInit(void);
  userShellInit();
  SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize shell log.");
  extern void logInit(void);
  logInit();
  SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Tracking Module");
  Track_Init();
  SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Motor Module");
  if (!Motor_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else {
    SHOW_DMESG(dmesg_ok, NULL);
    SHOW_DMESG(dmesg_wait, "Initialize Motion Control Module");
    extern void MotionControl_Init(void);
    MotionControl_Init();
    SHOW_DMESG(dmesg_ok, NULL);
  }

  extern Shell shell;
  Shell_New_Convo(&shell);

  osEventFlagsSet(System_StatusHandle, SYS_INIT_COMPLETE);
	vTaskDelete(NULL);
  /* USER CODE END Sys_Init_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

