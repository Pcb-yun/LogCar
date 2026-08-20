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

#define BAT_TIME 5000       // 电池电压显示时间
#define GIF_TIME 17000      // 动图显示时间
#define LOGO_TIME 7000      // 启动logo显示时间
#define ABOUT_TIME 3000     // 关于信息显示时间
#define GITHUB_TIME 5000    // GitHub显示时间
#define DEEPSEEK_TIME 5000  // DeepSeek显示时间



bool Tool_KeepAlive(void);
void nav_show(void);
void bat_show(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DSP_TOOL_H__ */
