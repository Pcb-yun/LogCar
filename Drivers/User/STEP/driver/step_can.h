/**
 * @file step_can.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief CAN处理逻辑层头文件
 */

#ifndef __STEP_CAN_H__
#define __STEP_CAN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "can.h"
#include "step_cmd.h"

/**
 * @brief 电机状态结构体
 */
typedef struct {
    uint8_t motor_id;

#if MOTOR_STATUS_ELECTRICAL
    uint16_t voltage;
    uint16_t current;
    uint16_t phase_current;
    uint8_t temp;
    #if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
    uint16_t battery_voltage;
    #endif
#endif

#if MOTOR_STATUS_MOTION
    int16_t vel;
    int32_t pos;
    int32_t target_pos;
    int32_t set_pos;
    int32_t pos_error;
    int32_t input_pulses;
#endif

#if MOTOR_STATUS_ENCODER
    uint16_t encoder_value;
#endif

#if MOTOR_STATUS_STATUS
    uint8_t status;
    uint8_t home_status;
    uint8_t pin_status;
#endif

#if MOTOR_STATUS_SYSTEM
    uint16_t firmware_version;
    uint8_t hardware_version;
    uint16_t phase_resistance;
    uint16_t phase_inductance;
    uint8_t option_params;
#endif

#if MOTOR_STATUS_CONTROL
    uint32_t kp;
    uint32_t ki;
    uint32_t kd;
    uint16_t pos_window;
    uint32_t integral_limit;
#endif

#if MOTOR_STATUS_PROTECTION
    uint16_t temp_threshold;
    uint16_t current_threshold;
    uint32_t heartbeat_time;
    uint16_t collision_angle;
#endif

#if MOTOR_STATUS_BATCH
    uint8_t micro_step;
    uint16_t open_current;
    uint16_t close_current;
#endif

#if MOTOR_STATUS_COMM
    uint8_t uart_baudrate;
    uint8_t can_baudrate;
    uint8_t verify_mode;
    uint8_t response_mode;
#endif
} MotorStatus_t;

typedef struct {
    MotorStatus_t motors[4];
    bool is_init;
} MotorStatusShared_t;

extern MotorStatusShared_t g_motor_status;

bool process_multi_packet(CAN_Rx_Message_t *msg);
void Motor_CAN_Send(uint8_t *cmd, uint8_t len);
void Motor_Process_Cmd(MotorCmd_t *cmd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_CAN_H__ */
