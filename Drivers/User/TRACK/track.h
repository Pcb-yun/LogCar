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
    TrackSet_t mode;     // 模式
    uint16_t time;       // 时间间隔(ms)
} Track_t;

/**
 * @brief 巡线模块数据结构体
 */
typedef struct {
    uint8_t digitalData;        // 数字量数据
    uint16_t analogData[8];     // 模拟量数据
    TrackSet_t mode;            // 模式
} TrackData_t;

#define TRACK_TIMEOUT 300                                       // 串口发送超时时间(ms)

#define TRACK_DIGITAL_LEN 43                                    // 数字量数据长度
#define TRACK_ANALOG_LEN 67                                     // 模拟量数据长度
#define TRACK_ALL_LEN TRACK_DIGITAL_LEN + TRACK_ANALOG_LEN      // 所有数据长度

extern uint8_t trackBuffer[TRACK_ALL_LEN];

#define TRACK_CMD_CAL "$1,0,0#"                                 // 校准模式
#define TRACK_CMD_DIGITAL "$0,0,1#"                             // 发送数字量
#define TRACK_CMD_ANALOG "$0,1,0#"                              // 发送模拟量
#define TRACK_CMD_ALL "$0,1,1#"                                 // 发送所有数据
#define TRACK_CMD_STOP "$0,0,0#"                                // 停止发送

#define TRACK_KEY_Port GPIOG                                    // 按键端口
#define TRACK_KEY_PIN GPIO_PIN_13                               // 按键引脚

#define TRACK_RST_Port GPIOG                                    // 重置端口
#define TRACK_RST_PIN GPIO_PIN_14                               // 重置引脚

// 模式设置帮助信息
#define TRACK_MODE_HELP \
    "Usage: mode COMMAND\r\n" \
    "\r\n" \
    "commands:\r\n" \
    "  cal      Enable calibration mode\r\n" \
    "  a        Send analog data\r\n" \
    "  d        Send digital data\r\n" \
    "  all      Send all data\r\n" \
    "  stop     Stop sending data\r\n" \
    "  rst      Reset track module"

// 时间设置帮助信息
#define TRACK_TIME_HELP \
    "Usage: time [value] (ms)"

// 校准帮助信息
#define TRACK_CAL_HELP_1 \
    "Calibration Mode Started\r\n" \
    "1. When red light is on, place all sensors on black line for 3s, then press Enter"

// 校准帮助信息
#define TRACK_CAL_HELP_2 \
    "2. Place all sensors on white line for 3s, then press Enter"

// 校准帮助信息
#define TRACK_CAL_HELP_3 \
    "3. Check red light status:\r\n" \
    "- Red light off: Calibration success\r\n" \
    "- Red light slow blink: Need recalibration"


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TRACK_H__ */
