/**
 * @file scan_driver.h
 * @brief 扫描器驱动层
 */

#ifndef __SCAN_DRIVER_H
#define __SCAN_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "stream_buffer.h"

/* ========== 缓冲区配置 ========== */
#define SCAN_FRAME_MAX_SIZE      128
#define SCAN_BARCODE_MAX_LEN     64

/* ========== 超时配置（ms）========== */
#define FRAME_TIMEOUT_MS         20    // 字节间超时：超过此间隔视为一帧结束
#define SCAN_GROUP_WINDOW_MS     20    // 分组窗口：窗口内只保留最后一条

/* ========== StreamBuffer 配置 ========== */
#define SCAN_STREAM_BUF_SIZE     512   // 环形缓冲区大小（字节）
#define SCAN_STREAM_TRIGGER_LVL  1     // 收到多少字节才唤醒消费者

/* ========== 接收状态结构 ========== */
typedef struct {
    uint8_t  buf[SCAN_FRAME_MAX_SIZE];
    uint16_t len;
    uint32_t last_time;
} Scan_Rx_t;

/* ========== 暂存结构 ========== */
typedef struct {
    char     data[SCAN_BARCODE_MAX_LEN + 1];
    uint32_t time;
    bool     valid;
} Scan_Pending_t;

/* ========== 全局变量（extern）========== */
extern StreamBufferHandle_t Scan_Rx_Stream;
extern Scan_Rx_t            rx;
extern Scan_Pending_t       pending;

/* ========== API ========== */
bool Scan_Init(void);
void Scan_Get_Task(void *argument);
bool Scan_GetLatestBarcode(char *buf, uint16_t size);

#endif /* __SCAN_DRIVER_H */
