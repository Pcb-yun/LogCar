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

/* 舵机数量配置 */
#define SERVO_COUNT         6       // 实际使用的舵机数量
#define SERVO_MONITOR_PERIOD 50     // 监控任务周期 (ms)

/* 舵机 ID 列表 (用户可根据实际修改) */
extern const uint8_t ServoIDList[SERVO_COUNT];

/* 串口对象 */
extern UART_HandleTypeDef huart2;
#define ServoUsart huart2

/**
 * @brief 舵机控制命令类型
 */
typedef enum {
    SERVO_CMD_SET_ANGLE = 0,        // 设置单圈角度
    SERVO_CMD_SET_ANGLE_MTURN,      // 设置多圈角度
    SERVO_CMD_SET_ANGLE_BY_VELOCITY,// 指定转速设置角度
    SERVO_CMD_DAMPING,              // 阻尼模式
    SERVO_CMD_STOP,                 // 停止
    SERVO_CMD_RESET_ORIGIN,         // 设置零点
    SERVO_CMD_RESET_ANGLE,          // 重置多圈圈数
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

/* 队列句柄 (外部可用) */
extern osMessageQueueId_t Servo_DataHandle;
extern osMessageQueueId_t Servo_CmdHandle;

/* 初始化函数 */
void Servo_Init(void);

/* 任务函数 */
void Servo_Control_Task(void *argument);
void Servo_Monitor_Task(void *argument);

/* 舵机控制接口 */
bool Servo_SetAngle(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW);
bool Servo_SetAngleMultiTurn(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW);
bool Servo_SetAngleByVelocity(uint8_t id, float angle, int16_t velocity_rpm, uint16_t t_acc_ms, uint16_t t_dec_ms, uint16_t power_mW);
bool Servo_SetDampingMode(uint8_t id, uint16_t power_mW);
bool Servo_Stop(uint8_t id);
bool Servo_SetOrigin(uint8_t id);
bool Servo_ResetMultiTurnAngle(uint8_t id);
bool Servo_GetMonitorData(uint8_t id, ServoData_t *outData);
bool Servo_GetAllMonitorData(ServoData_t *outArray, uint8_t maxCount);
void Servo_SetMultiAngles(const uint8_t *ids, const float *angles, uint8_t count, uint16_t interval_ms, uint16_t power_mW);
void Servo_StopAll(void);
void Servo_WaitForStop(uint8_t id, uint32_t timeout_ms);
void Servo_SmoothMove(uint8_t id, float targetAngle, uint16_t totalTimeMs, uint8_t segments, uint16_t power_mW);
bool Servo_IsAlive(uint8_t id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SERVO_PORT_H__ */