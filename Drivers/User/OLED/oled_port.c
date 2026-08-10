/**
 * @file oled_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief OLED模块用户层源文件
 */

#include "oled_port.h"
#include "ssd1306_fonts.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Events.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "log.h"
#include "spi.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief OLED初始化函数
 */
void OLED_Init(void) {
    MX_SPI1_Init();
    ssd1306_Init();
}

/**
 * @brief OLED刷新任务
 */
void Oled_Refresh_Task(void *argument) {
    (void) argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    extern osMutexId_t OLED_MutexHandle;

    for (;;) {
        osDelay(OLED_REFRESH_DELAY);
        osMutexAcquire(OLED_MutexHandle, osWaitForever);
        ssd1306_UpdateScreen();
        osMutexRelease(OLED_MutexHandle);
    }
}

/**
 * @brief 清除OLED屏幕
 */
static void Oled_Clear(void) {
    extern osMutexId_t OLED_MutexHandle;
    osMutexAcquire(OLED_MutexHandle, osWaitForever);
    ssd1306_Fill(Black);
    osMutexRelease(OLED_MutexHandle);
}

/**
 * @brief 设置OLED对比度
 */
static void Oled_SetContrast(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: oled ctrs <contrast>"); return;
    }
    uint8_t contrast = atoi(argv[1]);
    if (contrast > 255) {
        logPrintln("invalid contrast value: %d, must be in range [0, 255]", contrast); return;
    }
    ssd1306_SetContrast(contrast);
    logPrintln("set contrast to %d", contrast);
}

/**
 * @brief OLED开关控制
 */
static void Oled_Power(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: oled power <0/1>"); return;
    }
    uint8_t power = atoi(argv[1]);
    ssd1306_SetDisplayOn(power);
    logPrintln("display power %s", power ? "on" : "off");
}

/**
 * @brief 屏幕绘制工具
 */
static void Oled_Draw(int argc, char *argv[]) {
    extern osMutexId_t OLED_MutexHandle;

    if (argc < 2) {
        logPrintln(OLED_DRAW_HELP); return;
    }

    osMutexAcquire(OLED_MutexHandle, osWaitForever);

    if (strcmp(argv[1], "pixel") == 0) {
        if (argc != 5) { logPrintln("Usage: oled draw pixel <x> <y> <color>"); goto end; }
        ssd1306_DrawPixel(atoi(argv[2]), atoi(argv[3]), (SSD1306_COLOR)atoi(argv[4]));
    }
    else if (strcmp(argv[1], "line") == 0) {
        if (argc != 7) { logPrintln("Usage: oled draw line <x1> <y1> <x2> <y2> <color>"); goto end; }
        ssd1306_Line(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), (SSD1306_COLOR)atoi(argv[6]));
    }
    else if (strcmp(argv[1], "rect") == 0) {
        if (argc != 7) { logPrintln("Usage: oled draw rect <x1> <y1> <x2> <y2> <color>"); goto end; }
        ssd1306_DrawRectangle(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), (SSD1306_COLOR)atoi(argv[6]));
    }
    else if (strcmp(argv[1], "fillrect") == 0) {
        if (argc != 7) { logPrintln("Usage: oled draw fillrect <x1> <y1> <x2> <y2> <color>"); goto end; }
        ssd1306_FillRectangle(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), (SSD1306_COLOR)atoi(argv[6]));
    }
    else if (strcmp(argv[1], "invertrect") == 0) {
        if (argc != 6) { logPrintln("Usage: oled draw invertrect <x1> <y1> <x2> <y2>"); goto end; }
        ssd1306_InvertRectangle(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    }
    else if (strcmp(argv[1], "circle") == 0) {
        if (argc != 6) { logPrintln("Usage: oled draw circle <x> <y> <r> <color>"); goto end; }
        ssd1306_DrawCircle(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), (SSD1306_COLOR)atoi(argv[5]));
    }
    else if (strcmp(argv[1], "fillcircle") == 0) {
        if (argc != 6) { logPrintln("Usage: oled draw fillcircle <x> <y> <r> <color>"); goto end; }
        ssd1306_FillCircle(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), (SSD1306_COLOR)atoi(argv[5]));
    }
    else if (strcmp(argv[1], "arc") == 0) {
        if (argc != 8) { logPrintln("Usage: oled draw arc <x> <y> <r> <start> <sweep> <color>"); goto end; }
        ssd1306_DrawArc(atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]), (SSD1306_COLOR)atoi(argv[7]));
    }
    else if (strcmp(argv[1], "cursor") == 0) {
        if (argc != 4) { logPrintln("Usage: oled draw cursor <x> <y>"); goto end; }
        ssd1306_SetCursor(atoi(argv[2]), atoi(argv[3]));
    }
    else if (strcmp(argv[1], "char") == 0) {
        if (argc != 5) { logPrintln("Usage: oled draw char <ch> <font> <color>"); goto end; }
        const SSD1306_Font_t *font = &Font_6x8;
        switch (atoi(argv[3])) {
            case 0: font = &Font_6x8; break;
#ifdef SSD1306_INCLUDE_FONT_7x10
            case 1: font = &Font_7x10; break;
 #endif
 #ifdef SSD1306_INCLUDE_FONT_11x18
            case 2: font = &Font_11x18; break;
 #endif
 #ifdef SSD1306_INCLUDE_FONT_16x15
            case 3: font = &Font_16x15; break;
 #endif
 #ifdef SSD1306_INCLUDE_FONT_16x24
            case 4: font = &Font_16x24; break;
 #endif
 #ifdef SSD1306_INCLUDE_FONT_16x26
            case 5: font = &Font_16x26; break;
 #endif
         }
        ssd1306_WriteChar(argv[2][0], *font, (SSD1306_COLOR)atoi(argv[4]));
    }
    else if (strcmp(argv[1], "string") == 0) {
        if (argc != 5) { logPrintln("Usage: oled draw string <str> <font> <color>"); goto end; }
        const SSD1306_Font_t *font = &Font_6x8;
        switch (atoi(argv[3])) {
            case 0: font = &Font_6x8; break;
#ifdef SSD1306_INCLUDE_FONT_7x10
           case 1: font = &Font_7x10; break;
#endif
#ifdef SSD1306_INCLUDE_FONT_11x18
           case 2: font = &Font_11x18; break;
#endif
#ifdef SSD1306_INCLUDE_FONT_16x15
           case 3: font = &Font_16x15; break;
#endif
#ifdef SSD1306_INCLUDE_FONT_16x24
           case 4: font = &Font_16x24; break;
#endif
#ifdef SSD1306_INCLUDE_FONT_16x26
           case 5: font = &Font_16x26; break;
#endif
        }
        ssd1306_WriteString(argv[2], *font, (SSD1306_COLOR)atoi(argv[4]));
    }
    else if (strcmp(argv[1], "bitmap") == 0) {
        logPrintln("Bitmap drawing not supported via shell, use Play_Image API instead");
    }
    else {
        logPrintln("Unknown command: %s", argv[1]);
    }

end:
    osMutexRelease(OLED_MutexHandle);
}


ShellCommand OLED_Group[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, clear, Oled_Clear, clear screen),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, ctrs, Oled_SetContrast, set oled contrast),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, power, Oled_Power, set oled power),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, draw, Oled_Draw, screen draw tool),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
                       oled, OLED_Group, OLED Tool);
