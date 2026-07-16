/**
 * @file scan_driver.h
 * @brief 扫描器驱动层 - 公共接口
 */

#ifndef __SCAN_DRIVER_H
#define __SCAN_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "stream_buffer.h"

/**
 * @brief 缓冲区配置
 */
#define SCAN_FRAME_MAX_SIZE      128
#define SCAN_BARCODE_MAX_LEN     64

/**
 * @brief 字节间超时：超过此间隔视为一帧结束
 */
#define FRAME_TIMEOUT_MS         20    // 字节间超时：超过此间隔视为一帧结束
#define SCAN_GROUP_WINDOW_MS     20    // 分组窗口：窗口内只保留最后一条

/**
 * @brief StreamBuffer 配置
 */
#define SCAN_STREAM_BUF_SIZE     512   // 环形缓冲区大小（字节）
/**
 * @brief StreamBuffer 触发条件（字节）
 */
#define SCAN_STREAM_TRIGGER_LVL  1     // 收到多少字节才唤醒消费者

extern StreamBufferHandle_t     Scan_Rx_Stream;

bool Scan_Init(void);
void Scan_Get_Task(void *argument);
bool Scan_GetLatestBarcode(char *buf, uint16_t size);

#endif /* __SCAN_DRIVER_H */
