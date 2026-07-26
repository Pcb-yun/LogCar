/**
 * @file oled_port.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief OLED模块用户层头文件
 */

#ifndef __OLED_PORT_H__
#define __OLED_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#define OLED_FPS                60	// 帧率
#define OLED_REFRESH_DELAY   1000 / OLED_FPS	// 刷新延迟


void OLED_Init(void);
void Oled_Refresh_Task(void *argument);


#define OLED_DRAW_HELP \
    "Usage: oled draw <cmd> [args...]\r\n" \
    "Commands:\r\n" \
    "  pixel <x> <y> <color>       Draw pixel\r\n" \
    "  line <x1> <y1> <x2> <y2> <color>\r\n" \
    "  rect <x1> <y1> <x2> <y2> <color>\r\n" \
    "  fillrect <x1> <y1> <x2> <y2> <color>\r\n" \
    "  invertrect <x1> <y1> <x2> <y2>\r\n" \
    "  circle <x> <y> <r> <color>\r\n" \
    "  fillcircle <x> <y> <r> <color>\r\n" \
    "  arc <x> <y> <r> <start> <sweep> <color>\r\n" \
    "  cursor <x> <y>              Set cursor\r\n" \
    "  char <ch> <font> <color>    Write char\r\n" \
    "  string <str> <font> <color> Write string\r\n" \
    "Font: 0=6x8, 1=7x10, 2=11x18, 3=16x15, 4=16x24, 5=16x26\r\n" \
    "Color: 0=Black, 1=White"


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLED_PORT_H__ */
