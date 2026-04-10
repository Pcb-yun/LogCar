/**
 * @file track.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 巡线模块头文件
 */

#ifndef __TRACK_H__
#define __TRACK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/**
 * @brief 巡线模块模式枚举
 */
typedef enum {
    TRACK_CAL = 0,      // 校准模式
    TRACK_ANALOG,       // 发送模拟量
    TRACK_DIGITAL,      // 发送数字量
    TRACK_ALL,          // 发送所有数据
    TRACK_STOP,         // 停止发送
} TrackSet_t;

/**
 * @brief 巡线模块结构体
 */
typedef struct {
    TrackSet_t mode;
    uint16_t time;
} Track_t;

#define DIGITAL_LEN 43                      // 数字量数据长度
#define ANALOG_LEN 67                       // 模拟量数据长度
#define ALL_LEN DIGITAL_LEN + ANALOG_LEN    // 所有数据长度

#define CMD_CAL "$1,0,0#"                   // 校准模式
#define CMD_DIGITAL "$0,0,1#"               // 发送数字量
#define CMD_ANALOG "$0,1,0#"                // 发送模拟量
#define CMD_ALL "$0,1,1#"                   // 发送所有数据
#define CMD_STOP "$0,0,0#"                   // 停止发送

#define TRACK_KEY_Port GPIOG                // 按键端口
#define TRACK_KEY_PIN GPIO_PIN_13           // 按键引脚

#define TRACK_RST_Port GPIOG                // 重置端口
#define TRACK_RST_PIN GPIO_PIN_14           // 重置引脚


extern uint8_t trackBuffer[ALL_LEN];

void Track_Get_Task(void *argument);
void Track_Set_Mode(TrackSet_t mode);
uint8_t Track_Get_Size(void);


#define TRACK_SET_HELP \
    "Usage: set COMMAND [VALUE]\r\n" \
    "\r\n" \
    "commands:\r\n" \
    "  mode [cal|d|a|all|stop]  Set track mode\r\n" \
    "  time [value]            Set track sample time (ms)\r\n" \
    "\r\n" \
    "mode options:\r\n" \
    "  cal      Enable calibration mode\r\n" \
    "  a        Send analog data\r\n" \
    "  d        Send digital data\r\n" \
    "  all      Send all data\r\n" \
    "  stop     Stop sending data\r\n" \
    "  rst      Reset track module\r\n"


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TRACK_H__ */
