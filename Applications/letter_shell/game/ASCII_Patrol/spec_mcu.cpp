/**
 * @file spec_mcu.cpp
 * @brief ASCII Patrol 嵌入式平台适配层
 *
 * 实现 spec.h 中定义的接口，针对 FreeRTOS + UART 环境适配
 * @version 1.0.0
 * @date 2026-06-08
 *
 * @copyright (c) 2026
 */

#include "spec.h"
#include "shell.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/** 终端宽度 */
#define TERMINAL_WIDTH	110
/** 终端高度 */
#define TERMINAL_HEIGHT 25

/** 输入缓冲区大小 */
#define INPUT_BUFFER_SIZE 16

/** 按键释放超时时间 */
#define KEY_RELEASE_TIMEOUT_MS 100
/** 最多跟踪的按键数量 */
#define MAX_TRACKED_KEYS 4

/** 输出缓冲区大小 */
#define OUTPUT_BUF_SIZE 4096

static CON_INPUT g_input_buffer[INPUT_BUFFER_SIZE];
static int g_input_head = 0;
static int g_input_tail = 0;

static struct {
	char key;
	unsigned long last_time;
} g_key_state[MAX_TRACKED_KEYS];

static int g_key_state_count = 0;

extern Shell *ascii_patrol_shell;
static char *g_output_buf = NULL;

/**
 * @brief 从 shell 读取字符
 *
 * @return int 读取到的字符，失败返回-1
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
 *
 * @return unsigned int 时间戳
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
	osDelay(100);
}

/**
 * @brief 延时函数
 *
 * @param ms 延时毫秒数
 */
void sleep_ms(int ms)
{
	osDelay(ms);
}

/**
 * @brief 终端初始化
 *
 * @param argc 参数个数
 * @param argv 参数数组
 * @param dw 返回终端宽度
 * @param dh 返回终端高度
 * @return int 0表示成功
 */
int terminal_init(int argc, char* argv[], int* dw, int* dh)
{
	(void)argc;
	(void)argv;

	if (dw) {
		*dw = TERMINAL_WIDTH;
	}
	if (dh) {
		*dh = TERMINAL_HEIGHT;
	}

	if (!g_output_buf) {
		g_output_buf = (char*)pvPortMalloc(OUTPUT_BUF_SIZE);
	}

	g_input_head = 0;
	g_input_tail = 0;

	return 0;
}

/**
 * @brief 终端清理
 */
void terminal_done()
{
	if (g_output_buf) {
		vPortFree(g_output_buf);
		g_output_buf = NULL;
	}
}

/**
 * @brief 获取终端尺寸
 *
 * @param dw 返回终端宽度
 * @param dh 返回终端高度
 */
void get_terminal_wh(int* dw, int* dh)
{
	if (dw) {
		*dw = TERMINAL_WIDTH;
	}
	if (dh) {
		*dh = TERMINAL_HEIGHT;
	}
}

/**
 * @brief 释放终端输出结构
 *
 * @param screen 终端输出结构
 */
void free_con_output(CON_OUTPUT* screen)
{
	if (screen) {
		if (screen->buf) {
			vPortFree(screen->buf);
			screen->buf = NULL;
		}
		screen->color = NULL;
	}
}

/**
 * @brief 终端刷新（串口输出无需刷新）
 */
void terminal_flush()
{
}

/**
 * @brief 终端清屏（由 screen_write 处理）
 */
void terminal_clear()
{
}

/**
 * @brief 屏幕写入
 *
 * @param screen 屏幕输出结构
 * @param dw 终端宽度
 * @param dh 终端高度
 * @param sx 起始x坐标
 * @param sy 起始y坐标
 * @param sw 写入宽度
 * @param sh 写入高度
 * @return int 0表示成功
 */
int screen_write(CON_OUTPUT* screen, int dw, int dh, int sx, int sy, int sw, int sh)
{
	(void)dw;
	(void)dh;
	(void)sx;
	(void)sy;
	(void)sw;
	(void)sh;

	if (!screen || !screen->buf || !g_output_buf) {
		return 0;
	}

	char *output_buf = g_output_buf;
	int len = 0;

	len += sprintf(output_buf + len, "\033[%dA\033[J\033[2A", TERMINAL_HEIGHT);

	for (int y = 0; y < screen->h && y < TERMINAL_HEIGHT; y++) {
		len += sprintf(output_buf + len, "\033[%d;1H", y + 1);

		for (int x = 0; x < screen->w && x < TERMINAL_WIDTH; x++) {
			int idx = y * (screen->w + 1) + x;
			if (idx < (screen->w + 1) * screen->h) {
				char c = screen->buf[idx];
				c = c ? c : ' ';
				output_buf[len++] = c;
			}
		}
		output_buf[len++] = '\r';
		output_buf[len++] = '\n';
	}

	if (len > 0 && ascii_patrol_shell) {
		ascii_patrol_shell->write(output_buf, len);
	}

	return 0;
}

/**
 * @brief 更新按键状态跟踪
 *
 * @param ch 按键字符
 */
static void update_key_state(char ch)
{
	unsigned long now = get_time();

	for (int i = 0; i < g_key_state_count; i++) {
		if (g_key_state[i].key == ch) {
			g_key_state[i].last_time = now;
			return;
		}
	}

	if (g_key_state_count < MAX_TRACKED_KEYS) {
		g_key_state[g_key_state_count].key = ch;
		g_key_state[g_key_state_count].last_time = now;
		g_key_state_count++;
	}
}

/**
 * @brief 检查超时并生成按键释放事件
 */
static void check_key_releases()
{
	unsigned long now = get_time();

	for (int i = 0; i < g_key_state_count; ) {
		unsigned long elapsed = now - g_key_state[i].last_time;

		if (elapsed >= KEY_RELEASE_TIMEOUT_MS) {
			int next = (g_input_head + 1) % INPUT_BUFFER_SIZE;
			if (next != g_input_tail) {
				g_input_buffer[g_input_head].EventType = CON_INPUT_KBD;
				g_input_buffer[g_input_head].Event.KeyEvent.bKeyDown = false;
				g_input_buffer[g_input_head].Event.KeyEvent.uChar.AsciiChar = g_key_state[i].key;
				g_input_head = next;
			}

			g_key_state[i] = g_key_state[g_key_state_count - 1];
			g_key_state_count--;
		} else {
			i++;
		}
	}
}

/**
 * @brief 从 shell 读取字符并缓冲为游戏输入事件
 *
 * 所有从 shell 读取的字符都会入缓冲。游戏中的 ConfMapInput()
 * 会将原始按键映射为游戏操作（1/2/4/8/16 等），不需要在适配层过滤。
 *
 * @param ch 输入字符
 */
static void push_input_event(char ch)
{
	update_key_state(ch);

	int next = (g_input_head + 1) % INPUT_BUFFER_SIZE;
	if (next != g_input_tail) {
		g_input_buffer[g_input_head].EventType = CON_INPUT_KBD;
		g_input_buffer[g_input_head].Event.KeyEvent.bKeyDown = true;
		g_input_buffer[g_input_head].Event.KeyEvent.uChar.AsciiChar = ch;
		g_input_head = next;
	}
}

/**
 * @brief 获取输入长度
 *
 * @param r 返回输入数量
 * @return bool true表示成功
 */
bool get_input_len(int* r)
{
	if (!r) {
		return false;
	}

	int ch = read_uart_char();
	while (ch >= 0) {
		push_input_event((char)ch);
		ch = read_uart_char();
	}

	check_key_releases();

	int count = 0;
	int pos = g_input_tail;
	while (pos != g_input_head) {
		count++;
		pos = (pos + 1) % INPUT_BUFFER_SIZE;
	}
	*r = count;

	return true;
}

/**
 * @brief 读取输入（适配层包装）
 *
 * @param ir 输入事件数组
 * @param n 最大读取数量
 * @param r 返回实际读取数量
 * @return bool true表示成功
 */
bool spec_read_input(CON_INPUT* ir, int n, int* r)
{
	return read_input(ir, n, r);
}

/**
 * @brief 读取输入
 *
 * @param ir 输入事件数组
 * @param n 最大读取数量
 * @param r 返回实际读取数量
 * @return bool true表示成功
 */
bool read_input(CON_INPUT* ir, int n, int* r)
{
	if (!ir || n <= 0 || !r) {
		return false;
	}

	int count = 0;
	while (count < n && g_input_tail != g_input_head) {
		ir[count] = g_input_buffer[g_input_tail];
		g_input_tail = (g_input_tail + 1) % INPUT_BUFFER_SIZE;
		count++;
	}

	*r = count;
	return count > 0;
}

/**
 * @brief 检查是否有按键释放（通过超时检测实现）
 *
 * @return bool true表示支持
 */
bool has_key_releases()
{
	return true;
}

/**
 * @brief 调试输出
 *
 * @param str 调试字符串
 */
void DBG(const char* str)
{
	if (!ascii_patrol_shell) {
		return;
	}

	char buf[128];
	int len = sprintf(buf, "[DBG] %s\r\n", str);

	ascii_patrol_shell->write(buf, len);
}

/**
 * @brief new 操作符重载（使用 FreeRTOS 内存分配）
 *
 * @param size 分配大小
 * @return void* 分配的内存指针
 */
void* operator new(size_t size)
{
	void* p = pvPortMalloc(size);
	return p;
}

/**
 * @brief new[] 操作符重载（使用 FreeRTOS 内存分配）
 *
 * @param size 分配大小
 * @return void* 分配的内存指针
 */
void* operator new[](size_t size)
{
	void* p = pvPortMalloc(size);
	return p;
}

/**
 * @brief delete 操作符重载（使用 FreeRTOS 内存释放）
 *
 * @param ptr 释放的内存指针
 */
void operator delete(void* ptr)
{
	vPortFree(ptr);
}

/**
 * @brief delete[] 操作符重载（使用 FreeRTOS 内存释放）
 *
 * @param ptr 释放的内存指针
 */
void operator delete[](void* ptr)
{
	vPortFree(ptr);
}

/**
 * @brief delete 操作符重载（带大小）
 *
 * @param ptr 释放的内存指针
 * @param size 大小（未使用）
 */
void operator delete(void* ptr, size_t size)
{
	(void)size;
	vPortFree(ptr);
}

/**
 * @brief delete[] 操作符重载（带大小）
 *
 * @param ptr 释放的内存指针
 * @param size 大小（未使用）
 */
void operator delete[](void* ptr, size_t size)
{
	(void)size;
	vPortFree(ptr);
}

/**
 * @brief 游戏主循环
 */
void terminal_loop()
{
	while (modal) {
		int result = modal->Run();
		if (result < 0) {
			break;
		}
		osDelay(1);
	}
}

/**
 * @brief 应用退出
 */
void app_exit()
{
	modal = NULL;
}
