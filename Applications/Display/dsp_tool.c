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

extern osMutexId_t OLED_MutexHandle;
static ShowTool_t show_tool = SHOW_TOOL_OPS;
static bool Tool_NAV(void);
static bool Tool_BAT(void);


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
