/**
 * @file servo_driver.h
 * @brief Fashion Star总线伺服舵机FreeRTOS驱动层
 */

#ifndef __SERVO_DRIVER_H__
#define __SERVO_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "freertos.h"

#define SERVO_UART_RECV_BUF_SIZE 500
#define SERVO_UART_SEND_BUF_SIZE 500

#define FSUS_TIMEOUT_MS 100

#define FSUS_PACK_REQUEST_HEADER 0x4c12
#define FSUS_PACK_RESPONSE_HEADER 0x1c05
#define FSUS_PACK_RESPONSE_MAX_SIZE 350

#define FSUS_CMD_NUM 30
#define FSUS_CMD_PING 1
#define FSUS_CMD_RESET_USER_DATA 2
#define FSUS_CMD_READ_DATA 3
#define FSUS_CMD_WRITE_DATA 4
#define FSUS_CMD_READ_BATCH_DATA 5
#define FSUS_CMD_WRITE_BATCH_DATA 6
#define FSUS_CMD_SPIN 7
#define FSUS_CMD_ROTATE 8
#define FSUS_CMD_DAMPING 9
#define FSUS_CMD_READ_ANGLE 10
#define FSUS_CMD_SET_SERVO_ANGLE_BY_INTERVAL 11
#define FSUS_CMD_SET_SERVO_ANGLE_BY_VELOCITY 12
#define FSUS_CMD_SET_SERVO_ANGLE_MTURN 13
#define FSUS_CMD_SET_SERVO_ANGLE_MTURN_BY_INTERVAL 14
#define FSUS_CMD_SET_SERVO_ANGLE_MTURN_BY_VELOCITY 15
#define FSUS_CMD_QUERY_SERVO_ANGLE_MTURN 16
#define FSUS_CMD_RESERT_SERVO_ANGLE_MTURN 17
#define FSUS_CMD_BEGIN_ASYNC 18
#define FSUS_CMD_END_ASYNC 19
#define FSUS_CMD_SET_SERVO_ReadData 22
#define FSUS_CMD_SET_ORIGIN_POINT 23
#define FSUS_CMD_CONTROL_MODE_STOP 24
#define FSUS_CMD_SET_SERVO_SyncCommand 25

#define FSUS_STATUS uint8_t
#define FSUS_STATUS_SUCCESS 0
#define FSUS_STATUS_FAIL 1
#define FSUS_STATUS_TIMEOUT 2
#define FSUS_STATUS_WRONG_RESPONSE_HEADER 3
#define FSUS_STATUS_UNKOWN_CMD_ID 4
#define FSUS_STATUS_SIZE_TOO_BIG 5
#define FSUS_STATUS_CHECKSUM_ERROR 6
#define FSUS_STATUS_ID_NOT_MATCH 7
#define FSUS_STATUS_ERRO 8

#define FSUS_ANGLE_DEADAREA 2.0f
#define FSUS_WAIT_COUNT_MAX 10000

#define FSUS_PARAM_VOLTAGE 1
#define FSUS_PARAM_CURRENT 2
#define FSUS_PARAM_POWER 3
#define FSUS_PARAM_TEMPRATURE 4
#define FSUS_PARAM_SERVO_STATUS 5
#define FSUS_PARAM_RESPONSE_SWITCH 33
#define FSUS_PARAM_SERVO_ID 34
#define FSUS_PARAM_BAUDRATE 36
#define FSUS_PARAM_STALL_PROTECT 37
#define FSUS_PARAM_STALL_POWER_LIMIT 38
#define FSUS_PARAM_OVER_VOLT_LOW 39
#define FSUS_PARAM_OVER_VOLT_HIGH 40
#define FSUS_PARAM_OVER_TEMPERATURE 41
#define FSUS_PARAM_OVER_POWER 42
#define FSUS_PARAM_OVER_CURRENT 43
#define FSUS_PARAM_ACCEL_SWITCH 44
#define FSUS_PARAM_POWER_ON_LOCK_SWITCH 46
#define FSUS_PARAM_WHEEL_MODE_BRAKE_SWITCH 47
#define FSUS_PARAM_ANGLE_LIMIT_SWITCH 48
#define FSUS_PARAM_SOFT_START_SWITCH 49
#define FSUS_PARAM_SOFT_START_TIME 50
#define FSUS_PARAM_ANGLE_LIMIT_HIGH 51
#define FSUS_PARAM_ANGLE_LIMIT_LOW 52
#define FSUS_PARAM_ANGLE_MID_OFFSET 53

#define FSUS_RECV_FLAG_HEADER 0x01
#define FSUS_RECV_FLAG_CMD_ID 0x02
#define FSUS_RECV_FLAG_SIZE 0x04
#define FSUS_RECV_FLAG_CONTENT 0x08
#define FSUS_RECV_FLAG_CHECKSUM 0x10

extern osSemaphoreId_t ServoUartRxSemHandle;
extern UART_HandleTypeDef huart3;

typedef struct {
    uint16_t header;
    uint8_t cmdId;
    uint16_t size;
    uint8_t content[FSUS_PACK_RESPONSE_MAX_SIZE];
    uint8_t checksum;
    uint8_t status;
    uint8_t isSync;
} PackageTypeDef;

typedef struct {
    uint8_t id;
    int16_t voltage;
    int16_t current;
    int16_t power;
    int16_t temperature;
    uint8_t status;
    float angle;
    int16_t circle_count;
} ServoData;

typedef struct {
    uint8_t id;
    float angle;
    float velocity;
    uint16_t interval_single;
    uint32_t interval_multi;
    uint16_t t_acc;
    uint16_t t_dec;
    uint16_t power;
} FSUS_sync_servo;

typedef enum {
    MODE_SET_SERVO_ANGLE = 1,
    MODE_SET_SERVO_ANGLE_BY_INTERVAL = 2,
    MODE_SET_SERVO_ANGLE_BY_VELOCITY = 3,
    MODE_SET_SERVO_ANGLE_MTURN = 4,
    MODE_SET_SERVO_ANGLE_MTURN_BY_INTERVAL = 5,
    MODE_SET_SERVO_ANGLE_MTURN_BY_VELOCITY = 6,
    MODE_Query_SERVO_Monitor = 7
} ServoMode;

extern FSUS_sync_servo SyncArray[20];
extern ServoData servodata[20];

void ServoDriver_Init(void);

void RingBuffer_Init(uint16_t capacity);
void RingBuffer_Reset(void);
uint16_t RingBuffer_GetByteUsed(void);
uint8_t RingBuffer_ReadByte(void);
void RingBuffer_ReadByteArray(uint8_t* dest, uint16_t size);
void RingBuffer_Push(uint8_t value);
void RingBuffer_WriteByte(uint8_t value);
void RingBuffer_WriteByteArray(uint8_t* src, uint16_t size);
void RingBuffer_WriteShort(int16_t value);
void RingBuffer_WriteUShort(uint16_t value);
void RingBuffer_WriteLong(int32_t value);
void RingBuffer_WriteULong(uint32_t value);
uint8_t RingBuffer_PeekByte(uint16_t index);
uint16_t RingBuffer_PeekUShort(uint16_t index);
uint8_t RingBuffer_GetChecksum(void);
uint16_t RingBuffer_ReadUShort(void);

void Servo_Uart_Send(uint8_t* data, uint16_t size);
void Servo_Uart_Receive_IT(uint8_t* data, uint16_t size);
void Servo_Uart_RxCpltCallback(void);

void FSUS_Package2RingBuffer(PackageTypeDef *pkg);
uint8_t FSUS_CalcChecksum(PackageTypeDef *pkg);
FSUS_STATUS FSUS_IsValidResponsePackage(PackageTypeDef *pkg);
void FSUS_SendPackage_Common(uint8_t cmdId, uint16_t size, uint8_t *content, uint8_t isSync);
FSUS_STATUS FSUS_RecvPackage(PackageTypeDef *pkg);
FSUS_STATUS FSUS_sync_RecvPackage(PackageTypeDef *pkg);

FSUS_STATUS FSUS_Ping(uint8_t servo_id);
FSUS_STATUS FSUS_ResetUserData(uint8_t servo_id);
FSUS_STATUS FSUS_ReadData(uint8_t servo_id, uint8_t address, uint8_t *value, uint8_t *size);
FSUS_STATUS FSUS_WriteData(uint8_t servo_id, uint8_t address, uint8_t *value, uint8_t size);
FSUS_STATUS FSUS_SetServoAngle(uint8_t servo_id, float angle, uint16_t interval, uint16_t power);
FSUS_STATUS FSUS_SetServoAngleByInterval(uint8_t servo_id, float angle, uint16_t interval,
                                          uint16_t t_acc, uint16_t t_dec, uint16_t power);
FSUS_STATUS FSUS_SetServoAngleByVelocity(uint8_t servo_id, float angle, float velocity,
                                           uint16_t t_acc, uint16_t t_dec, uint16_t power);
FSUS_STATUS FSUS_QueryServoAngle(uint8_t servo_id, float *angle);
FSUS_STATUS FSUS_SetServoAngleMTurn(uint8_t servo_id, float angle, uint32_t interval, uint16_t power);
FSUS_STATUS FSUS_SetServoAngleMTurnByInterval(uint8_t servo_id, float angle, uint32_t interval,
                                                 uint16_t t_acc, uint16_t t_dec, uint16_t power);
FSUS_STATUS FSUS_SetServoAngleMTurnByVelocity(uint8_t servo_id, float angle, float velocity,
                                                 uint16_t t_acc, uint16_t t_dec, uint16_t power);
FSUS_STATUS FSUS_QueryServoAngleMTurn(uint8_t servo_id, float *angle);
FSUS_STATUS FSUS_DampingMode(uint8_t servo_id, uint16_t power);
FSUS_STATUS FSUS_ServoAngleReset(uint8_t servo_id);
FSUS_STATUS FSUS_SetOriginPoint(uint8_t servo_id);
FSUS_STATUS FSUS_BeginAsync(void);
FSUS_STATUS FSUS_EndAsync(uint8_t mode);
FSUS_STATUS FSUS_ServoMonitor(uint8_t servo_id, ServoData servodata[]);
FSUS_STATUS FSUS_StopOnControlMode(uint8_t servo_id, uint8_t mode, uint16_t power);
FSUS_STATUS FSUS_SyncServoMonitor(uint8_t servo_count, ServoData servodata[]);
FSUS_STATUS FSUS_SyncCommand(uint8_t servo_count, uint8_t ServoMode, FSUS_sync_servo servoSync[]);

#ifdef __cplusplus
}
#endif

#endif
