/**
 * @file servo_port.h
 * @author (your name)
 * @brief 总线舵机驱动 FreeRTOS 适配层头文件
 */

#ifndef __SERVO_PORT_H__
#define __SERVO_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "fashion_star_uart_servo.h"
#include "cmsis_os.h"

/* 舵机串口配置 */
#define ServoUsart huart3

/* 舵机数量配置 */
#define SERVO_COUNT         6       // 实际使用的舵机数量

/* 舵机 ID 列表 (用户可根据实际修改) */
extern const uint8_t ServoIDList[SERVO_COUNT];

/**
 * @brief 舵机控制命令类型
 */
typedef enum {
    SERVO_CMD_SET_ANGLE = 0,        // 设置单圈角度
    SERVO_CMD_STOP,                 // 停止
} ServoCmdType_t;

/**
 * @brief 舵机控制命令结构体
 */
typedef struct {
    ServoCmdType_t cmdType;         // 命令类型
    uint8_t servoId;                // 目标舵机 ID
    float angle;                    // 角度值 (单圈: -180~180, 多圈: -368640~368640)
    float velocity;                 // 转速 (°/s)
    uint16_t interval;              // 运动时间 (ms) 或周期
    uint16_t t_acc;                 // 加速时间 (ms)
    uint16_t t_dec;                 // 减速时间 (ms)
    uint16_t power;                 // 功率 (mW) 或阻尼功率
} ServoCmd_t;

/**
 * @brief 舵机监控数据结构体
 */
typedef struct {
    uint8_t id;                     // 舵机 ID
    float angle;                    // 当前角度 (°)
    int16_t voltage;                // 电压 (mV)
    int16_t current;                // 电流 (mA)
    int16_t power;                  // 功率 (mW)
    float temperature;              // 温度 (°C)
    uint8_t status;                 // 状态标志
    int16_t circleCount;            // 当前圈数 (多圈模式)
} ServoData_t;

/* 队列句柄 */
extern osMessageQueueId_t Servo_DataHandle;
extern osMessageQueueId_t Servo_CmdHandle;

/* 初始化函数 */
void Servo_Init(void);

/* 任务函数 */
void Servo_Ctrl_Task(void *argument);

/* 舵机控制接口 */
bool Servo_SetAngle(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW);
bool Servo_Stop(uint8_t id);
void Servo_StopAll(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SERVO_PORT_H__ */