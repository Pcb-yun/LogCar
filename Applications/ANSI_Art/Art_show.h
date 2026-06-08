/**
 * @file Art_show.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief ASCII Art渲染头文件
 */

#ifndef __ART_SHOW_H__
#define __ART_SHOW_H__

#include <stdbool.h>
#include "shell.h"

#if __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief ASCII Art图片数据结构体
 */
typedef struct {
    const char* name;
    const char* data;
} Artdata_t;


bool Art_show(const char *name);
bool find_art(const char *name, const char **data);
void Art_list(void);


//定义ASCII Art输出宏
#define ART_OUT(usr, data, len) usr->write((char *)data, len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_SHOW_H__ */
