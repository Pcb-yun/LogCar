/**
 * @file servo_driver.h
 * @author MIKE
 * @brief Fashion Star总线伺服舵机FreeRTOS驱动层
 */

#ifndef __SCAN_DRIVER_H__
#define __SCAN_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "freertos.h"
#include "Events.h"

extern osMessageQueueId_t Scan_Rx_DataHandle;

#define SCAN_FRAME_MAX_SIZE     512
#define SCAN_BARCODE_MAX_LEN    256
#define FRAME_TIMEOUT_MS        50    // 帧结束超时
#define SCAN_GROUP_WINDOW_MS    20   // 分组窗口：窗口内只保留最后一个条码

// 接收缓冲区
static struct {
    uint8_t buf[SCAN_FRAME_MAX_SIZE];
    uint16_t len;
    uint32_t last_time;
} rx = {0};

// 待输出条码（分组窗口机制）
static struct {
    char data[SCAN_BARCODE_MAX_LEN + 1];
    uint32_t time;      // 最后一次暂存的 tick
    bool valid;
} pending = {0};


void Scan_Init(void);
void Scan_Get_Task(void *argument);
bool Scan_GetLatestBarcode(char *buf, uint16_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SCAN_DRIVER_H__ */
