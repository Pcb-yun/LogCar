/**
 * @file servo_port.h
 * @brief 总线舵机驱动 FreeRTOS 适配层头文件
 */

#ifndef __SERVO_PORT_H__
#define __SERVO_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

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

#if SERVO_PING
#define SERVO_CMD_PING_HELP \
    "  ping       Ping servo\r\n"
#else
#define SERVO_CMD_PING_HELP ""
#endif

#if SERVO_DLC
#define SERVO_CMD_RESET_HELP \
    "  reset      Reset servo user data\r\n"
#else
#define SERVO_CMD_RESET_HELP ""
#endif

// 命令帮助信息
#define SERVO_CMD_HELP \
    SERVO_CMD_USAGE \
    SERVO_CMD_ANGLE_HELP \
    SERVO_CMD_STOP_HELP \
    SERVO_CMD_STOPALL_HELP \
    SERVO_CMD_PING_HELP \
    SERVO_CMD_RESET_HELP

/* 舵机数量配置 */
#define SERVO_COUNT         6

typedef enum {
    SERVO_CMD_SET_ANGLE_SHELL = 0,
    SERVO_CMD_STOP_SHELL = 1,
    SERVO_CMD_PING_SHELL = 2,
    SERVO_CMD_RESET_USER_DATA_SHELL = 3,
} ServoCmdType_t;

typedef struct {
    ServoCmdType_t cmdType;
    uint8_t servoId;
    float angle;
    float velocity;
    uint16_t interval;
    uint16_t t_acc;
    uint16_t t_dec;
    uint16_t power;
} ServoCmd_t;

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
extern osMessageQueueId_t Servo_CmdHandle;

void Servo_Init(void);
void Servo_Ctrl_Task(void *argument);

bool Servo_ANGLE(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW);
bool Servo_STOP(uint8_t id);
void Servo_STOP_ALL(void);

#if SERVO_PING
bool Servo_PING(uint8_t id);
#endif

#if SERVO_DLC
bool Servo_ResetData(uint8_t servo_id);
#endif


#ifdef __cplusplus
}
#endif

#endif