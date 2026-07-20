/**
 * @file servo_port.h
 * @author MIKE
 * @brief 总线舵机驱动 FreeRTOS 适配层头文件
 */

#ifndef __SERVO_PORT_H__
#define __SERVO_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "servo_driver.h"
#include "cmsis_os2.h"

// 基础使用说明
#define SERVO_CMD_USAGE \
    "Usage: servo COMMAND\r\n" \
    "\r\n" \
    "commands:\r\n"

// 各个命令的辅助宏
#define SERVO_CMD_ANGLE_HELP \
    "  angle      Set servo angle\r\n"

#define SERVO_CMD_STOP_HELP \
    "  stop       Stop servo\r\n"

#define SERVO_CMD_STOPALL_HELP \
    "  stopall    Stop all servos\r\n"

#define SERVO_CMD_MTURN_HELP \
    "  mturn      Set servo mturn\r\n"

#if SERVO_PING
#define SERVO_CMD_PING_HELP \
    "  ping       Ping servo\r\n"
#else
#define SERVO_CMD_PING_HELP ""
#endif

#if SERVO_ADVANCED_MODE
#define SERVO_CMD_ANGLE_BY_VELOCITY_HELP \
    "  angle_vel  Set servo angle by velocity\r\n"

#define SERVO_CMD_ANGLE_BY_INTERVAL_HELP \
    "  angle_int  Set servo angle by interval\r\n"

#define SERVO_CMD_MTURN_BY_VELOCITY_HELP \
    "  mturn_vel  Set servo mturn by velocity\r\n"

#define SERVO_CMD_MTURN_BY_INTERVAL_HELP \
    "  mturn_int  Set servo mturn by interval\r\n"
#else
#define SERVO_CMD_ANGLE_BY_VELOCITY_HELP ""
#define SERVO_CMD_ANGLE_BY_INTERVAL_HELP ""
#define SERVO_CMD_MTURN_BY_VELOCITY_HELP ""
#define SERVO_CMD_MTURN_BY_INTERVAL_HELP ""
#endif

#if SERVO_MONITOR
#define SERVO_CMD_MONITOR_HELP \
    "  monitor    Monitor servo status\r\n"
#else
#define SERVO_CMD_MONITOR_HELP ""
#endif



// 命令帮助信息
#define SERVO_CMD_HELP \
    SERVO_CMD_USAGE \
    SERVO_CMD_ANGLE_HELP \
    SERVO_CMD_MTURN_HELP \
    SERVO_CMD_STOP_HELP \
    SERVO_CMD_STOPALL_HELP \
    SERVO_CMD_PING_HELP \
    SERVO_CMD_ANGLE_BY_VELOCITY_HELP \
    SERVO_CMD_ANGLE_BY_INTERVAL_HELP \
    SERVO_CMD_MTURN_BY_VELOCITY_HELP \
    SERVO_CMD_MTURN_BY_INTERVAL_HELP \
    SERVO_CMD_MONITOR_HELP

/* 舵机数量配置 */
#define SERVO_COUNT         SERVO_MAX_COUNT

typedef enum {
    SERVO_CMD_SET_ANGLE_SHELL = 0,
    SERVO_CMD_STOP_SHELL,
    SERVO_CMD_SET_MTURN_SHELL,
    SERVO_CMD_PING_SHELL,
    SERVO_CMD_SET_ANGLE_BY_INTERVAL_SHELL,
    SERVO_CMD_SET_ANGLE_BY_VELOCITY_SHELL,
    SERVO_CMD_SET_MTURN_ANGLE_BY_VELOCITY_SHELL,
    SERVO_CMD_SET_MTURN_ANGLE_BY_INTERVAL_SHELL,

} ServoCmdType_t;

typedef struct {
    uint8_t id;
    float angle;
    int16_t voltage;
    int16_t current;
    int16_t power;
    float temperature;
    uint8_t status;
    int16_t circleCount;
} ServoData_t;

extern osMessageQueueId_t Servo_DataHandle;

void Servo_ANGLE(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW);
void Servo_MTURN(uint8_t id, float angle,uint32_t interval, uint16_t power);
void Servo_STOP(uint8_t id);
void Servo_STOP_ALL(void);

#if SERVO_MONITOR
void Servo_MONITOR(uint8_t servo_id, ServoData servodata[]);
#endif

#if SERVO_PING
void Servo_PING(uint8_t id);
#endif

#if SERVO_ADVANCED_MODE
void Servo_AngleByInterval(uint8_t servo_id, float angle, 
                                            uint16_t interval, uint16_t t_acc,
                                            uint16_t t_dec, uint16_t power);
void Servo_AngleByVelocity(uint8_t servo_id, float angle, 
                                            float velocity, uint16_t t_acc,
                                            uint16_t t_dec, uint16_t power);
void Servo_MTurnByVelocity(uint8_t servo_id, float angle,
                                                 float velocity, uint16_t t_acc,
                                                 uint16_t t_dec, uint16_t power);
void Servo_MTurnByInterval(uint8_t servo_id, float angle,
                                                 uint32_t interval, uint16_t t_acc,
                                                 uint16_t t_dec, uint16_t power);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __SERVO_PORT_H__ */
