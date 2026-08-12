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
#include "zdt_v5_cmd.h"
#include "ops.h"
#include "servo_driver.h"
#include "servo_port.h"
#include "nav_track.h"
#include "scan_driver.h"
#include "show_media.h"

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
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityRealtime7,
};
/* Definitions for Shell */
osThreadId_t ShellHandle;
const osThreadAttr_t Shell_attributes = {
  .name = "Shell",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for Track_Get */
osThreadId_t Track_GetHandle;
const osThreadAttr_t Track_Get_attributes = {
  .name = "Track_Get",
  .stack_size = 64 * 4,
  .priority = (osPriority_t) osPriorityHigh3,
};
/* Definitions for Motor_Receive */
osThreadId_t Motor_ReceiveHandle;
const osThreadAttr_t Motor_Receive_attributes = {
  .name = "Motor_Receive",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh3,
};
/* Definitions for Motor_Ctrl */
osThreadId_t Motor_CtrlHandle;
const osThreadAttr_t Motor_Ctrl_attributes = {
  .name = "Motor_Ctrl",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh4,
};
/* Definitions for Motor_Update */
osThreadId_t Motor_UpdateHandle;
const osThreadAttr_t Motor_Update_attributes = {
  .name = "Motor_Update",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal6,
};
/* Definitions for OPS_Update */
osThreadId_t OPS_UpdateHandle;
const osThreadAttr_t OPS_Update_attributes = {
  .name = "OPS_Update",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime4,
};
/* Definitions for Servo_Tx */
osThreadId_t Servo_TxHandle;
const osThreadAttr_t Servo_Tx_attributes = {
  .name = "Servo_Tx",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal4,
};
/* Definitions for Battery_Get */
osThreadId_t Battery_GetHandle;
const osThreadAttr_t Battery_Get_attributes = {
  .name = "Battery_Get",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow2,
};
/* Definitions for Loc_Update */
osThreadId_t Loc_UpdateHandle;
const osThreadAttr_t Loc_Update_attributes = {
  .name = "Loc_Update",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime3,
};
/* Definitions for Nav */
osThreadId_t NavHandle;
const osThreadAttr_t Nav_attributes = {
  .name = "Nav",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime3,
};
/* Definitions for Mission */
osThreadId_t MissionHandle;
const osThreadAttr_t Mission_attributes = {
  .name = "Mission",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityRealtime2,
};
/* Definitions for Scan_Get */
osThreadId_t Scan_GetHandle;
const osThreadAttr_t Scan_Get_attributes = {
  .name = "Scan_Get",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};
/* Definitions for Online_Check */
osThreadId_t Online_CheckHandle;
const osThreadAttr_t Online_Check_attributes = {
  .name = "Online_Check",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow3,
};
/* Definitions for Dist_Get */
osThreadId_t Dist_GetHandle;
const osThreadAttr_t Dist_Get_attributes = {
  .name = "Dist_Get",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal1,
};
/* Definitions for Oled_Refresh */
osThreadId_t Oled_RefreshHandle;
const osThreadAttr_t Oled_Refresh_attributes = {
  .name = "Oled_Refresh",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal4,
};
/* Definitions for Media_Player */
osThreadId_t Media_PlayerHandle;
const osThreadAttr_t Media_Player_attributes = {
  .name = "Media_Player",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal6,
};
/* Definitions for Nav_Track */
osThreadId_t Nav_TrackHandle;
const osThreadAttr_t Nav_Track_attributes = {
  .name = "Nav_Track",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityRealtime3,
};
/* Definitions for Turntable_Port */
osThreadId_t Turntable_PortHandle;
const osThreadAttr_t Turntable_Port_attributes = {
  .name = "Turntable_Port",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal5,
};
/* Definitions for Usart1_Rx_Data */
osMessageQueueId_t Usart1_Rx_DataHandle;
const osMessageQueueAttr_t Usart1_Rx_Data_attributes = {
  .name = "Usart1_Rx_Data"
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
/* Definitions for Uart4_Rx_Data */
osMessageQueueId_t Uart4_Rx_DataHandle;
const osMessageQueueAttr_t Uart4_Rx_Data_attributes = {
  .name = "Uart4_Rx_Data"
};
/* Definitions for Servo_Tx_Data */
osMessageQueueId_t Servo_Tx_DataHandle;
const osMessageQueueAttr_t Servo_Tx_Data_attributes = {
  .name = "Servo_Tx_Data"
};
/* Definitions for Media_Data */
osMessageQueueId_t Media_DataHandle;
const osMessageQueueAttr_t Media_Data_attributes = {
  .name = "Media_Data"
};
/* Definitions for Pose_Mutex */
osMutexId_t Pose_MutexHandle;
const osMutexAttr_t Pose_Mutex_attributes = {
  .name = "Pose_Mutex"
};
/* Definitions for Track_Mutex */
osMutexId_t Track_MutexHandle;
const osMutexAttr_t Track_Mutex_attributes = {
  .name = "Track_Mutex"
};
/* Definitions for Motor_Mutex */
osMutexId_t Motor_MutexHandle;
const osMutexAttr_t Motor_Mutex_attributes = {
  .name = "Motor_Mutex"
};
/* Definitions for OPS_Mutex */
osMutexId_t OPS_MutexHandle;
const osMutexAttr_t OPS_Mutex_attributes = {
  .name = "OPS_Mutex"
};
/* Definitions for OLED_Mutex */
osMutexId_t OLED_MutexHandle;
const osMutexAttr_t OLED_Mutex_attributes = {
  .name = "OLED_Mutex"
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
extern void Motor_Receive_Task(void *argument);
extern void Motor_Ctrl_Task(void *argument);
extern void Motor_Update_Task(void *argument);
extern void OPS_Update_Task(void *argument);
extern void Servo_Tx_Task(void *argument);
extern void Battery_Get_Task(void *argument);
extern void Loc_Update_Task(void *argument);
extern void Nav_Task(void *argument);
extern void mission_run(void *argument);
extern void Scan_Get_Task(void *argument);
extern void Online_Check_Task(void *argument);
extern void Dist_Get_Task(void *argument);
extern void Oled_Refresh_Task(void *argument);
extern void Media_Player_Task(void *argument);
extern void Nav_Track_Task(void *argument);
extern void Turntable_Port_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);
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

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */


#if USE_WG
  extern IWDG_HandleTypeDef hiwdg;
  HAL_IWDG_Refresh(&hiwdg);
#endif

}
/* USER CODE END 3 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
   my_printf("\r\n[ERROR] Stack overflow detected!\r\n");

   my_printf("Task name: %s\r\n", pcTaskName);
   my_printf("Task ID: 0x%p\r\n", xTask);

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
  /* Create the mutex(es) */
  /* creation of Pose_Mutex */
  Pose_MutexHandle = osMutexNew(&Pose_Mutex_attributes);

  /* creation of Track_Mutex */
  Track_MutexHandle = osMutexNew(&Track_Mutex_attributes);

  /* creation of Motor_Mutex */
  Motor_MutexHandle = osMutexNew(&Motor_Mutex_attributes);

  /* creation of OPS_Mutex */
  OPS_MutexHandle = osMutexNew(&OPS_Mutex_attributes);

  /* creation of OLED_Mutex */
  OLED_MutexHandle = osMutexNew(&OLED_Mutex_attributes);

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

  /* creation of MotorCmds */
  MotorCmdsHandle = osMessageQueueNew (6, sizeof(MotorCmd_t), &MotorCmds_attributes);

  /* creation of Usart6_Rx_Data */
  Usart6_Rx_DataHandle = osMessageQueueNew (4, sizeof(Usart6_RxBuf_t), &Usart6_Rx_Data_attributes);

  /* creation of Uart4_Rx_Data */
  Uart4_Rx_DataHandle = osMessageQueueNew (2, sizeof(Uart4_RxBuf_t), &Uart4_Rx_Data_attributes);

  /* creation of Servo_Tx_Data */
  Servo_Tx_DataHandle = osMessageQueueNew (4, sizeof(Package_t), &Servo_Tx_Data_attributes);

  /* creation of Media_Data */
  Media_DataHandle = osMessageQueueNew (1, sizeof(Media_t), &Media_Data_attributes);

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

  /* creation of Motor_Receive */
  Motor_ReceiveHandle = osThreadNew(Motor_Receive_Task, NULL, &Motor_Receive_attributes);

  /* creation of Motor_Ctrl */
  Motor_CtrlHandle = osThreadNew(Motor_Ctrl_Task, NULL, &Motor_Ctrl_attributes);

  /* creation of Motor_Update */
  Motor_UpdateHandle = osThreadNew(Motor_Update_Task, NULL, &Motor_Update_attributes);

  /* creation of OPS_Update */
  OPS_UpdateHandle = osThreadNew(OPS_Update_Task, NULL, &OPS_Update_attributes);

  /* creation of Servo_Tx */
  Servo_TxHandle = osThreadNew(Servo_Tx_Task, NULL, &Servo_Tx_attributes);

  /* creation of Battery_Get */
  Battery_GetHandle = osThreadNew(Battery_Get_Task, NULL, &Battery_Get_attributes);

  /* creation of Loc_Update */
  Loc_UpdateHandle = osThreadNew(Loc_Update_Task, NULL, &Loc_Update_attributes);

  /* creation of Nav */
  NavHandle = osThreadNew(Nav_Task, NULL, &Nav_attributes);

  /* creation of Mission */
  MissionHandle = osThreadNew(mission_run, NULL, &Mission_attributes);

  /* creation of Scan_Get */
  Scan_GetHandle = osThreadNew(Scan_Get_Task, NULL, &Scan_Get_attributes);

  /* creation of Online_Check */
  Online_CheckHandle = osThreadNew(Online_Check_Task, NULL, &Online_Check_attributes);

  /* creation of Dist_Get */
  Dist_GetHandle = osThreadNew(Dist_Get_Task, NULL, &Dist_Get_attributes);

  /* creation of Oled_Refresh */
  Oled_RefreshHandle = osThreadNew(Oled_Refresh_Task, NULL, &Oled_Refresh_attributes);

  /* creation of Media_Player */
  Media_PlayerHandle = osThreadNew(Media_Player_Task, NULL, &Media_Player_attributes);

  /* creation of Nav_Track */
  Nav_TrackHandle = osThreadNew(Nav_Track_Task, NULL, &Nav_Track_attributes);

  /* creation of Turntable_Port */
  Turntable_PortHandle = osThreadNew(Turntable_Port_Task, NULL, &Turntable_Port_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

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

#if USE_WG
  SHOW_DMESG(dmesg_wait, "Initialize Watch Dog");
  extern void MX_IWDG_Init(void);
  MX_IWDG_Init();
  SHOW_DMESG(dmesg_ok, NULL);
#else
  my_printf("[warn] Debug Mode, Watch Dog Disabled.\r\n");
#endif

  SHOW_DMESG(dmesg_wait, "Initialize Shell");
  extern void userShellInit(void);
  userShellInit();
  SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize shell log");
  extern void logInit(void);
  logInit();
  SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Servo Module");
  extern void Servo_Init(void);
  Servo_Init();
  SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Tracking Module");
  if (!Track_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize VL53L0X Module");
  extern bool VL53L0X_Init(void);
  if (!VL53L0X_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize OPS Module");
  if (!OPS_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Battery Module");
  extern bool Battery_Init(void);
  if (!Battery_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Scan Module");
  if (!Scan_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Motor Module");
  extern bool Motor_Init(void);
  if (!Motor_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else {
    SHOW_DMESG(dmesg_ok, NULL);
    SHOW_DMESG(dmesg_wait, "Initialize Chassis Module");
    extern bool Chassis_Init(void);
    if (!Chassis_Init()) SHOW_DMESG(dmesg_fail, NULL);
    else SHOW_DMESG(dmesg_ok, NULL);
  }

  SHOW_DMESG(dmesg_wait, "Initialize Localization Module");
  extern bool Loc_Init(void);
  if (!Loc_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Map Module");
  extern bool Map_Init(uint8_t max_point_num);
  if (!Map_Init(32)) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Nav Module");
  extern bool Nav_Core_Init(void);
  if (!Nav_Core_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Nav Track Module");
  extern bool Nav_Track_Init(void);
  if (!Nav_Track_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize OLED Module");
  extern void OLED_Init(void);
  OLED_Init();
  SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize Turntable Module");
  extern bool Turntable_Port_Init(void);
  if (!Turntable_Port_Init()) SHOW_DMESG(dmesg_fail, NULL);
  else SHOW_DMESG(dmesg_ok, NULL);

  SHOW_DMESG(dmesg_wait, "Initialize RPI Module");
  extern void RPI_Init(void);
  RPI_Init();
  SHOW_DMESG(dmesg_ok, NULL);



  extern Shell shell;
  Shell_New_Convo(&shell);
  osEventFlagsSet(System_StatusHandle, SYS_INIT_COMPLETE);
	vTaskDelete(NULL);
  /* USER CODE END Sys_Init_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
