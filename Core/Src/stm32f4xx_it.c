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
static void print_stacked_registers(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3,
                                  uint32_t r12, uint32_t lr, uint32_t pc, uint32_t xpsr);
static void print_xpsr_analysis(uint32_t xpsr);

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_adc1;
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_i2c1_rx;
extern I2C_HandleTypeDef hi2c1;
extern DMA_HandleTypeDef hdma_spi1_tx;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim4;
extern DMA_HandleTypeDef hdma_uart4_rx;
extern DMA_HandleTypeDef hdma_uart5_rx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern DMA_HandleTypeDef hdma_usart6_tx;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart1;
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
  my_printf("\r\n============================================================\r\n"
             "  [CRITICAL] NMI Handler Triggered\r\n"
             "============================================================\r\n\r\n");

  uint32_t *nmi_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;
  uint32_t nvic_icsr, nvic_adr, nvic_shp12, nvic_shp13, rcc_csr;

  nmi_args = (uint32_t *)__get_MSP();

  stacked_r0  = nmi_args[0];
  stacked_r1  = nmi_args[1];
  stacked_r2  = nmi_args[2];
  stacked_r3  = nmi_args[3];
  stacked_r12 = nmi_args[4];
  stacked_lr  = nmi_args[5];
  stacked_pc  = nmi_args[6];
  stacked_xpsr = nmi_args[7];

  nvic_icsr = *(volatile uint32_t *)0xE000ED04;
  nvic_adr  = *(volatile uint32_t *)0xE000ED08;
  nvic_shp12 = (*(volatile uint32_t *)0xE000ED20) >> 24;
  nvic_shp13 = (*(volatile uint32_t *)0xE000ED20) >> 16;
  rcc_csr   = *(volatile uint32_t *)0x40023804;

  print_stacked_registers(stacked_r0, stacked_r1, stacked_r2, stacked_r3,
                          stacked_r12, stacked_lr, stacked_pc, stacked_xpsr);

  my_printf("--- NMI Configuration Registers -------------------------------\r\n"
             "  NVIC_ICSR  = 0x%08lX\r\n"
             "  NVIC_ADR   = 0x%08lX\r\n"
             "  NVIC_SHp12 = 0x%08lX\r\n"
             "  NVIC_SHp13 = 0x%08lX\r\n"
             "  RCC_CSR    = 0x%08lX\r\n\r\n"
             "--- NMI Source Analysis --------------------------------------\r\n",
           nvic_icsr, nvic_adr, nvic_shp12, nvic_shp13, rcc_csr);
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

  print_xpsr_analysis(stacked_xpsr);

  my_printf("============================================================\r\n"
             "  [FATAL] System halted due to NMI exception\r\n"
             "============================================================\r\n");

  Error_Handler();

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
  my_printf("\r\n============================================================\r\n"
             "  [CRITICAL] HardFault Handler Triggered\r\n"
             "============================================================\r\n\r\n");

  uint32_t *hardfault_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;
  uint32_t cfsr, hfsr, dfsr, afsr, bfar, mmfar;

  hardfault_args = (uint32_t *)__get_MSP();

  stacked_r0  = hardfault_args[0];
  stacked_r1  = hardfault_args[1];
  stacked_r2  = hardfault_args[2];
  stacked_r3  = hardfault_args[3];
  stacked_r12 = hardfault_args[4];
  stacked_lr  = hardfault_args[5];
  stacked_pc  = hardfault_args[6];
  stacked_xpsr = hardfault_args[7];

  cfsr   = *(volatile uint32_t *)0xE000ED28;
  hfsr   = *(volatile uint32_t *)0xE000ED2C;
  dfsr   = *(volatile uint32_t *)0xE000ED30;
  afsr   = *(volatile uint32_t *)0xE000ED3C;
  bfar   = *(volatile uint32_t *)0xE000ED38;
  mmfar  = *(volatile uint32_t *)0xE000ED34;

  print_stacked_registers(stacked_r0, stacked_r1, stacked_r2, stacked_r3,
                          stacked_r12, stacked_lr, stacked_pc, stacked_xpsr);

  my_printf("--- Fault Status Registers -----------------------------------\r\n"
             "  CFSR  = 0x%08lX\r\n"
             "  HFSR  = 0x%08lX\r\n"
             "  DFSR  = 0x%08lX\r\n"
             "  AFSR  = 0x%08lX\r\n"
             "  BFAR  = 0x%08lX\r\n"
             "  MMFAR = 0x%08lX\r\n\r\n"
             "--- Fault Analysis --------------------------------------------\r\n",
           cfsr, hfsr, dfsr, afsr, bfar, mmfar);

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
  my_printf("\r\n--- Fault Context Information --------------------------------\r\n"
             "  Program Counter (PC) = 0x%08lX\r\n"
             "  Link Register (LR)   = 0x%08lX\r\n"
             "  Stack Pointer (MSP)  = 0x%08lX\r\n",
           stacked_pc, stacked_lr, (uint32_t)hardfault_args);
  if (bfar != 0) {
    my_printf("  Bus Fault Address    = 0x%08lX\r\n", bfar);
  }
  if (mmfar != 0) {
    my_printf("  MemManage Address    = 0x%08lX\r\n", mmfar);
  }

  print_xpsr_analysis(stacked_xpsr);

  my_printf("============================================================\r\n"
             "  [FATAL] System halted due to HardFault exception\r\n"
             "============================================================\r\n");
  Error_Handler();

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
  my_printf("\r\n============================================================\r\n"
             "  [CRITICAL] MemManage Handler Triggered\r\n"
             "============================================================\r\n\r\n");

  uint32_t *mm_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;
  uint32_t cfsr, hfsr, dfsr, afsr, mmfar;

  mm_args = (uint32_t *)__get_MSP();

  stacked_r0  = mm_args[0];
  stacked_r1  = mm_args[1];
  stacked_r2  = mm_args[2];
  stacked_r3  = mm_args[3];
  stacked_r12 = mm_args[4];
  stacked_lr  = mm_args[5];
  stacked_pc  = mm_args[6];
  stacked_xpsr = mm_args[7];

  cfsr   = *(volatile uint32_t *)0xE000ED28;
  hfsr   = *(volatile uint32_t *)0xE000ED2C;
  dfsr   = *(volatile uint32_t *)0xE000ED30;
  afsr   = *(volatile uint32_t *)0xE000ED3C;
  mmfar  = *(volatile uint32_t *)0xE000ED34;

  print_stacked_registers(stacked_r0, stacked_r1, stacked_r2, stacked_r3,
                          stacked_r12, stacked_lr, stacked_pc, stacked_xpsr);

  my_printf("--- MemManage Fault Registers -------------------------------\r\n"
             "  CFSR  = 0x%08lX\r\n"
             "  HFSR  = 0x%08lX\r\n"
             "  DFSR  = 0x%08lX\r\n"
             "  AFSR  = 0x%08lX\r\n"
             "  MMFAR = 0x%08lX\r\n\r\n"
             "--- MemManage Fault Analysis --------------------------------\r\n",
           cfsr, hfsr, dfsr, afsr, mmfar);
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

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n"
             "  Fault Address (MMFAR) = 0x%08lX\r\n"
             "  Program Counter (PC) = 0x%08lX\r\n"
             "  Link Register (LR)   = 0x%08lX\r\n",
           mmfar, stacked_pc, stacked_lr);
  if (stacked_pc != 0) {
    my_printf("  Instruction at fault: 0x%08lX\r\n", stacked_pc);
  }

  print_xpsr_analysis(stacked_xpsr);

  my_printf("============================================================\r\n"
             "  [FATAL] System halted due to MemManage exception\r\n"
             "============================================================\r\n");
  Error_Handler();

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
  my_printf("\r\n============================================================\r\n"
             "  [CRITICAL] BusFault Handler Triggered\r\n"
             "============================================================\r\n\r\n");

  uint32_t *bus_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;
  uint32_t cfsr, hfsr, dfsr, afsr, bfar;

  bus_args = (uint32_t *)__get_MSP();

  stacked_r0  = bus_args[0];
  stacked_r1  = bus_args[1];
  stacked_r2  = bus_args[2];
  stacked_r3  = bus_args[3];
  stacked_r12 = bus_args[4];
  stacked_lr  = bus_args[5];
  stacked_pc  = bus_args[6];
  stacked_xpsr = bus_args[7];

  cfsr   = *(volatile uint32_t *)0xE000ED28;
  hfsr   = *(volatile uint32_t *)0xE000ED2C;
  dfsr   = *(volatile uint32_t *)0xE000ED30;
  afsr   = *(volatile uint32_t *)0xE000ED3C;
  bfar   = *(volatile uint32_t *)0xE000ED38;

  print_stacked_registers(stacked_r0, stacked_r1, stacked_r2, stacked_r3,
                          stacked_r12, stacked_lr, stacked_pc, stacked_xpsr);

  my_printf("--- BusFault Registers ----------------------------------------\r\n"
             "  CFSR = 0x%08lX\r\n"
             "  HFSR = 0x%08lX\r\n"
             "  DFSR = 0x%08lX\r\n"
             "  AFSR = 0x%08lX\r\n"
             "  BFAR = 0x%08lX\r\n\r\n"
             "--- BusFault Analysis ------------------------------------------\r\n",
           cfsr, hfsr, dfsr, afsr, bfar);
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

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n"
             "  Bus Fault Address (BFAR) = 0x%08lX\r\n"
             "  Program Counter (PC)    = 0x%08lX\r\n"
             "  Link Register (LR)     = 0x%08lX\r\n",
           bfar, stacked_pc, stacked_lr);
  if (stacked_r0 != 0) {
    my_printf("  Memory access address  = 0x%08lX\r\n", stacked_r0);
  }

  my_printf("\r\n--- Possible Causes -------------------------------------------\r\n"
             "  1. Invalid memory access to peripheral register\r\n"
             "  2. Attempted to access non-existent memory region\r\n"
             "  3. Bus timeout or peripheral error\r\n"
             "  4. Flash memory read error (ECC failure)\r\n");

  print_xpsr_analysis(stacked_xpsr);

  my_printf("============================================================\r\n"
             "  [FATAL] System halted due to BusFault exception\r\n"
             "============================================================\r\n");

  Error_Handler();

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
  my_printf("\r\n============================================================\r\n"
             "  [CRITICAL] UsageFault Handler Triggered\r\n"
             "============================================================\r\n\r\n");

  uint32_t *usage_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;
  uint32_t cfsr, hfsr, dfsr, afsr;

  usage_args = (uint32_t *)__get_MSP();

  stacked_r0  = usage_args[0];
  stacked_r1  = usage_args[1];
  stacked_r2  = usage_args[2];
  stacked_r3  = usage_args[3];
  stacked_r12 = usage_args[4];
  stacked_lr  = usage_args[5];
  stacked_pc  = usage_args[6];
  stacked_xpsr = usage_args[7];

  cfsr   = *(volatile uint32_t *)0xE000ED28;
  hfsr   = *(volatile uint32_t *)0xE000ED2C;
  dfsr   = *(volatile uint32_t *)0xE000ED30;
  afsr   = *(volatile uint32_t *)0xE000ED3C;

  print_stacked_registers(stacked_r0, stacked_r1, stacked_r2, stacked_r3,
                          stacked_r12, stacked_lr, stacked_pc, stacked_xpsr);

  my_printf("--- UsageFault Registers ---------------------------------------\r\n"
             "  CFSR = 0x%08lX\r\n"
             "  HFSR = 0x%08lX\r\n"
             "  DFSR = 0x%08lX\r\n"
             "  AFSR = 0x%08lX\r\n\r\n"
             "--- UsageFault Analysis ----------------------------------------\r\n",
           cfsr, hfsr, dfsr, afsr);
  if (cfsr & 0x00800000) {
    my_printf("  [UsageFault] MemManage Fault also occurred\r\n");
  }
  if (cfsr & 0x00008000) {
    my_printf("  [UsageFault] Bus Fault also occurred\r\n");
  }
  if (cfsr & 0x00010000) {
    my_printf("  [UsageFault] UNDEFINSTR: Undefined instruction\r\n"
               "    -> Attempted to execute an undefined ARM instruction\r\n");
    if (stacked_pc != 0) {
      my_printf("    -> Instruction at: 0x%08lX\r\n", stacked_pc);
    }
  }
  if (cfsr & 0x00020000) {
    my_printf("  [UsageFault] INVSTATE: Invalid state\r\n"
               "    -> Tried to execute in ARM mode with Thumb bit clear\r\n"
               "    -> Or branch to non-word-aligned address\r\n");
  }
  if (cfsr & 0x00040000) {
    my_printf("  [UsageFault] INVPC: Invalid PC load\r\n"
               "    -> EXC_RETURN value is invalid\r\n"
               "    -> LR value: 0x%08lX\r\n", stacked_lr);
  }
  if (cfsr & 0x00080000) {
    my_printf("  [UsageFault] NOCP: No coprocessor\r\n"
               "    -> Attempted to access unavailable coprocessor\r\n"
               "    -> Possible FPU or DSP instruction without FPU enabled\r\n");
  }
  if (cfsr & 0x00100000) {
    my_printf("  [UsageFault] UNALIGNED: Unaligned memory access\r\n"
               "    -> Unaligned LDM/STM/PUSH/POP operation attempted\r\n"
               "    -> Accessing address: 0x%08lX\r\n", stacked_r0);
  }
  if (cfsr & 0x00200000) {
    my_printf("  [UsageFault] DIVBYZERO: Divide by zero\r\n"
               "    -> Integer division by zero attempted\r\n"
               "    -> Dividend (R0): 0x%08lX\r\n"
               "    -> Divisor (R1):  0x%08lX\r\n", stacked_r0, stacked_r1);
  }

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n"
             "  Program Counter (PC) = 0x%08lX\r\n"
             "  Link Register (LR)   = 0x%08lX\r\n"
             "  Stack Pointer (SP)   = 0x%08lX\r\n",
           stacked_pc, stacked_lr, (uint32_t)usage_args);

  my_printf("--- Common Causes ----------------------------------------------\r\n"
             "  1. NULL pointer dereference\r\n"
             "  2. Invalid function pointer call\r\n"
             "  3. Division by zero in integer arithmetic\r\n"
             "  4. Misaligned memory access\r\n"
             "  5. Corrupted stack or heap memory\r\n");

  print_xpsr_analysis(stacked_xpsr);

  my_printf("============================================================\r\n"
             "  [FATAL] System halted due to UsageFault exception\r\n"
             "============================================================\r\n");

  Error_Handler();

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
  my_printf("\r\n============================================================\r\n"
             "  [WARNING] DebugMonitor Handler Triggered\r\n"
             "============================================================\r\n\r\n");

  uint32_t *debug_args;
  uint32_t stacked_r0, stacked_r1, stacked_r2, stacked_r3;
  uint32_t stacked_r12, stacked_lr, stacked_pc, stacked_xpsr;
  uint32_t dfsr, hfsr, dcrsr, dcrdr, demcr;

  debug_args = (uint32_t *)__get_MSP();

  stacked_r0  = debug_args[0];
  stacked_r1  = debug_args[1];
  stacked_r2  = debug_args[2];
  stacked_r3  = debug_args[3];
  stacked_r12 = debug_args[4];
  stacked_lr  = debug_args[5];
  stacked_pc  = debug_args[6];
  stacked_xpsr = debug_args[7];

  dfsr   = *(volatile uint32_t *)0xE000ED30;
  hfsr   = *(volatile uint32_t *)0xE000ED2C;
  dcrsr  = *(volatile uint32_t *)0xE000ED34;
  dcrdr  = *(volatile uint32_t *)0xE000ED38;
  demcr  = *(volatile uint32_t *)0xE000EDFC;

  print_stacked_registers(stacked_r0, stacked_r1, stacked_r2, stacked_r3,
                          stacked_r12, stacked_lr, stacked_pc, stacked_xpsr);

  my_printf("--- Debug Monitor Registers ----------------------------------\r\n"
             "  DFSR  = 0x%08lX\r\n"
             "  HFSR  = 0x%08lX\r\n"
             "  DCRSR = 0x%08lX\r\n"
             "  DCRDR = 0x%08lX\r\n"
             "  DEMCR = 0x%08lX\r\n\r\n"
             "--- Debug Fault Analysis --------------------------------------\r\n",
           dfsr, hfsr, dcrsr, dcrdr, demcr);
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
    my_printf("  [Debug] BKPT: Breakpoint match\r\n"
               "    -> Breakpoint instruction encountered at: 0x%08lX\r\n", stacked_pc);
  }
  if (dfsr & 0x00000004) {
    my_printf("  [Debug] DWTTRAP: Data Watchpoint and Trace match\r\n");
  }
  if (dfsr & 0x00000008) {
    my_printf("  [Debug] VCATCH: Vector catch triggered\r\n"
               "    -> Vector catch event occurred\r\n");
  }
  if (dfsr & 0x00000010) {
    my_printf("  [Debug] EXTERNAL: External debug request\r\n"
               "    -> Debug request from external source\r\n");
  }

  my_printf("\r\n--- Fault Context Information --------------------------------\r\n"
             "  Program Counter (PC) = 0x%08lX\r\n"
             "  Link Register (LR)   = 0x%08lX\r\n"
             "  Debug Data Reg (DCRDR) = 0x%08lX\r\n\r\n"
             "============================================================\r\n"
             "  [INFO] DebugMonitor exception handled\r\n"
             "  [INFO] System continuing (not halted)\r\n"
             "============================================================\r\n",
           stacked_pc, stacked_lr, dcrdr);

  Error_Handler();

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
  * @brief This function handles EXTI line3 interrupt.
  */
void EXTI3_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI3_IRQn 0 */

  /* USER CODE END EXTI3_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(START_Pin);
  /* USER CODE BEGIN EXTI3_IRQn 1 */

  /* USER CODE END EXTI3_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream0 global interrupt.
  */
void DMA1_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream0_IRQn 0 */

  /* USER CODE END DMA1_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_uart5_rx);
  /* USER CODE BEGIN DMA1_Stream0_IRQn 1 */

  /* USER CODE END DMA1_Stream0_IRQn 1 */
}

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
  * @brief This function handles DMA1 stream4 global interrupt.
  */
void DMA1_Stream4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream4_IRQn 0 */

  /* USER CODE END DMA1_Stream4_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_tx);
  /* USER CODE BEGIN DMA1_Stream4_IRQn 1 */

  /* USER CODE END DMA1_Stream4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 stream5 global interrupt.
  */
void DMA1_Stream5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Stream5_IRQn 0 */

  /* USER CODE END DMA1_Stream5_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_i2c1_rx);
  /* USER CODE BEGIN DMA1_Stream5_IRQn 1 */

  /* USER CODE END DMA1_Stream5_IRQn 1 */
}

/**
  * @brief This function handles ADC1, ADC2 and ADC3 global interrupts.
  */
void ADC_IRQHandler(void)
{
  /* USER CODE BEGIN ADC_IRQn 0 */

  /* USER CODE END ADC_IRQn 0 */
  HAL_ADC_IRQHandler(&hadc1);
  /* USER CODE BEGIN ADC_IRQn 1 */

  /* USER CODE END ADC_IRQn 1 */
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
  * @brief This function handles TIM4 global interrupt.
  */
void TIM4_IRQHandler(void)
{
  /* USER CODE BEGIN TIM4_IRQn 0 */

  /* USER CODE END TIM4_IRQn 0 */
  HAL_TIM_IRQHandler(&htim4);
  /* USER CODE BEGIN TIM4_IRQn 1 */

  /* USER CODE END TIM4_IRQn 1 */
}

/**
  * @brief This function handles I2C1 event interrupt.
  */
void I2C1_EV_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_EV_IRQn 0 */

  /* USER CODE END I2C1_EV_IRQn 0 */
  HAL_I2C_EV_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_EV_IRQn 1 */

  /* USER CODE END I2C1_EV_IRQn 1 */
}

/**
  * @brief This function handles I2C1 error interrupt.
  */
void I2C1_ER_IRQHandler(void)
{
  /* USER CODE BEGIN I2C1_ER_IRQn 0 */

  /* USER CODE END I2C1_ER_IRQn 0 */
  HAL_I2C_ER_IRQHandler(&hi2c1);
  /* USER CODE BEGIN I2C1_ER_IRQn 1 */

  /* USER CODE END I2C1_ER_IRQn 1 */
}

/**
  * @brief This function handles SPI1 global interrupt.
  */
void SPI1_IRQHandler(void)
{
  /* USER CODE BEGIN SPI1_IRQn 0 */

  /* USER CODE END SPI1_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi1);
  /* USER CODE BEGIN SPI1_IRQn 1 */

  /* USER CODE END SPI1_IRQn 1 */
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
  * @brief This function handles UART5 global interrupt.
  */
void UART5_IRQHandler(void)
{
  /* USER CODE BEGIN UART5_IRQn 0 */

  /* USER CODE END UART5_IRQn 0 */
  HAL_UART_IRQHandler(&huart5);
  /* USER CODE BEGIN UART5_IRQn 1 */

  /* USER CODE END UART5_IRQn 1 */
}

/**
  * @brief This function handles DMA2 stream0 global interrupt.
  */
void DMA2_Stream0_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream0_IRQn 0 */

  /* USER CODE END DMA2_Stream0_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_adc1);
  /* USER CODE BEGIN DMA2_Stream0_IRQn 1 */

  /* USER CODE END DMA2_Stream0_IRQn 1 */
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
  * @brief This function handles DMA2 stream3 global interrupt.
  */
void DMA2_Stream3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream3_IRQn 0 */

  /* USER CODE END DMA2_Stream3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_spi1_tx);
  /* USER CODE BEGIN DMA2_Stream3_IRQn 1 */

  /* USER CODE END DMA2_Stream3_IRQn 1 */
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

static void print_stacked_registers(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3,
                                    uint32_t r12, uint32_t lr, uint32_t pc, uint32_t xpsr) {
  my_printf("--- Stacked Registers ----------------------------------------\r\n"
             "  R0  = 0x%08lX  |  R1  = 0x%08lX\r\n"
             "  R2  = 0x%08lX  |  R3  = 0x%08lX\r\n"
             "  R12 = 0x%08lX\r\n"
             "  LR  = 0x%08lX\r\n"
             "  PC  = 0x%08lX  |  xPSR = 0x%08lX\r\n\r\n",
           r0, r1, r2, r3, r12, lr, pc, xpsr);
}

static void print_xpsr_analysis(uint32_t xpsr) {
  my_printf("\r\n--- xPSR Analysis -------------------------------------------\r\n"
             "  [xPSR] C: Carry flag set\r\n"
             "  [xPSR] Z: Zero flag set\r\n"
             "  [xPSR] N: Negative flag set\r\n"
             "  [xPSR] V: Overflow flag set\r\n"
             "  [xPSR] Q: Saturation flag set\r\n"
             "  [xPSR] Thumb bit: %s\r\n"
             "  [xPSR] Exception Number: %lu\r\n"
             "  [xPSR] IPSR (Interrupt Program Status): %u\r\n\r\n",
           (xpsr & 0x01000000) ? "Set (Thumb mode)" : "Clear",
           (xpsr & 0x000001FF),
           (uint8_t)(xpsr & 0x000000FF));
}

/* USER CODE END 1 */
