/**
 * @file track.h
 * @brief 巡线模块头文件
 */

#ifndef __TRACK_H__
#define __TRACK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define TRACK_I2C_HANDLE    &hi2c1      // 请根据实际I2C外设修改
#define TRACK_I2C_ADDR      0x12        // 模块I2C地址

/**
 * @brief 巡线模块模式枚举
 */
typedef enum {
    TRACK_CAL = 0,
    TRACK_DIGITAL,
    TRACK_STOP,
} TrackMode_t;

/**
 * @brief 巡线模块状态枚举
 */
typedef enum {
    TRACK_STATUS_IDLE = 1,
    TRACK_STATUS_BUSY = 0,
} TrackI2CStatus_t;

/**
 * @brief 巡线模块数据结构体
 */
typedef struct {
    uint8_t digitalData;        // 数字量（bit7~bit0对应探头1~8）
    uint32_t timestamp;         // 时间戳（ms）
    uint16_t time;              // 更新周期(ms)
    TrackMode_t mode;
} TrackData_t;

extern TrackI2CStatus_t track_i2c_status;
bool Track_Init(void);
bool Track_GetData(TrackData_t *data);
bool Track_SetDigitalMode(void);


// GPIO引脚定义
#define TRACK_KEY_Port  GPIOG
#define TRACK_KEY_Pin   GPIO_PIN_13
#define TRACK_RST_Port  GPIOG
#define TRACK_RST_Pin   GPIO_PIN_14

// Shell 帮助信息
#define TRACK_MODE_HELP \
    "Usage: mode COMMAND\r\n" \
    "commands:\r\n" \
    "  cal      Enable calibration mode\r\n" \
    "  d        Send digital data\r\n" \
    "  stop     Stop sending data\r\n" \
    "  rst      Reset track module\r\n" \
    "  sta      Show track module status"

#define TRACK_TIME_HELP \
    "Usage: time [value] (ms)"

#define TRACK_CAL_HELP_1 \
    "Calibration Mode Started\r\n" \
    "1. When red light is on, place all sensors on black line for 3s, then press Enter"

#define TRACK_CAL_HELP_2 \
    "2. Place all sensors on white line for 3s, then press Enter"

#define TRACK_CAL_HELP_3 \
    "3. Check red light status:\r\n" \
    "- Red light off: Calibration success\r\n" \
    "- Red light slow blink: Need recalibration"

#ifdef __cplusplus
}
#endif

#endif /* __TRACK_H__ */
