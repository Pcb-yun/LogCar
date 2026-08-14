/**
 * @file dsp_tool.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 显示工具模块用户层头文件
 */

#ifndef __DSP_TOOL_H__
#define __DSP_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>

typedef enum {
    SHOW_TOOL_OPS = 0,
    SHOW_TOOL_BAT,
} ShowTool_t;



bool Tool_KeepAlive(void);
void nav_show(void);
void bat_show(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DSP_TOOL_H__ */
