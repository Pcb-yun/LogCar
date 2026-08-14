/**
 * @file show_media.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 媒体显示模块用户层源文件
 */

#include "show_media.h"
#include "oled_port.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "shell.h"
#include "log.h"
#include "Events.h"
#include <string.h>
#include "image_data.h"
#include "gif_data.h"
#include "dsp_tool.h"

extern osMessageQueueId_t Media_DataHandle;
static bool Play = false;


/**
 * @brief 媒体播放任务
 */
void Media_Player_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    Media_t media;

    for (;;) {
        osMessageQueueGet(Media_DataHandle, &media, NULL, osWaitForever);
        switch (media.type) {
            case MEDIA_TYPE_IMAGE: Play_Image(((Image_t *)media.data)->name); break;
            case MEDIA_TYPE_GIF:
                Play = true;
                while (Play) if (!Play_Gif(((Gif_t *)media.data)->name)) Play = false; break;
            case MEDIA_TYPE_TOOL:
                Play = true;
                while (Play) if (!Tool_KeepAlive()) Play = false;
        }
    }
}

/**
 * @brief 显示图片
 * @param name 图片名称
 * @return 是否成功
 */
bool Play_Image(const char *name) {
    const Image_t *image = NULL;
    if (!find_image(name, &image)) {
        return false;
    }

    uint8_t x_offset = (SSD1306_WIDTH - image->width) / 2;
    uint8_t y_offset = (SSD1306_HEIGHT - image->height) / 2;
    extern osMutexId_t OLED_MutexHandle;

    osMutexAcquire(OLED_MutexHandle, osWaitForever);
    ssd1306_Fill(image->background ? White : Black);
    ssd1306_BlitPageData(x_offset, y_offset, image->width, image->height, image->data);
    osMutexRelease(OLED_MutexHandle);
    return true;
}

/**
 * @brief 图片显示命令
 */
static void Image_show_Shell(int argc, char *argv[]) {
    if (argc < 2) {
        logPrintln("Usage: image <name>\r\n"
                   "Available names:");
        Image_list(); return;
    }
    Image_t *image = NULL;

    if (!find_image(argv[1], (const Image_t **)&image)) {
        logPrintln("Image %s not found", argv[1]); return;
    }

    Play = false;
    Media_t media = {MEDIA_TYPE_IMAGE, image};
    if (osMessageQueuePut(Media_DataHandle, &media, NULL, osWaitForever) != osOK) {
        logPrintln("Image %s put to queue failed", argv[1]);
    } else {
        logPrintln("Show Image: %s", argv[1]);
    }
}
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    image, Image_show_Shell, Show image on screen);

/**
 * @brief 显示GIF动画
 * @param name GIF名称
 * @return 是否成功
 */
bool Play_Gif(const char *name) {
    const Gif_t *gif = NULL;
    if (!find_gif(name, &gif)) {
        return false;
    }

    uint8_t x_offset = (SSD1306_WIDTH - gif->width) / 2;
    uint8_t y_offset = (SSD1306_HEIGHT - gif->height) / 2;
    extern osMutexId_t OLED_MutexHandle;
    uint16_t frame_delay_ms = 1000 / gif->fps;
    osMutexAcquire(OLED_MutexHandle, osWaitForever);
    ssd1306_Fill(gif->background ? White : Black);
    osMutexRelease(OLED_MutexHandle);

    for (uint16_t frame = 0; frame < gif->frame_count; frame++) {
        if (!Play) break;
        osMutexAcquire(OLED_MutexHandle, osWaitForever);
        ssd1306_BlitPageData(x_offset, y_offset, gif->width, gif->height, &gif->data[frame * gif->frame_size]);
        osMutexRelease(OLED_MutexHandle);
        osDelay(frame_delay_ms);
    }
    return true;
}

/**
 * @brief GIF显示命令
 */
static void Gif_show_Shell(int argc, char *argv[]) {
    if (argc < 2) {
        logPrintln("Usage: gif <name>\r\n"
                   "Available names:");
        Gif_list(); return;
    }
    Gif_t *gif = NULL;

    if (strcmp(argv[1], "stop") == 0) {
        logPrintln("stop play GIF");
        Play = false; return;
    }

    if (!find_gif(argv[1], (const Gif_t **)&gif)) {
        logPrintln("GIF %s not found", argv[1]); return;
    }

    Play = false;
    Media_t media = {MEDIA_TYPE_GIF, gif};
    if (osMessageQueuePut(Media_DataHandle, &media, NULL, osWaitForever) != osOK) {
        logPrintln("GIF %s put to queue failed", argv[1]);
    } else {
        logPrintln("Show GIF: %s", argv[1]);
    }
}
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    gif, Gif_show_Shell, Show GIF on screen);

/**
 * @brief 显示开发者信息
 */
static void Show_About(void) {
    const Image_t *image = NULL;
    if (!find_image("Txwz", &image)) {
        return;
    }

    Play = false;
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
