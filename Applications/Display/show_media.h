/**
 * @file show_media.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 媒体显示模块用户层头文件
 */

#ifndef __SHOW_MEDIA_H__
#define __SHOW_MEDIA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 媒体类型枚举
 */
typedef enum {
    MEDIA_TYPE_IMAGE = 0,
    MEDIA_TYPE_GIF,
    MEDIA_TYPE_TOOL
} MediaType_type;

/**
 * @brief 媒体数据结构体
 */
typedef struct {
    MediaType_type type;  // 媒体类型
    void *data;           // 媒体数据指针
} Media_t;


bool Play_Image(const char *name);
bool Play_Gif(const char *name);
void Show_About(void);
void Media_stop(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SHOW_MEDIA_H__ */
