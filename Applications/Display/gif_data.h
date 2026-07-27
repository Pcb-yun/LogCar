/**
 * @file gif_data.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief GIF数据头文件
 */

#ifndef __GIF_DATA_H__
#define __GIF_DATA_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief GIF数据结构体
 */
typedef struct {
    const char *name;           // 名称
    const uint8_t *data;        // 数据指针
    const uint8_t width;        // 宽度
    const uint8_t height;       // 高度
    const uint16_t frame_count; // 帧数
    const uint16_t frame_size;  // 单帧大小（字节）
    const uint8_t background;   // 背景颜色 (0: 黑色, 1: 白色)
    const uint8_t fps;          // 帧率
} Gif_t;


bool find_gif(const char *name, const Gif_t **data);
void Gif_list(void);



#endif /* __GIF_DATA_H__ */
