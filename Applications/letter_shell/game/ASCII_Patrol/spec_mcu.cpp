/**
 * @file spec_mcu.cpp
 * @brief ASCII Patrol 嵌入式平台适配层
 *
 * 实现 spec.h 中定义的接口，针对 FreeRTOS + UART 环境适配
 */

#include "spec.h"
#include "shell.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 终端尺寸常量
#define TERMINAL_WIDTH  90
#define TERMINAL_HEIGHT 25

// 游戏内部颜色格式（每个字节高4位=背景，低4位=前景）
// 映射到 ANSI 16色标准
static const char ansi_color_map[16] = {
	0,   // 0: black
	4,   // 1: blue
	2,   // 2: green
	6,   // 3: cyan
	1,   // 4: red
	5,   // 5: magenta
	3,   // 6: yellow/brown
	7,   // 7: white
	8,   // 8: gray
	12,  // 9: bright blue
	10,  // A: bright green
	14,  // B: bright cyan
	9,   // C: bright red
	13,  // D: bright magenta
	11,  // E: bright yellow
	15   // F: bright white
};

// 按键缓冲区
#define INPUT_BUFFER_SIZE 16
static CON_INPUT g_input_buffer[INPUT_BUFFER_SIZE];
static int g_input_head = 0;
static int g_input_tail = 0;

// 当前 shell 实例（由 temp.cpp 中的 main_ascii_patrol 设置）
extern Shell *ascii_patrol_shell;

// 屏幕输出缓冲区（动态分配，terminal_init 中分配，terminal_done 中释放）
#define OUTPUT_BUF_SIZE 5120
static char *g_output_buf = NULL;

/**
 * @brief 从 shell 读取字符
 */
static int read_uart_char(void)
{
	char ch;
	if (ascii_patrol_shell && ascii_patrol_shell->read(&ch, 1) > 0) {
		return (int)ch;
	}
	return -1;
}

/**
 * @brief 获取毫秒时间戳
 */
unsigned int get_time()
{
    return (unsigned int)xTaskGetTickCount();
}

/**
 * @brief 垂直同步等待（简化为固定延时）
 */
void vsync_wait()
{
    osDelay(80);
}

/**
 * @brief 延时函数
 */
void sleep_ms(int ms)
{
    osDelay(ms);
}

// ==================== 终端管理 ====================

int terminal_init(int argc, char* argv[], int* dw, int* dh)
{
    (void)argc;
    (void)argv;

    // 设置终端尺寸
    if (dw) *dw = TERMINAL_WIDTH;
    if (dh) *dh = TERMINAL_HEIGHT;

    // 分配输出缓冲区
    if (!g_output_buf) {
        g_output_buf = (char*)pvPortMalloc(OUTPUT_BUF_SIZE);
    }

    // 清空输入缓冲区
    g_input_head = 0;
    g_input_tail = 0;

    return 0;
}

void terminal_done()
{
    // 释放输出缓冲区
    if (g_output_buf) {
        vPortFree(g_output_buf);
        g_output_buf = NULL;
    }
}

void get_terminal_wh(int* dw, int* dh)
{
    if (dw) *dw = TERMINAL_WIDTH;
    if (dh) *dh = TERMINAL_HEIGHT;
}

void free_con_output(CON_OUTPUT* screen)
{
    // screen->color 始终指向 screen->buf 内部，不需要独立释放
    if (screen) {
        if (screen->buf) {
            vPortFree(screen->buf);
            screen->buf = NULL;
        }
        screen->color = NULL;
    }
}

void terminal_flush()
{
    // 串口输出无需刷新
}

void terminal_clear()
{
    // 清屏由 screen_write 处理
}

// ==================== 屏幕写入 ====================

int screen_write(CON_OUTPUT* screen, int dw, int dh, int sx, int sy, int sw, int sh)
{
    (void)dw;
    (void)dh;
    (void)sx;
    (void)sy;
    (void)sw;
    (void)sh;

    if (!screen || !screen->buf || !g_output_buf) return 0;

    char *output_buf = g_output_buf;
    int len = 0;

    // 发送清屏命令
    len += sprintf(output_buf + len, "\033[%dA\033[J\033[2A", TERMINAL_HEIGHT);

    // 当前活动的前景色和背景色（用于跟踪变化）
    int cur_fg = -1, cur_bg = -1;

    // 逐行发送内容
    for (int y = 0; y < screen->h && y < TERMINAL_HEIGHT; y++) {
        // 移动光标到行首
        len += sprintf(output_buf + len, "\033[%d;1H", y + 1);

        // 发送该行内容
        for (int x = 0; x < screen->w && x < TERMINAL_WIDTH; x++) {
            int idx = y * screen->w + x;
            if (idx < screen->w * screen->h) {
                char c = screen->buf[idx];
                c = c ? c : ' ';

                // 输出颜色转义
                if (screen->color) {
                    unsigned char col = screen->color[idx];
                    int fg = ansi_color_map[col & 0x0F];
                    int bg = ansi_color_map[(col >> 4) & 0x0F];

                    if (fg != cur_fg || bg != cur_bg) {
                        int fg_code = (fg < 8) ? (fg + 30) : (fg + 82);   // 30-37 或 90-97
                        int bg_code = (bg < 8) ? (bg + 40) : (bg + 92);   // 40-47 或 100-107
                        len += sprintf(output_buf + len, "\033[%d;%dm",
                                       fg_code, bg_code);
                        cur_fg = fg;
                        cur_bg = bg;
                    }
                }

                output_buf[len++] = c;
            }
        }
        // 行尾复位颜色
        if (screen->color) {
            len += sprintf(output_buf + len, "\033[0m");
            cur_fg = cur_bg = -1;
        }
        // 换行
        output_buf[len++] = '\r';
        output_buf[len++] = '\n';
    }

    // 发送完整缓冲区（通过 shell 写接口）
    if (len > 0 && ascii_patrol_shell) {
        ascii_patrol_shell->write(output_buf, len);
    }

    return 0;
}

/**
 * @brief 从 shell 读取字符并缓冲为游戏输入事件
 *
 * 所有从 shell 读取的字符都会入缓冲。游戏中的 ConfMapInput()
 * 会将原始按键映射为游戏操作（1/2/4/8/16 等），不需要在适配层过滤。
 */
static void push_input_event(char ch)
{
	int next = (g_input_head + 1) % INPUT_BUFFER_SIZE;
	if (next != g_input_tail) {
		g_input_buffer[g_input_head].EventType = CON_INPUT_KBD;
		g_input_buffer[g_input_head].Event.KeyEvent.bKeyDown = true;
		g_input_buffer[g_input_head].Event.KeyEvent.uChar.AsciiChar = ch;
		g_input_head = next;
	}
}

// ==================== 输入处理 ====================

bool get_input_len(int* r)
{
    if (!r) return false;

    // 从 shell 读取所有可用字符，全部入缓冲
    int ch = read_uart_char();
    while (ch >= 0) {
        push_input_event((char)ch);
        ch = read_uart_char();
    }

    // 计算可用输入数量
    int count = 0;
    int pos = g_input_tail;
    while (pos != g_input_head) {
        count++;
        pos = (pos + 1) % INPUT_BUFFER_SIZE;
    }
    *r = count;

    return true;
}

bool spec_read_input(CON_INPUT* ir, int n, int* r)
{
    return read_input(ir, n, r);
}

bool read_input(CON_INPUT* ir, int n, int* r)
{
    if (!ir || n <= 0 || !r) return false;

    int count = 0;
    while (count < n && g_input_tail != g_input_head) {
        ir[count] = g_input_buffer[g_input_tail];
        g_input_tail = (g_input_tail + 1) % INPUT_BUFFER_SIZE;
        count++;
    }

    *r = count;
    return count > 0;
}

bool has_key_releases()
{
    // 嵌入式环境简化为始终返回 false
    return false;
}

// ==================== 调试输出 ====================

void DBG(const char* str)
{
	// 调试信息通过 shell 发送
	if (!ascii_patrol_shell) return;

	char buf[128];
	int len = sprintf(buf, "[DBG] %s\r\n", str);

	ascii_patrol_shell->write(buf, len);
}

// ==================== 全局 new/delete 重载 ====================
// 使用 FreeRTOS 内存分配，替代 C 标准库 malloc/free

void* operator new(size_t size)
{
	void* p = pvPortMalloc(size);
	return p;
}

void* operator new[](size_t size)
{
	void* p = pvPortMalloc(size);
	return p;
}

void operator delete(void* ptr)
{
	vPortFree(ptr);
}

void operator delete[](void* ptr)
{
	vPortFree(ptr);
}

void operator delete(void* ptr, size_t size)
{
	(void)size;
	vPortFree(ptr);
}

void operator delete[](void* ptr, size_t size)
{
	(void)size;
	vPortFree(ptr);
}

// ==================== 游戏主循环 ====================

void terminal_loop()
{
    // 游戏主循环，运行 modal 直到返回
    while (modal) {
        int result = modal->Run();
        if (result < 0) {
            // 负数退出码，退出游戏
            break;
        }
        // 正数或零通常表示继续下一个 modal
        osDelay(1);
    }
}

// ==================== 应用控制 ====================

void app_exit()
{
    // 设置 modal 为 NULL，terminal_loop 检测到后退出，返回 shell
    modal = NULL;
}
