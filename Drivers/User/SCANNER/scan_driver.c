/**
 * @file scan_driver.c
 * @brief 扫描器驱动层
 */

#include "scan_driver.h"
#include "shell.h"
#include "log.h"
#include "usart.h"
#include <string.h>
#include <stdbool.h>

static bool scan_output_enabled = true;   // 默认开启输出

/**
 * @brief 从接收缓冲区提取可打印字符
 * @param src 输入缓冲区
 * @param src_len 输入缓冲区长度
 * @param dst 输出缓冲区
 * @return 实际提取的字符数
 */
static uint16_t extract_printable(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t dst_len = 0;
    for (uint16_t i = 0; i < src_len && dst_len < SCAN_BARCODE_MAX_LEN; i++) {
        if (src[i] >= 0x20 && src[i] <= 0x7E) {
            dst[dst_len++] = src[i];
        }
    }
    return dst_len;
}

/**
 * @brief 清空接收缓冲区
 */
static void Scan_Clear_Buffer(void)
{
    rx.len = 0;
    memset(rx.buf, 0, SCAN_FRAME_MAX_SIZE);
    rx.last_time = HAL_GetTick();
}

/**
 * @brief 解析条码并暂存（不立即输出）
 *
 * 扫描器每次触发会重发历史结果 + 真正结果，
 * 间隔仅 2~3ms。暂存机制让后者覆盖前者，
 * 等窗口超时后只输出最后一个（即真正结果）。
 */
static void parse_and_store(void)
{
    if (rx.len == 0) return;

    uint8_t barcode[SCAN_BARCODE_MAX_LEN + 1] = {0};
    uint16_t barcode_len = 0;

    // 协议帧
    if (rx.len >= 5 && rx.buf[0] == 0x02 && rx.buf[1] == 0x00) {
        uint8_t data_len = rx.buf[3];
        if (data_len > 0 && 4 + data_len <= rx.len) {
            barcode_len = extract_printable(rx.buf + 4, data_len, barcode);
        }
    } else {
        // 纯文本
        barcode_len = extract_printable(rx.buf, rx.len, barcode);
    }

    if (barcode_len > 0) {
        barcode[barcode_len] = '\0';
        // 暂存：覆盖窗口内之前到达的条码
        strncpy(pending.data, (char *)barcode, SCAN_BARCODE_MAX_LEN);
        pending.data[SCAN_BARCODE_MAX_LEN] = '\0';
        pending.time = HAL_GetTick();
        pending.valid = true;
    }

    Scan_Clear_Buffer();
}

/**
 * @brief 分组窗口超时后输出暂存的条码
 */
static void flush_pending(uint32_t now)
{
    if (pending.valid && (now - pending.time) >= SCAN_GROUP_WINDOW_MS) {
        if (scan_output_enabled) {
            logPrintln("[SCAN] %s", pending.data);
        }
        pending.valid = false;
    }
}

/**
 * @brief 处理接收到的字节
 * @param byte 接收到的字节
 * @param now 当前时间戳
 */
static void process_byte(uint8_t byte, uint32_t now)
{
    rx.last_time = now;

    // CR → 一帧结束，暂存解析结果
    if (byte == 0x0D && rx.len > 0) {
        parse_and_store();
        return;
    }

    // 存储原始字节
    if (rx.len < SCAN_FRAME_MAX_SIZE - 1) {
        rx.buf[rx.len++] = byte;
    } else {
        parse_and_store();
        rx.buf[0] = byte;
        rx.len = 1;
    }
}

/**
 * @brief 初始化扫描器驱动层
 */
void Scan_Init(void)
{
    MX_UART5_Init();
    memset(&rx, 0, sizeof(rx));
    memset(&pending, 0, sizeof(pending));
}

/**
 * @brief 接收任务，处理接收到的字节
 * @param argument 任务参数（未使用）
 */
void Scan_Get_Task(void *argument)
{
    (void)argument;
    uint8_t byte;
    uint32_t now;

    while (1) {
        now = HAL_GetTick();

        // 读取队列中所有字节
        while (osMessageQueueGet(Scan_Rx_DataHandle, &byte, NULL, 0) == osOK) {
            process_byte(byte, now);
        }

        // 帧超时兜底
        if (rx.len > 0 && (now - rx.last_time) >= FRAME_TIMEOUT_MS) {
            parse_and_store();
        }

        // 分组窗口超时 → 输出最后暂存的条码
        flush_pending(now);

        osDelay(1);
    }
}

/**
 * @brief 扫描器 Shell 命令：控制条码输出开关
 * @param argc 参数个数
 * @param argv 参数数组
 * @note 用法：
 *       scan on   - 开启条码输出到终端
 *       scan off  - 关闭条码输出到终端
 */
static void Scan_Shell(int argc, char *argv[])
{
    if (argc < 2) {
        logPrintln("Usage: scan [on|off]");
        return;
    }

    if (strcmp(argv[1], "on") == 0) {
        scan_output_enabled = true;
        logPrintln("Scan output: ON");
    } else if (strcmp(argv[1], "off") == 0) {
        scan_output_enabled = false;
        logPrintln("Scan output: OFF");
    } else {
        logPrintln("Invalid argument: %s", argv[1]);
        logPrintln("Usage: scan [on|off]");
    }
}

/**
 * @brief 导出 Shell 命令：scan
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
    scan,
    Scan_Shell,
    "scan control commands (on/off)"
);