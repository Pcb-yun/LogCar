/**
 * @file dsp_tool.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 显示工具模块实现文件
 */

#include "dsp_tool.h"
#include "shell.h"
#include "log.h"
#include "oled_port.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "Events.h"
#include <stdio.h>

#include "nav_local.h"
#include "show_media.h"
#include "battery.h"
#include "image_data.h"
#include "gif_data.h"

extern osMutexId_t OLED_MutexHandle;
static ShowTool_t show_tool = SHOW_TOOL_OPS;
static bool Tool_NAV(void);
static bool Tool_BAT(void);


/**
 * @brief 画面轮询任务
 */
void AD_Rotation_Task(void *argument){
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);

    extern osMessageQueueId_t Media_DataHandle;
    Image_t *image = NULL;
    Gif_t *gif = NULL;

    for(;;) {
        if (find_image("Logo", (const Image_t **)&image)) {
            Media_t media = {MEDIA_TYPE_IMAGE, image};
            osMessageQueuePut(Media_DataHandle, &media, NULL, osWaitForever);
        }
        osDelay(LOGO_TIME);

        if (find_gif("Eevee1", (const Gif_t **)&gif)) {
            Media_t media = {MEDIA_TYPE_GIF, gif};
            osMessageQueuePut(Media_DataHandle, &media, NULL, osWaitForever);
        }
        osDelay(GIF_TIME);
        Media_stop();

        bat_show();
        osDelay(BAT_TIME);
        Media_stop();

        if (find_gif("Eevee2", (const Gif_t **)&gif)) {
            Media_t media = {MEDIA_TYPE_GIF, gif};
            osMessageQueuePut(Media_DataHandle, &media, NULL, osWaitForever);
        }
        osDelay(GIF_TIME);

        Show_About();
        osDelay(ABOUT_TIME);
    }
}

/**
 * @brief 显示工具模块保持运行
 * @return 是否继续刷新
 */
bool Tool_KeepAlive(void) {
    switch (show_tool) {
        case SHOW_TOOL_OPS: return Tool_NAV();
        case SHOW_TOOL_BAT: return Tool_BAT();
    }
    return false;
}

/**
 * @brief 在屏幕上显示位姿数据
 */
void nav_show(void) {
    extern osMessageQueueId_t Media_DataHandle;
    Media_t media = {MEDIA_TYPE_TOOL, NULL};
    show_tool = SHOW_TOOL_OPS;
    osMessageQueuePut(Media_DataHandle, &media, NULL, osWaitForever);
}

/**
 * @brief 在屏幕上显示电池电量
 */
void bat_show(void) {
    extern osMessageQueueId_t Media_DataHandle;
    Media_t media = {MEDIA_TYPE_TOOL, NULL};
    show_tool = SHOW_TOOL_BAT;
    osMessageQueuePut(Media_DataHandle, &media, NULL, osWaitForever);
}

/**
 * @brief 刷新一次位姿数据
 * @return 显示状态
 */
static bool Tool_NAV(void) {
    PoseTimestamp_t pose;
    char buf[16];
    if (!Loc_Get(&pose)) return false;

    osMutexAcquire(OLED_MutexHandle, osWaitForever);
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString("Pose:", Font_6x8, White);

    uint8_t y_offset = Font_6x8.height + 2;
    ssd1306_SetCursor(0, y_offset);
    snprintf(buf, sizeof(buf), "X: %.2f cm", pose.pose.x);
    ssd1306_WriteString(buf, Font_6x8, White);

    y_offset += Font_6x8.height + 2;
    ssd1306_SetCursor(0, y_offset);
    snprintf(buf, sizeof(buf), "Y: %.2f cm", pose.pose.y);
    ssd1306_WriteString(buf, Font_6x8, White);

    y_offset += Font_6x8.height + 2;
    ssd1306_SetCursor(0, y_offset);
    snprintf(buf, sizeof(buf), "Yaw: %.2f deg", pose.pose.yaw);
    ssd1306_WriteString(buf, Font_6x8, White);

    y_offset += Font_6x8.height + 2;
    ssd1306_SetCursor(0, y_offset);
    snprintf(buf, sizeof(buf), "Time: %d", pose.timestamp);
    ssd1306_WriteString(buf, Font_6x8, White);
    osMutexRelease(OLED_MutexHandle);

    osDelay(33);
    return true;
}

/**
 * @brief 刷新一次电池数据
 * @return 显示状态
 */
static bool Tool_BAT(void) {
    uint16_t voltage;
    char buf[16];
    if (!Battery_GetVoltage(&voltage)) return false;
    snprintf(buf, sizeof(buf), "BAT: %d mV", voltage);

    osMutexAcquire(OLED_MutexHandle, osWaitForever);
    ssd1306_Fill(Black);

    ssd1306_SetCursor(0, 0);
    ssd1306_WriteString(buf, Font_7x10, White);
    osMutexRelease(OLED_MutexHandle);

    osDelay(33);
    return true;
}

/**
 * @brief 显示开发者信息
 */
void Show_About(void) {
    const Image_t *image = NULL;
    if (!find_image("Txwz", &image)) {
        return;
    }

    Media_stop();
    extern osMutexId_t OLED_MutexHandle;
    osMutexAcquire(OLED_MutexHandle, osWaitForever);

    ssd1306_Fill(image->background ? White : Black);
    ssd1306_BlitPageData(0, 0, image->width, image->height, image->data);

    uint8_t x_offset = image->width + ((SSD1306_WIDTH - image->width) - Font_11x18.width * 7) / 2;
    uint8_t y_offset = 0;
    ssd1306_SetCursor(x_offset, y_offset);
    ssd1306_WriteString("Log Car", Font_11x18, White);

    x_offset = image->width + 3;
    y_offset = Font_11x18.height + 5;
    ssd1306_SetCursor(x_offset, y_offset);
    ssd1306_WriteString("by:", Font_7x10, White);

    x_offset = image->width + ((SSD1306_WIDTH - image->width) - Font_7x10.width * 7) / 2;
    y_offset = y_offset + Font_7x10.height + 2;
    ssd1306_SetCursor(x_offset, y_offset);
    ssd1306_WriteString("Pcb-yun", Font_7x10, White);

    x_offset = image->width + ((SSD1306_WIDTH - image->width) - Font_7x10.width * 4) / 2;
    y_offset = y_offset + Font_7x10.height + 2;
    ssd1306_SetCursor(x_offset, y_offset);
    ssd1306_WriteString("MIKE", Font_7x10, White);

    osMutexRelease(OLED_MutexHandle);
}
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    about, Show_About, Show developer information on screen);
