/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
#include "usart.h"

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_uart4_rx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern DMA_HandleTypeDef hdma_usart6_tx;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;
extern TIM_HandleTypeDef htim1;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  my_printf("\r\n==================== HardFault Detected ====================\r\n");

  uint32_t *hardfault_args;
  uint32_t stacked_r0;
  uint32_t stacked_r1;
  uint32_t stacked_r2;
  uint32_t stacked_r3;
  uint32_t stacked_r12;
  uint32_t stacked_lr;
  uint32_t stacked_pc;
  uint32_t stacked_xpsr;

  hardfault_args = (uint32_t *)__get_MSP();

  stacked_r0  = hardfault_args[0];
  stacked_r1  = hardfault_args[1];
  stacked_r2  = hardfault_args[2];
  stacked_r3  = hardfault_args[3];
  stacked_r12 = hardfault_args[4];
  stacked_lr  = hardfault_args[5];
  stacked_pc  = hardfault_args[6];
  stacked_xpsr = hardfault_args[7];

  my_printf("R0  = 0x%08lX\r\n", stacked_r0);
  my_printf("R1  = 0x%08lX\r\n", stacked_r1);
  my_printf("R2  = 0x%08lX\r\n", stacked_r2);
  my_printf("R3  = 0x%08lX\r\n", stacked_r3);
  my_printf("R12 = 0x%08lX\r\n", stacked_r12);
  my_printf("LR  = 0x%08lX\r\n", stacked_lr);
  my_printf("PC  = 0x%08lX (Fault Address)\r\n", stacked_pc);
  my_printf("xPSR= 0x%08lX\r\n", stacked_xpsr);

  my_printf("\r\n--- Fault Status Registers ---\r\n");

  uint32_t cfsr = (*((volatile uint32_t *)(0xE000ED28)));
  uint32_t hfsr = (*((volatile uint32_t *)(0xE000ED2C)));
  uint32_t dfsr = (*((volatile uint32_t *)(0xE000ED30)));
  uint32_t afsr = (*((volatile uint32_t *)(0xE000ED3C)));
  uint32_t bfar = (*((volatile uint32_t *)(0xE000ED38)));
  uint32_t mmfar = (*((volatile uint32_t *)(0xE000ED34)));

  my_printf("CFSR = 0x%08lX\r\n", cfsr);
  my_printf("HFSR = 0x%08lX\r\n", hfsr);
  my_printf("DFSR = 0x%08lX\r\n", dfsr);
  my_printf("AFSR = 0x%08lX\r\n", afsr);
  my_printf("BFAR = 0x%08lX\r\n", bfar);
  my_printf("MMFAR= 0x%08lX\r\n", mmfar);

  my_printf("\r\n--- Detailed Fault Analysis ---\r\n");

  if (cfsr & 0x00800000) {
    my_printf("[MemManage] MemManage Fault occurred\r\n");
    if (cfsr & 0x00008000) {
      my_printf("  - MMARVALID: MMFAR (0x%08lX) holds a valid fault address\r\n", mmfar);
    }
    if (cfsr & 0x00000001) {
      my_printf("  - IACCVIOL: Instruction access violation\r\n");
    }
    if (cfsr & 0x00000002) {
      my_printf("  - DACCVIOL: Data access violation\r\n");
    }
    if (cfsr & 0x00000008) {
      my_printf("  - MUNSTKERR: Unstacking error\r\n");
    }
    if (cfsr & 0x00000010) {
      my_printf("  - MSTKERR: Stacking error\r\n");
    }
    if (cfsr & 0x00000020) {
      my_printf("  - MLSPERR: MemManage fault during FP lazy state preservation\r\n");
    }
  }

  if (cfsr & 0x00008000) {
    my_printf("[BusFault] Bus Fault occurred\r\n");
    if (cfsr & 0x00004000) {
      my_printf("  - BFARVALID: BFAR (0x%08lX) holds a valid fault address\r\n", bfar);
    }
    if (cfsr & 0x00000100) {
      my_printf("  - IBUSERR: Instruction bus error\r\n");
    }
    if (cfsr & 0x00000200) {
      my_printf("  - PRECISERR: Precise data bus error\r\n");
    }
    if (cfsr & 0x00000400) {
      my_printf("  - IMPRECISERR: Imprecise data bus error\r\n");
    }
    if (cfsr & 0x00000800) {
      my_printf("  - UNSTKERR: Unstacking error\r\n");
    }
    if (cfsr & 0x00001000) {
      my_printf("  - STKERR: Stacking error\r\n");
    }
    if (cfsr & 0x00002000) {
      my_printf("  - LSPERR: Bus fault during FP lazy state preservation\r\n");
    }
  }

  if (cfsr & 0x00000001) {
    my_printf("[UsageFault] Usage Fault occurred\r\n");
    if (cfsr & 0x00000001) {
      my_printf("  - UNDEFINSTR: Undefined instruction\r\n");
    }
    if (cfsr & 0x00000002) {
      my_printf("  - INVSTATE: Invalid state\r\n");
    }
    if (cfsr & 0x00000004) {
      my_printf("  - INVPC: Invalid PC load\r\n");
    }
    if (cfsr & 0x00000008) {
      my_printf("  - NOCP: No coprocessor\r\n");
    }
    if (cfsr & 0x00000010) {
      my_printf("  - UNALIGNED: Unaligned access\r\n");
    }
    if (cfsr & 0x00000020) {
      my_printf("  - DIVBYZERO: Divide by zero\r\n");
    }
  }

  if (hfsr & 0x40000000) {
    my_printf("[HardFault] FORCED: Escalated from configurable fault\r\n");
  }
  if (hfsr & 0x80000000) {
    my_printf("[HardFault] DEBUGEVT: Debug event\r\n");
  }

  if (stacked_xpsr & 0x000001FF) {
    my_printf("\r\n--- xPSR Analysis ---\r\n");
    if (stacked_xpsr & 0x00000080) {
      my_printf("  - C: Carry flag set\r\n");
    }
    if (stacked_xpsr & 0x00000040) {
      my_printf("  - Z: Zero flag set\r\n");
    }
    if (stacked_xpsr & 0x00000020) {
      my_printf("  - N: Negative flag set\r\n");
    }
    if (stacked_xpsr & 0x00000010) {
      my_printf("  - V: Overflow flag set\r\n");
    }
    if (stacked_xpsr & 0x00000008) {
      my_printf("  - Q: Saturation flag set\r\n");
    }
    my_printf("  - Thumb bit: %s\r\n", (stacked_xpsr & 0x01000000) ? "Set (Thumb mode)" : "Clear");
  }

  my_printf("\r\n[ERROR] System halted due to HardFault\r\n");
  my_printf("=============================================================\r\n");

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  my_printf("\r\n==================== MemManage Detected ====================\r\n");

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  my_printf("\r\n==================== BusFault Detected ====================\r\n");

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  my_printf("\r\n==================== UsageFault Detected ====================\r\n");

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  my_printf("\r\n==================== DebugMonitor Detected ====================\r\n");

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 stream1 global interrupt.
  */
void DMA1_Stream1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream1_IRQn 0 */

  /* USER CODE END DMA1_Stream1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_rx);
  /* USER CODE BEGIN DMA1_Stream1_IRQn 1 */

  /* USER CODE END DMA1_Stream1_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream2 global interrupt.
  */
void DMA1_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream2_IRQn 0 */

  /* USER CODE END DMA1_Stream2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_uart4_rx);
  /* USER CODE BEGIN DMA1_Stream2_IRQn 1 */

  /* USER CODE END DMA1_Stream2_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream3 global interrupt.
  */
void DMA1_Stream3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream3_IRQn 0 */

  /* USER CODE END DMA1_Stream3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_tx);
  /* USER CODE BEGIN DMA1_Stream3_IRQn 1 */

  /* USER CODE END DMA1_Stream3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream5 global interrupt.
  */
void DMA1_Stream5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */

  /* USER CODE END DMA1_Stream5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart2_rx);
  /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */

  /* USER CODE END DMA1_Stream5_IRQn 1 */
}

/**
  * @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
  */
void TIM1_UP_TIM10_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 0 */

  /* USER CODE END TIM1_UP_TIM10_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_TIM10_IRQn 1 */

  /* USER CODE END TIM1_UP_TIM10_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */

  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/**
  * @brief This function handles USART3 global interrupt.
  */
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */

  /* USER CODE END USART3_IRQn 0 */
  HAL_UART_IRQHandler(&huart3);
  /* USER CODE BEGIN USART3_IRQn 1 */

  /* USER CODE END USART3_IRQn 1 */
}

/**
  * @brief This function handles UART4 global interrupt.
  */
void UART4_IRQHandler(void)
{
  /* USER CODE BEGIN UART4_IRQn 0 */

  /* USER CODE END UART4_IRQn 0 */
  HAL_UART_IRQHandler(&huart4);
  /* USER CODE BEGIN UART4_IRQn 1 */

  /* USER CODE END UART4_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream1 global interrupt.
  */
void DMA2_Stream1_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream1_IRQn 0 */

  /* USER CODE END DMA2_Stream1_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart6_rx);
  /* USER CODE BEGIN DMA2_Stream1_IRQn 1 */

  /* USER CODE END DMA2_Stream1_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream2 global interrupt.
  */
void DMA2_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream2_IRQn 0 */

  /* USER CODE END DMA2_Stream2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
  /* USER CODE BEGIN DMA2_Stream2_IRQn 1 */

  /* USER CODE END DMA2_Stream2_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream6 global interrupt.
  */
void DMA2_Stream6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream6_IRQn 0 */

  /* USER CODE END DMA2_Stream6_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart6_tx);
  /* USER CODE BEGIN DMA2_Stream6_IRQn 1 */

  /* USER CODE END DMA2_Stream6_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream7 global interrupt.
  */
void DMA2_Stream7_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream7_IRQn 0 */

  /* USER CODE END DMA2_Stream7_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart1_tx);
  /* USER CODE BEGIN DMA2_Stream7_IRQn 1 */

  /* USER CODE END DMA2_Stream7_IRQn 1 */
}

/**
  * @brief This function handles USART6 global interrupt.
  */
void USART6_IRQHandler(void)
{
  /* USER CODE BEGIN USART6_IRQn 0 */

  /* USER CODE END USART6_IRQn 0 */
  HAL_UART_IRQHandler(&huart6);
  /* USER CODE BEGIN USART6_IRQn 1 */

  /* USER CODE END USART6_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
