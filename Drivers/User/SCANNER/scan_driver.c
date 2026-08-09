/**
 * @file scan_driver.c
 * @brief 扫描器驱动层（StreamBuffer 版）
 */

#include "scan_driver.h"
#include "shell.h"
#include "log.h"
#include "usart.h"
#include <string.h>

/**
 * @brief 帧接收状态
 */
typedef struct {
    uint8_t  buf[SCAN_FRAME_MAX_SIZE];
    uint16_t len;
    uint32_t last_time;
} Scan_Rx_t;

/**
 * @brief 暂存条码
 */
typedef struct {
    char     data[SCAN_BARCODE_MAX_LEN + 1];
    uint32_t time;        /**< 时间戳 */
    bool     valid;       /**< 是否有效 */
    bool     consumed;    /**< 是否已被应用层读取(供 Scan_GetNewBarcode) */
} Scan_Pending_t;

StreamBufferHandle_t Scan_Rx_Stream = NULL;
static bool          is_init       = false;
static Scan_Rx_t     rx;
static Scan_Pending_t pending;

static bool scan_output_enabled  = true;
static bool pending_output       = false;

/**
 * @brief 从缓冲区提取可打印 ASCII 字符（过滤控制字符）
 * @param src  输入缓冲区指针
 * @param src_len 输入缓冲区长度
 * @param dst  输出缓冲区指针
 * @return 实际提取的字符数（包含 '\0'）
 */
static uint16_t extract_printable(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    uint16_t n = 0;
    for (uint16_t i = 0; i < src_len && n < SCAN_BARCODE_MAX_LEN; i++) {
        if (src[i] >= 0x20 && src[i] <= 0x7E) {
            dst[n++] = src[i];
        }
    }
    return n;
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
 * @brief 解析条码并暂存（覆盖窗口内之前的结果）
 *
 * 帧格式: 0x02 | 0x00 | LEN | DATA(LEN 字节) | CR/LF
 *   [0] STX   = 0x02
 *   [1] 状态   = 0x00
 *   [2] 数据长度
 *   [3 .. 3+LEN-1] 条码数据
 */
static void parse_and_store(void)
{
    if (rx.len == 0) return;

    uint8_t  barcode[SCAN_BARCODE_MAX_LEN + 1] = {0};
    uint16_t barcode_len = 0;

    if (rx.len >= 4 && rx.buf[0] == 0x02 && rx.buf[1] == 0x00) {
        uint8_t data_len = rx.buf[2];
        if (data_len > 0 && (uint16_t)(3 + data_len) <= rx.len) {
            barcode_len = extract_printable(rx.buf + 3, data_len, barcode);
        }
    }

    if (barcode_len == 0) {
        barcode_len = extract_printable(rx.buf, rx.len, barcode);
    }

    if (barcode_len > 0) {
        barcode[barcode_len] = '\0';
        strncpy(pending.data, (char *)barcode, SCAN_BARCODE_MAX_LEN);
        pending.data[SCAN_BARCODE_MAX_LEN] = '\0';
        pending.time   = HAL_GetTick();
        pending.valid  = true;
        pending.consumed = false;
        pending_output = false;
    }

    Scan_Clear_Buffer();
}

/**
 * @brief 分组窗口超时 → 输出暂存条码
 * @param now 当前时间戳（毫秒）
 */
static void flush_pending(uint32_t now)
{
    if (pending.valid && !pending_output &&
        (now - pending.time) >= SCAN_GROUP_WINDOW_MS) {
        if (scan_output_enabled) {
            logPrintln("[SCAN] %s", pending.data);
        }
        pending_output = true;
    }
}

/**
 * @brief 处理单个字节（状态机）
 * @param byte 接收到的字节
 * @param now  当前时间戳（毫秒）
 * *
 * CR(0x0D) / LF(0x0A) / 帧间超时 → 触发一次解析
 */
static void process_byte(uint8_t byte, uint32_t now)
{
    rx.last_time = now;

    if (byte == 0x0D || byte == 0x0A) {
        if (rx.len > 0) {
            parse_and_store();
        }
        return;  // 丢弃孤立的 CR/LF
    }

    if (rx.len < SCAN_FRAME_MAX_SIZE - 1) {
        rx.buf[rx.len++] = byte;
    } else {
        parse_and_store();
        rx.buf[0] = byte;
        rx.len    = 1;
    }
}

/**
 * @brief 初始化扫描器驱动层
 */
bool Scan_Init(void)
{
    
    MX_UART5_Init();
    memset(&rx, 0, sizeof(rx));
    memset(&pending, 0, sizeof(pending));

    Scan_Rx_Stream = xStreamBufferCreate(SCAN_STREAM_BUF_SIZE,
                                         SCAN_STREAM_TRIGGER_LVL);
    
    if (Scan_Rx_Stream == NULL) {
        configASSERT(Scan_Rx_Stream != NULL);
    } else {
        is_init = true;
    }
    
    return is_init;
}

/**
 * @brief 获取最新扫描到的条码
 * @param buf  存储条码的缓冲区
 * @param size 缓冲区大小（包含终止符 '\0'）
 * @return true 成功获取条码
 * @return false 无新条码
 */
bool Scan_GetLatestBarcode(char *buf, uint16_t size)
{
    if (!pending.valid) return false;

    strncpy(buf, pending.data, size - 1);
    buf[size - 1] = '\0';
    return true;
}

/**
 * @brief 获取一次新的扫码(每次扫码仅返回一次, 读取后标记为已消费)
 * @param buf  存储条码的缓冲区
 * @param size 缓冲区大小（包含终止符 '\0'）
 * @return true 成功获取一条新条码
 * @return false 无新条码(与上次相同或尚未扫描)
 *
 * 与 Scan_GetLatestBarcode 不同, 本接口消费式读取:
 * 即使连续扫到相同的码, 每次扫码也只返回一次。
 */
bool Scan_GetNewBarcode(char *buf, uint16_t size)
{
    if (!pending.valid || pending.consumed) return false;

    strncpy(buf, pending.data, size - 1);
    buf[size - 1] = '\0';
    pending.consumed = true;
    return true;
}

/**
 * @brief 扫描器接收任务
 *
 * 事件驱动：
 *   ISR 写入 StreamBuffer → xStreamBufferReceive 自动唤醒
 *   无数据时阻塞（带超时兜底处理不完整帧）
 */
void Scan_Get_Task(void *argument)
{
    (void)argument;
    uint8_t buf[64];
    uint32_t now;

    while (1) {
        /*
         * 阻塞读取：最多等 FRAME_TIMEOUT_MS
         *   - 有数据到达 → 立即返回，批量取出
         *   - 超时 → 返回 0，进入兜底逻辑
         */
        size_t n = xStreamBufferReceive(Scan_Rx_Stream,
                                        buf, sizeof(buf),
                                        pdMS_TO_TICKS(FRAME_TIMEOUT_MS));
        now = HAL_GetTick();

        if (n > 0) {
            for (size_t i = 0; i < n; i++) {
                process_byte(buf[i], now);
            }
        }

        if (rx.len > 0 && (HAL_GetTick() - rx.last_time) >= FRAME_TIMEOUT_MS) {
            parse_and_store();
        }

        flush_pending(HAL_GetTick());
    }
}

/**
 * @brief 扫描器控制命令
 * @param argc 命令参数数量
 * @param argv 命令参数数组
 */
static void Scan_Shell(int argc, char *argv[])
{
    if (argc < 2) {
        logPrintln("Usage: scan [on|off|status|get]");
        return;
    }

    if (strcmp(argv[1], "on") == 0) {
        scan_output_enabled = true;
        logPrintln("Scan output: ON");
    } else if (strcmp(argv[1], "off") == 0) {
        scan_output_enabled = false;
        logPrintln("Scan output: OFF");
    } else if (strcmp(argv[1], "status") == 0) {
        logPrintln("Output : %s", scan_output_enabled ? "ON" : "OFF");
        logPrintln("Pending: %s (age=%lums)",
                   pending.valid ? pending.data : "(none)",
                   pending.valid ? HAL_GetTick() - pending.time : 0UL);
        logPrintln("Stream : %u bytes available",
                   (unsigned)xStreamBufferBytesAvailable(Scan_Rx_Stream));
    } else if (strcmp(argv[1], "get") == 0) {
        char buf[SCAN_BARCODE_MAX_LEN + 1];
        if (Scan_GetLatestBarcode(buf, sizeof(buf))) {
            logPrintln("Latest barcode: %s", buf);
        } else {
            logPrintln("No new barcode available");
        }
    } else {
        logPrintln("Invalid argument: %s", argv[1]);
        logPrintln("Usage: scan [on|off|status|get]");
    }
}

/**
 * @brief 导出 Shell 命令
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0) | SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN) | SHELL_CMD_DISABLE_RETURN,
    scan,
    Scan_Shell,
    scan control: on | off | status | get
);
