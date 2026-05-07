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
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  
  my_printf("\r\n");
  my_printf("============================================================\r\n");
  my_printf("  [CRITICAL] NMI Handler Triggered\r\n");
  my_printf("============================================================\r\n\r\n");

  uint32_t *nmi_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;

  nmi_args = (uint32_t *)__get_MSP();

  stacked_r0  = nmi_args[0];
  stacked_r1  = nmi_args[1];
  stacked_r2  = nmi_args[2];
  stacked_r3  = nmi_args[3];
  stacked_r12 = nmi_args[4];
  stacked_lr  = nmi_args[5];
  stacked_pc  = nmi_args[6];
  stacked_xpsr = nmi_args[7];

  my_printf("--- Stacked Registers ----------------------------------------\r\n");
  my_printf("  R0  = 0x%08lX  |  R1  = 0x%08lX\r\n", stacked_r0, stacked_r1);
  my_printf("  R2  = 0x%08lX  |  R3  = 0x%08lX\r\n", stacked_r2, stacked_r3);
  my_printf("  R12 = 0x%08lX\r\n", stacked_r12);
  my_printf("  LR  = 0x%08lX\r\n", stacked_lr);
  my_printf("  PC  = 0x%08lX  |  xPSR = 0x%08lX\r\n", stacked_pc, stacked_xpsr);

  my_printf("\r\n--- NMI Configuration Registers -------------------------------\r\n");
  uint32_t nvic_icsr = (*((volatile uint32_t *)(0xE000ED04)));
  uint32_t nvic_adr  = (*((volatile uint32_t *)(0xE000ED0C)));
  uint32_t nvic_shp12 = (*((volatile uint32_t *)(0xE000ED90)));
  uint32_t nvic_shp13 = (*((volatile uint32_t *)(0xE000ED94)));
  uint32_t rcc_csr    = (*((volatile uint32_t *)(0x40023824)));
  
  my_printf("  NVIC_ICSR  = 0x%08lX\r\n", nvic_icsr);
  my_printf("  NVIC_ADR   = 0x%08lX\r\n", nvic_adr);
  my_printf("  NVIC_SHp12 = 0x%08lX\r\n", nvic_shp12);
  my_printf("  NVIC_SHp13 = 0x%08lX\r\n", nvic_shp13);
  my_printf("  RCC_CSR    = 0x%08lX\r\n", rcc_csr);

  my_printf("\r\n--- NMI Source Analysis --------------------------------------\r\n");
  if (nvic_icsr & 0x04000000) {
    my_printf("  [NMI] NMI bit set in NVIC_ICSR\r\n");
  }
  if (rcc_csr & 0x00000001) {
    my_printf("  [NMI] PLL Ready Interrupt Flag\r\n");
  }
  if (rcc_csr & 0x00000002) {
    my_printf("  [NMI] CSS Flag - Clock Security System triggered\r\n");
  }
  if (rcc_csr & 0x10000000) {
    my_printf("  [NMI] LSIRDY - Low Speed Internal RC Oscillator ready\r\n");
  }
  if (rcc_csr & 0x20000000) {
    my_printf("  [NMI] LSERDY - Low Speed External Crystal ready\r\n");
  }
  if (rcc_csr & 0x40000000) {
    my_printf("  [NMI] HSIRDY - High Speed Internal RC Oscillator ready\r\n");
  }
  if (rcc_csr & 0x80000000) {
    my_printf("  [NMI] HSERDY - High Speed External Crystal ready\r\n");
  }

  my_printf("\r\n--- xPSR Analysis -------------------------------------------\r\n");
  if (stacked_xpsr & 0x00000080) {
    my_printf("  [xPSR] C: Carry flag set\r\n");
  }
  if (stacked_xpsr & 0x00000040) {
    my_printf("  [xPSR] Z: Zero flag set\r\n");
  }
  if (stacked_xpsr & 0x00000020) {
    my_printf("  [xPSR] N: Negative flag set\r\n");
  }
  if (stacked_xpsr & 0x00000010) {
    my_printf("  [xPSR] V: Overflow flag set\r\n");
  }
  my_printf("  [xPSR] Thumb bit: %s\r\n", 
           (stacked_xpsr & 0x01000000) ? "Set (Thumb mode)" : "Clear");
  my_printf("  [xPSR] Exception Number: %lu\r\n", 
           (stacked_xpsr & 0x000001FF));

  my_printf("\r\n============================================================\r\n");
  my_printf("  [FATAL] System halted due to NMI exception\r\n");
  my_printf("============================================================\r\n");

  /* USER CODE END NonMaskableInt_IRQn 0 */
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
  
  my_printf("\r\n");
  my_printf("============================================================\r\n");
  my_printf("  [CRITICAL] HardFault Handler Triggered\r\n");
  my_printf("============================================================\r\n\r\n");

  uint32_t *hardfault_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;

  hardfault_args = (uint32_t *)__get_MSP();

  stacked_r0  = hardfault_args[0];
  stacked_r1  = hardfault_args[1];
  stacked_r2  = hardfault_args[2];
  stacked_r3  = hardfault_args[3];
  stacked_r12 = hardfault_args[4];
  stacked_lr  = hardfault_args[5];
  stacked_pc  = hardfault_args[6];
  stacked_xpsr = hardfault_args[7];

  my_printf("--- Stacked Registers ----------------------------------------\r\n");
  my_printf("  R0  = 0x%08lX  |  R1  = 0x%08lX\r\n", stacked_r0, stacked_r1);
  my_printf("  R2  = 0x%08lX  |  R3  = 0x%08lX\r\n", stacked_r2, stacked_r3);
  my_printf("  R12 = 0x%08lX\r\n", stacked_r12);
  my_printf("  LR  = 0x%08lX\r\n", stacked_lr);
  my_printf("  PC  = 0x%08lX  |  xPSR = 0x%08lX\r\n", stacked_pc, stacked_xpsr);

  my_printf("\r\n--- Fault Status Registers -----------------------------------\r\n");
  uint32_t cfsr  = (*((volatile uint32_t *)(0xE000ED28)));
  uint32_t hfsr  = (*((volatile uint32_t *)(0xE000ED2C)));
  uint32_t dfsr  = (*((volatile uint32_t *)(0xE000ED30)));
  uint32_t afsr  = (*((volatile uint32_t *)(0xE000ED3C)));
  uint32_t bfar  = (*((volatile uint32_t *)(0xE000ED38)));
  uint32_t mmfar = (*((volatile uint32_t *)(0xE000ED34)));

  my_printf("  CFSR  = 0x%08lX\r\n", cfsr);
  my_printf("  HFSR  = 0x%08lX\r\n", hfsr);
  my_printf("  DFSR  = 0x%08lX\r\n", dfsr);
  my_printf("  AFSR  = 0x%08lX\r\n", afsr);
  my_printf("  BFAR  = 0x%08lX\r\n", bfar);
  my_printf("  MMFAR = 0x%08lX\r\n", mmfar);

  my_printf("\r\n--- Fault Analysis --------------------------------------------\r\n");
  
  if (cfsr & 0x00800000) {
    my_printf("  [MemManage] MemManage Fault also occurred\r\n");
    if (cfsr & 0x00008000) {
      my_printf("    -> MMARVALID: MMFAR (0x%08lX) holds valid fault address\r\n", mmfar);
    }
    if (cfsr & 0x00000001) {
      my_printf("    -> IACCVIOL: Instruction access violation\r\n");
    }
    if (cfsr & 0x00000002) {
      my_printf("    -> DACCVIOL: Data access violation\r\n");
    }
    if (cfsr & 0x00000008) {
      my_printf("    -> MUNSTKERR: Unstacking error\r\n");
    }
    if (cfsr & 0x00000010) {
      my_printf("    -> MSTKERR: Stacking error\r\n");
    }
    if (cfsr & 0x00000020) {
      my_printf("    -> MLSPERR: FP lazy state preservation error\r\n");
    }
  }

  if (cfsr & 0x00008000) {
    my_printf("  [BusFault] Bus Fault also occurred\r\n");
    if (cfsr & 0x00004000) {
      my_printf("    -> BFARVALID: BFAR (0x%08lX) holds valid fault address\r\n", bfar);
    }
    if (cfsr & 0x00000100) {
      my_printf("    -> IBUSERR: Instruction bus error\r\n");
    }
    if (cfsr & 0x00000200) {
      my_printf("    -> PRECISERR: Precise data bus error\r\n");
    }
    if (cfsr & 0x00000400) {
      my_printf("    -> IMPRECISERR: Imprecise data bus error\r\n");
    }
    if (cfsr & 0x00000800) {
      my_printf("    -> UNSTKERR: Unstacking error\r\n");
    }
    if (cfsr & 0x00001000) {
      my_printf("    -> STKERR: Stacking error\r\n");
    }
    if (cfsr & 0x00002000) {
      my_printf("    -> LSPERR: FP lazy state preservation error\r\n");
    }
  }

  if (cfsr & 0x00010000) {
    my_printf("  [UsageFault] Usage Fault also occurred\r\n");
    if (cfsr & 0x00010000) {
      my_printf("    -> UNDEFINSTR: Undefined instruction\r\n");
    }
    if (cfsr & 0x00020000) {
      my_printf("    -> INVSTATE: Invalid state\r\n");
    }
    if (cfsr & 0x00040000) {
      my_printf("    -> INVPC: Invalid PC load\r\n");
    }
    if (cfsr & 0x00080000) {
      my_printf("    -> NOCP: No coprocessor\r\n");
    }
    if (cfsr & 0x00100000) {
      my_printf("    -> UNALIGNED: Unaligned access\r\n");
    }
    if (cfsr & 0x00200000) {
      my_printf("    -> DIVBYZERO: Divide by zero\r\n");
    }
  }

  if (hfsr & 0x40000000) {
    my_printf("  [HardFault] FORCED: Escalated from configurable fault\r\n");
  }
  if (hfsr & 0x80000000) {
    my_printf("  [HardFault] DEBUGEVT: Debug event\r\n");
  }

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n");
  my_printf("  Program Counter (PC) = 0x%08lX\r\n", stacked_pc);
  my_printf("  Link Register (LR)   = 0x%08lX\r\n", stacked_lr);
  my_printf("  Stack Pointer (MSP)  = 0x%08lX\r\n", (uint32_t)hardfault_args);
  if (bfar != 0) {
    my_printf("  Bus Fault Address    = 0x%08lX\r\n", bfar);
  }
  if (mmfar != 0) {
    my_printf("  MemManage Address    = 0x%08lX\r\n", mmfar);
  }

  my_printf("\r\n--- xPSR Analysis -------------------------------------------\r\n");
  if (stacked_xpsr & 0x00000080) {
    my_printf("  [xPSR] C: Carry flag set\r\n");
  }
  if (stacked_xpsr & 0x00000040) {
    my_printf("  [xPSR] Z: Zero flag set\r\n");
  }
  if (stacked_xpsr & 0x00000020) {
    my_printf("  [xPSR] N: Negative flag set\r\n");
  }
  if (stacked_xpsr & 0x00000010) {
    my_printf("  [xPSR] V: Overflow flag set\r\n");
  }
  if (stacked_xpsr & 0x00000008) {
    my_printf("  [xPSR] Q: Saturation flag set\r\n");
  }
  my_printf("  [xPSR] Thumb bit: %s\r\n", 
           (stacked_xpsr & 0x01000000) ? "Set (Thumb mode)" : "Clear");
  my_printf("  [xPSR] Exception Number: %lu\r\n", 
           (stacked_xpsr & 0x000001FF));

  my_printf("\r\n============================================================\r\n");
  my_printf("  [FATAL] System halted due to HardFault exception\r\n");
  my_printf("============================================================\r\n");

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  
  my_printf("\r\n");
  my_printf("============================================================\r\n");
  my_printf("  [CRITICAL] MemManage Handler Triggered\r\n");
  my_printf("============================================================\r\n\r\n");

  uint32_t *mm_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;

  mm_args = (uint32_t *)__get_MSP();

  stacked_r0  = mm_args[0];
  stacked_r1  = mm_args[1];
  stacked_r2  = mm_args[2];
  stacked_r3  = mm_args[3];
  stacked_r12 = mm_args[4];
  stacked_lr  = mm_args[5];
  stacked_pc  = mm_args[6];
  stacked_xpsr = mm_args[7];

  my_printf("--- Stacked Registers ----------------------------------------\r\n");
  my_printf("  R0  = 0x%08lX  |  R1  = 0x%08lX\r\n", stacked_r0, stacked_r1);
  my_printf("  R2  = 0x%08lX  |  R3  = 0x%08lX\r\n", stacked_r2, stacked_r3);
  my_printf("  R12 = 0x%08lX\r\n", stacked_r12);
  my_printf("  LR  = 0x%08lX\r\n", stacked_lr);
  my_printf("  PC  = 0x%08lX  |  xPSR = 0x%08lX\r\n", stacked_pc, stacked_xpsr);

  my_printf("\r\n--- MemManage Fault Registers -------------------------------\r\n");
  uint32_t cfsr  = (*((volatile uint32_t *)(0xE000ED28)));
  uint32_t hfsr  = (*((volatile uint32_t *)(0xE000ED2C)));
  uint32_t dfsr  = (*((volatile uint32_t *)(0xE000ED30)));
  uint32_t afsr  = (*((volatile uint32_t *)(0xE000ED3C)));
  uint32_t mmfar = (*((volatile uint32_t *)(0xE000ED34)));

  my_printf("  CFSR  = 0x%08lX\r\n", cfsr);
  my_printf("  HFSR  = 0x%08lX\r\n", hfsr);
  my_printf("  DFSR  = 0x%08lX\r\n", dfsr);
  my_printf("  AFSR  = 0x%08lX\r\n", afsr);
  my_printf("  MMFAR = 0x%08lX\r\n", mmfar);

  my_printf("\r\n--- MemManage Fault Analysis --------------------------------\r\n");
  if (cfsr & 0x00008000) {
    my_printf("  [MemManage] Bus Fault also occurred\r\n");
  }
  if (cfsr & 0x00000001) {
    my_printf("  [MemManage] IACCVIOL: Instruction access violation\r\n");
    my_printf("    -> Attempted to execute code from a non-executable region\r\n");
  }
  if (cfsr & 0x00000002) {
    my_printf("  [MemManage] DACCVIOL: Data access violation\r\n");
    my_printf("    -> Fault address (MMFAR): 0x%08lX\r\n", mmfar);
  }
  if (cfsr & 0x00000004) {
    my_printf("  [MemManage] MUNSTKERR: Unstacking error\r\n");
    my_printf("    -> Failed to restore stack during exception return\r\n");
  }
  if (cfsr & 0x00000008) {
    my_printf("  [MemManage] MSTKERR: Stacking error\r\n");
    my_printf("    -> Failed to save context during exception entry\r\n");
  }
  if (cfsr & 0x00000010) {
    my_printf("  [MemManage] MLSPERR: FP lazy state preservation error\r\n");
    my_printf("    -> Error during FP lazy stacking\r\n");
  }
  if (cfsr & 0x00000020) {
    my_printf("  [MemManage] MMARVALID: MMFAR contains valid fault address\r\n");
    my_printf("    -> Valid fault address: 0x%08lX\r\n", mmfar);
  }
  if (cfsr & 0x00000040) {
    my_printf("  [MemManage] MCLSRERR: MPU or fault with FPU lazy stacking\r\n");
  }

  my_printf("\r\n--- Fault Context Information -------------------------------\r\n");
  my_printf("  Fault Address (MMFAR) = 0x%08lX\r\n", mmfar);
  my_printf("  Program Counter (PC) = 0x%08lX\r\n", stacked_pc);
  my_printf("  Link Register (LR)   = 0x%08lX\r\n", stacked_lr);
  
  if (stacked_pc != 0) {
    my_printf("  Instruction at fault: 0x%08lX\r\n", stacked_pc);
  }

  my_printf("\r\n--- xPSR Analysis -------------------------------------------\r\n");
  if (stacked_xpsr & 0x00000080) {
    my_printf("  [xPSR] C: Carry flag set\r\n");
  }
  if (stacked_xpsr & 0x00000040) {
    my_printf("  [xPSR] Z: Zero flag set\r\n");
  }
  if (stacked_xpsr & 0x00000020) {
    my_printf("  [xPSR] N: Negative flag set\r\n");
  }
  if (stacked_xpsr & 0x00000010) {
    my_printf("  [xPSR] V: Overflow flag set\r\n");
  }
  my_printf("  [xPSR] Thumb bit: %s\r\n", 
           (stacked_xpsr & 0x01000000) ? "Set (Thumb mode)" : "Clear");
  my_printf("  [xPSR] Exception Number: %lu\r\n", 
           (stacked_xpsr & 0x000001FF));

  my_printf("\r\n============================================================\r\n");
  my_printf("  [FATAL] System halted due to MemManage exception\r\n");
  my_printf("============================================================\r\n");

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  
  my_printf("\r\n");
  my_printf("============================================================\r\n");
  my_printf("  [CRITICAL] BusFault Handler Triggered\r\n");
  my_printf("============================================================\r\n\r\n");

  uint32_t *bus_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;

  bus_args = (uint32_t *)__get_MSP();

  stacked_r0  = bus_args[0];
  stacked_r1  = bus_args[1];
  stacked_r2  = bus_args[2];
  stacked_r3  = bus_args[3];
  stacked_r12 = bus_args[4];
  stacked_lr  = bus_args[5];
  stacked_pc  = bus_args[6];
  stacked_xpsr = bus_args[7];

  my_printf("--- Stacked Registers ----------------------------------------\r\n");
  my_printf("  R0  = 0x%08lX  |  R1  = 0x%08lX\r\n", stacked_r0, stacked_r1);
  my_printf("  R2  = 0x%08lX  |  R3  = 0x%08lX\r\n", stacked_r2, stacked_r3);
  my_printf("  R12 = 0x%08lX\r\n", stacked_r12);
  my_printf("  LR  = 0x%08lX\r\n", stacked_lr);
  my_printf("  PC  = 0x%08lX  |  xPSR = 0x%08lX\r\n", stacked_pc, stacked_xpsr);

  my_printf("\r\n--- BusFault Registers ----------------------------------------\r\n");
  uint32_t cfsr  = (*((volatile uint32_t *)(0xE000ED28)));
  uint32_t hfsr  = (*((volatile uint32_t *)(0xE000ED2C)));
  uint32_t dfsr  = (*((volatile uint32_t *)(0xE000ED30)));
  uint32_t afsr  = (*((volatile uint32_t *)(0xE000ED3C)));
  uint32_t bfar  = (*((volatile uint32_t *)(0xE000ED38)));

  my_printf("  CFSR = 0x%08lX\r\n", cfsr);
  my_printf("  HFSR = 0x%08lX\r\n", hfsr);
  my_printf("  DFSR = 0x%08lX\r\n", dfsr);
  my_printf("  AFSR = 0x%08lX\r\n", afsr);
  my_printf("  BFAR = 0x%08lX\r\n", bfar);

  my_printf("\r\n--- BusFault Analysis ------------------------------------------\r\n");
  if (cfsr & 0x00800000) {
    my_printf("  [BusFault] MemManage Fault also occurred\r\n");
  }
  if (cfsr & 0x00004000) {
    my_printf("  [BusFault] BFARVALID: BFAR holds valid fault address\r\n");
    my_printf("    -> Bus fault address (BFAR): 0x%08lX\r\n", bfar);
  }
  if (cfsr & 0x00000100) {
    my_printf("  [BusFault] IBUSERR: Instruction bus error\r\n");
    my_printf("    -> Bus error during instruction fetch\r\n");
    my_printf("    -> Faulting address: 0x%08lX\r\n", stacked_pc);
  }
  if (cfsr & 0x00000200) {
    my_printf("  [BusFault] PRECISERR: Precise data bus error\r\n");
    my_printf("    -> Precise bus fault occurred\r\n");
    my_printf("    -> Faulting address (BFAR): 0x%08lX\r\n", bfar);
  }
  if (cfsr & 0x00000400) {
    my_printf("  [BusFault] IMPRECISERR: Imprecise data bus error\r\n");
    my_printf("    -> Imprecise bus fault (address may not be accurate)\r\n");
  }
  if (cfsr & 0x00000800) {
    my_printf("  [BusFault] UNSTKERR: Unstacking error\r\n");
    my_printf("    -> Failed to restore bus context on exception return\r\n");
  }
  if (cfsr & 0x00001000) {
    my_printf("  [BusFault] STKERR: Stacking error\r\n");
    my_printf("    -> Failed to save bus context on exception entry\r\n");
  }
  if (cfsr & 0x00002000) {
    my_printf("  [BusFault] LSPERR: FP lazy state preservation error\r\n");
    my_printf("    -> Bus fault during FP lazy stacking\r\n");
  }

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n");
  my_printf("  Bus Fault Address (BFAR) = 0x%08lX\r\n", bfar);
  my_printf("  Program Counter (PC)    = 0x%08lX\r\n", stacked_pc);
  my_printf("  Link Register (LR)      = 0x%08lX\r\n", stacked_lr);
  
  if (stacked_r0 != 0) {
    my_printf("  Memory access address   = 0x%08lX\r\n", stacked_r0);
  }

  my_printf("\r\n--- Possible Causes -------------------------------------------\r\n");
  my_printf("  1. Invalid memory access to peripheral register\r\n");
  my_printf("  2. Attempted to access non-existent memory region\r\n");
  my_printf("  3. Bus timeout or peripheral error\r\n");
  my_printf("  4. Flash memory read error (ECC failure)\r\n");

  my_printf("\r\n--- xPSR Analysis -------------------------------------------\r\n");
  if (stacked_xpsr & 0x00000080) {
    my_printf("  [xPSR] C: Carry flag set\r\n");
  }
  if (stacked_xpsr & 0x00000040) {
    my_printf("  [xPSR] Z: Zero flag set\r\n");
  }
  if (stacked_xpsr & 0x00000020) {
    my_printf("  [xPSR] N: Negative flag set\r\n");
  }
  if (stacked_xpsr & 0x00000010) {
    my_printf("  [xPSR] V: Overflow flag set\r\n");
  }
  my_printf("  [xPSR] Thumb bit: %s\r\n", 
           (stacked_xpsr & 0x01000000) ? "Set (Thumb mode)" : "Clear");
  my_printf("  [xPSR] Exception Number: %lu\r\n", 
           (stacked_xpsr & 0x000001FF));

  my_printf("\r\n============================================================\r\n");
  my_printf("  [FATAL] System halted due to BusFault exception\r\n");
  my_printf("============================================================\r\n");

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  
  my_printf("\r\n");
  my_printf("============================================================\r\n");
  my_printf("  [CRITICAL] UsageFault Handler Triggered\r\n");
  my_printf("============================================================\r\n\r\n");

  uint32_t *usage_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;

  usage_args = (uint32_t *)__get_MSP();

  stacked_r0  = usage_args[0];
  stacked_r1  = usage_args[1];
  stacked_r2  = usage_args[2];
  stacked_r3  = usage_args[3];
  stacked_r12 = usage_args[4];
  stacked_lr  = usage_args[5];
  stacked_pc  = usage_args[6];
  stacked_xpsr = usage_args[7];

  my_printf("--- Stacked Registers ----------------------------------------\r\n");
  my_printf("  R0  = 0x%08lX  |  R1  = 0x%08lX\r\n", stacked_r0, stacked_r1);
  my_printf("  R2  = 0x%08lX  |  R3  = 0x%08lX\r\n", stacked_r2, stacked_r3);
  my_printf("  R12 = 0x%08lX\r\n", stacked_r12);
  my_printf("  LR  = 0x%08lX\r\n", stacked_lr);
  my_printf("  PC  = 0x%08lX  |  xPSR = 0x%08lX\r\n", stacked_pc, stacked_xpsr);

  my_printf("\r\n--- UsageFault Registers ---------------------------------------\r\n");
  uint32_t cfsr = (*((volatile uint32_t *)(0xE000ED28)));
  uint32_t hfsr = (*((volatile uint32_t *)(0xE000ED2C)));
  uint32_t dfsr = (*((volatile uint32_t *)(0xE000ED30)));
  uint32_t afsr = (*((volatile uint32_t *)(0xE000ED3C)));

  my_printf("  CFSR = 0x%08lX\r\n", cfsr);
  my_printf("  HFSR = 0x%08lX\r\n", hfsr);
  my_printf("  DFSR = 0x%08lX\r\n", dfsr);
  my_printf("  AFSR = 0x%08lX\r\n", afsr);

  my_printf("\r\n--- UsageFault Analysis ----------------------------------------\r\n");
  if (cfsr & 0x00800000) {
    my_printf("  [UsageFault] MemManage Fault also occurred\r\n");
  }
  if (cfsr & 0x00008000) {
    my_printf("  [UsageFault] Bus Fault also occurred\r\n");
  }
  if (cfsr & 0x00010000) {
    my_printf("  [UsageFault] UNDEFINSTR: Undefined instruction\r\n");
    my_printf("    -> Attempted to execute an undefined ARM instruction\r\n");
    if (stacked_pc != 0) {
      my_printf("    -> Instruction at: 0x%08lX\r\n", stacked_pc);
    }
  }
  if (cfsr & 0x00020000) {
    my_printf("  [UsageFault] INVSTATE: Invalid state\r\n");
    my_printf("    -> Tried to execute in ARM mode with Thumb bit clear\r\n");
    my_printf("    -> Or branch to non-word-aligned address\r\n");
  }
  if (cfsr & 0x00040000) {
    my_printf("  [UsageFault] INVPC: Invalid PC load\r\n");
    my_printf("    -> EXC_RETURN value is invalid\r\n");
    my_printf("    -> LR value: 0x%08lX\r\n", stacked_lr);
  }
  if (cfsr & 0x00080000) {
    my_printf("  [UsageFault] NOCP: No coprocessor\r\n");
    my_printf("    -> Attempted to access unavailable coprocessor\r\n");
    my_printf("    -> Possible FPU or DSP instruction without FPU enabled\r\n");
  }
  if (cfsr & 0x00100000) {
    my_printf("  [UsageFault] UNALIGNED: Unaligned memory access\r\n");
    my_printf("    -> Unaligned LDM/STM/PUSH/POP operation attempted\r\n");
    my_printf("    -> Accessing address: 0x%08lX\r\n", stacked_r0);
  }
  if (cfsr & 0x00200000) {
    my_printf("  [UsageFault] DIVBYZERO: Divide by zero\r\n");
    my_printf("    -> Integer division by zero attempted\r\n");
    my_printf("    -> Dividend (R0): 0x%08lX\r\n", stacked_r0);
    my_printf("    -> Divisor (R1):  0x%08lX\r\n", stacked_r1);
  }

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n");
  my_printf("  Program Counter (PC) = 0x%08lX\r\n", stacked_pc);
  my_printf("  Link Register (LR)   = 0x%08lX\r\n", stacked_lr);
  my_printf("  Stack Pointer (SP)   = 0x%08lX\r\n", (uint32_t)usage_args);
  
  my_printf("\r\n--- Common Causes ----------------------------------------------\r\n");
  my_printf("  1. NULL pointer dereference\r\n");
  my_printf("  2. Invalid function pointer call\r\n");
  my_printf("  3. Division by zero in integer arithmetic\r\n");
  my_printf("  4. Misaligned memory access\r\n");
  my_printf("  5. Corrupted stack or heap memory\r\n");

  my_printf("\r\n--- xPSR Analysis -------------------------------------------\r\n");
  if (stacked_xpsr & 0x00000080) {
    my_printf("  [xPSR] C: Carry flag set\r\n");
  }
  if (stacked_xpsr & 0x00000040) {
    my_printf("  [xPSR] Z: Zero flag set\r\n");
  }
  if (stacked_xpsr & 0x00000020) {
    my_printf("  [xPSR] N: Negative flag set\r\n");
  }
  if (stacked_xpsr & 0x00000010) {
    my_printf("  [xPSR] V: Overflow flag set\r\n");
  }
  if (stacked_xpsr & 0x00000008) {
    my_printf("  [xPSR] Q: Saturation flag set\r\n");
  }
  my_printf("  [xPSR] Thumb bit: %s\r\n", 
           (stacked_xpsr & 0x01000000) ? "Set (Thumb mode)" : "Clear");
  my_printf("  [xPSR] Exception Number: %lu\r\n", 
           (stacked_xpsr & 0x000001FF));
  
  uint8_t ipsr = stacked_xpsr & 0x000000FF;
  my_printf("  [xPSR] IPSR (Interrupt Program Status): %u\r\n", ipsr);

  my_printf("\r\n============================================================\r\n");
  my_printf("  [FATAL] System halted due to UsageFault exception\r\n");
  my_printf("============================================================\r\n");

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
  
  my_printf("\r\n");
  my_printf("============================================================\r\n");
  my_printf("  [WARNING] DebugMonitor Handler Triggered\r\n");
  my_printf("============================================================\r\n\r\n");

  uint32_t *debug_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;

  debug_args = (uint32_t *)__get_MSP();

  stacked_r0  = debug_args[0];
  stacked_r1  = debug_args[1];
  stacked_r2  = debug_args[2];
  stacked_r3  = debug_args[3];
  stacked_r12 = debug_args[4];
  stacked_lr  = debug_args[5];
  stacked_pc  = debug_args[6];
  stacked_xpsr = debug_args[7];

  my_printf("--- Stacked Registers ----------------------------------------\r\n");
  my_printf("  R0  = 0x%08lX  |  R1  = 0x%08lX\r\n", stacked_r0, stacked_r1);
  my_printf("  R2  = 0x%08lX  |  R3  = 0x%08lX\r\n", stacked_r2, stacked_r3);
  my_printf("  R12 = 0x%08lX\r\n", stacked_r12);
  my_printf("  LR  = 0x%08lX\r\n", stacked_lr);
  my_printf("  PC  = 0x%08lX  |  xPSR = 0x%08lX\r\n", stacked_pc, stacked_xpsr);

  my_printf("\r\n--- Debug Monitor Registers ----------------------------------\r\n");
  uint32_t dfsr  = (*((volatile uint32_t *)(0xE000ED30)));
  uint32_t hfsr  = (*((volatile uint32_t *)(0xE000ED2C)));
  uint32_t dcrsr = (*((volatile uint32_t *)(0xE000EDF4)));
  uint32_t dcrdr = (*((volatile uint32_t *)(0xE000EDF8)));
  uint32_t demcr = (*((volatile uint32_t *)(0xE000EDFC)));

  my_printf("  DFSR  = 0x%08lX\r\n", dfsr);
  my_printf("  HFSR  = 0x%08lX\r\n", hfsr);
  my_printf("  DCRSR = 0x%08lX\r\n", dcrsr);
  my_printf("  DCRDR = 0x%08lX\r\n", dcrdr);
  my_printf("  DEMCR = 0x%08lX\r\n", demcr);

  my_printf("\r\n--- Debug Fault Analysis --------------------------------------\r\n");
  if (demcr & 0x00000001) {
    my_printf("  [Debug] DEMCR: VC_CORERESET - Core reset vector trap enabled\r\n");
  }
  if (demcr & 0x00000002) {
    my_printf("  [Debug] DEMCR: VC_MMERR - MemManage fault trap enabled\r\n");
  }
  if (demcr & 0x00000004) {
    my_printf("  [Debug] DEMCR: VC_NOCPERR - No CP fault trap enabled\r\n");
  }
  if (demcr & 0x00000008) {
    my_printf("  [Debug] DEMCR: VC_CHKERR - Checking fault trap enabled\r\n");
  }
  if (demcr & 0x00000010) {
    my_printf("  [Debug] DEMCR: VC_STATERR - State fault trap enabled\r\n");
  }
  if (demcr & 0x00000020) {
    my_printf("  [Debug] DEMCR: VC_BUSERR - Bus fault trap enabled\r\n");
  }
  if (demcr & 0x00000040) {
    my_printf("  [Debug] DEMCR: VC_IRQERR - IRQ fault trap enabled\r\n");
  }
  if (demcr & 0x00000080) {
    my_printf("  [Debug] DEMCR: VC_SFERR - Secure fault trap enabled\r\n");
  }
  if (demcr & 0x00010000) {
    my_printf("  [Debug] DEMCR: TRCENA - Trace enable\r\n");
  }
  if (demcr & 0x01000000) {
    my_printf("  [Debug] DEMCR: MON_EN - DebugMonitor enabled\r\n");
  }
  if (demcr & 0x02000000) {
    my_printf("  [Debug] DEMCR: MON_PEND - DebugMonitor pending\r\n");
  }
  if (demcr & 0x04000000) {
    my_printf("  [Debug] DEMCR: MON_STEP - DebugMonitor step\r\n");
  }
  if (demcr & 0x08000000) {
    my_printf("  [Debug] DEMCR: MON_REQ - DebugMonitor request\r\n");
  }

  my_printf("\r\n--- Debug Event Source ----------------------------------------\r\n");
  if (dfsr & 0x00000001) {
    my_printf("  [Debug] HALTED: Core halted due to BKPT or DBGRQ\r\n");
  }
  if (dfsr & 0x00000002) {
    my_printf("  [Debug] BKPT: Breakpoint match\r\n");
    my_printf("    -> Breakpoint instruction encountered at: 0x%08lX\r\n", stacked_pc);
  }
  if (dfsr & 0x00000004) {
    my_printf("  [Debug] DWTTRAP: Data Watchpoint and Trace match\r\n");
  }
  if (dfsr & 0x00000008) {
    my_printf("  [Debug] VCATCH: Vector catch triggered\r\n");
    my_printf("    -> Vector catch event occurred\r\n");
  }
  if (dfsr & 0x00000010) {
    my_printf("  [Debug] EXTERNAL: External debug request\r\n");
    my_printf("    -> Debug request from external source\r\n");
  }

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n");
  my_printf("  Program Counter (PC) = 0x%08lX\r\n", stacked_pc);
  my_printf("  Link Register (LR)   = 0x%08lX\r\n", stacked_lr);
  my_printf("  Debug Data Reg (DCRDR) = 0x%08lX\r\n", dcrdr);

  my_printf("\r\n============================================================\r\n");
  my_printf("  [INFO] DebugMonitor exception handled\r\n");
  my_printf("  [INFO] System continuing (not halted)\r\n");
  my_printf("============================================================\r\n");

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
