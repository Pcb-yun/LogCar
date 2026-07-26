/**
 * @file image_data.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 图片数据头文件
 */

#ifndef __IMAGE_DATA_H__
#define __IMAGE_DATA_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 图片数据结构体
 */
typedef struct {
    const char *name;           // 名称
    const uint8_t *data;        // 数据指针
    const uint8_t width;        // 宽度
    const uint8_t height;       // 高度
    const uint8_t background;   // 背景颜色 (0: 黑色, 1: 白色)
} Image_t;


bool find_image(const char *name, const Image_t **data);
void Image_list(void);




#endif /* __IMAGE_DATA_H__ */
