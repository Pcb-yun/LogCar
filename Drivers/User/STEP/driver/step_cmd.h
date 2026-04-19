/**
 * @file step_cmd.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 电机命令结构体定义头文件
 */

#ifndef __STEP_CMD_H__
#define __STEP_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "step_def.h"
#include "step_cfg.h"

/**
 * @brief 电机命令操作类型枚举
 */
typedef enum {
    OP_NONE = 0,      // 无操作
    OP_CONTROL,       // 控制操作
    OP_PARAM_READ,    // 参数读取
    OP_PARAM_WRITE,   // 参数写入
} MotorOpType_t;

/**
 * @brief 电机控制命令类型枚举
 */
typedef enum {
    CMD_NONE = 0,       // 无命令
    CMD_ENABLE,         // 使能控制
    CMD_VELOCITY,       // 速度控制
    CMD_POSITION,       // 位置控制
    CMD_TORQUE,         // 力矩控制
    CMD_STOP,           // 停止命令
    CMD_HOME,           // 回零命令
    CMD_SYNC,           // 同步执行
    CMD_FAST_SET,       // 快速位置模式 - 设定参数
    CMD_FAST_SEND,      // 快速位置模式 - 发送位置
} MotorCmdType_t;

/**
 * @brief 参数类型枚举
 */
typedef enum {
    PARAM_NONE = 0,      // 无参数
    PARAM_BASIC,         // 基本参数
    PARAM_CURRENT,       // 电流参数
    PARAM_PID,           // PID参数
    PARAM_PROTECT,       // 保护参数
    PARAM_COMM,          // 通信参数
    PARAM_BATCH_STATUS,  // 批量读取系统状态
    PARAM_BATCH_CONFIG,  // 批量读取驱动配置
} MotorParamType_t;

/**
 * @brief 控制命令子结构体
 */
typedef struct {
    MotorCmdType_t type;       // 控制命令类型

    // 子命令
    union {
#if MOTOR_CMD_ENABLE
        // 使能控制
        struct {
            bool enable;        // 使能状态：true-使能，false-不使能
            bool sync;          // 同步标志：true-等待同步触发，false-立即执行
        } en;
#endif

#if MOTOR_CMD_VELOCITY
        // 速度控制
        struct {
            uint8_t dir;            // 速度方向：0-CW，其他-CCW
            uint16_t vel;           // 速度（RPM）
            uint16_t acc;           // 加速度（RPM/S或档位）
            uint16_t max_current;   // 最大电流限制（mA）
        } vel;
#endif

#if MOTOR_CMD_POSITION
        // 位置控制
        struct {
            uint8_t dir;            // 位置方向：0-CW，其他-CCW
            uint16_t vel;           // 运动速度（RPM）
            uint16_t acc;           // 加速度
            int32_t target;         // 目标位置（Emm：脉冲数，X：角度）
            uint8_t mode;           // 运动模式：0-相对上一目标，1-绝对位置，2-相对当前位置

            #if CURRENT_FIRMWARE == FIRMWARE_X
            uint16_t dec;           // 减速度（X固件）
            uint16_t max_current;   // 最大电流限制（mA）
            #endif

            bool sync;              // 同步标志
        } pos;
#endif

#if MOTOR_CMD_TORQUE && CURRENT_FIRMWARE == FIRMWARE_X
        // 力矩控制
        struct {
            uint8_t dir;        // 力矩方向：0-CW，其他-CCW
            uint16_t slope;     // 电流斜率（mA/S）
            uint16_t current;   // 目标电流（mA）
            uint16_t max_vel;   // 最大速度限制（RPM）
        } torque;
#endif

#if MOTOR_CMD_STOP
        // 停止控制
        struct {
            bool stop;     // 停止同步标志
        } stop;
#endif

#if MOTOR_CMD_HOME
        // 回零控制
        struct {
            uint8_t mode;           // 回零模式：0-单圈就近，1-单圈方向，2-无限位碰撞，3-限位回零
            uint8_t dir;            // 回零方向：0-CW，1-CCW
            uint16_t vel;           // 回零速度（RPM）
            uint32_t timeout;       // 回零超时时间（ms）
            uint16_t sl_vel;        // 碰撞检测转速（RPM）
            uint16_t sl_current;    // 碰撞检测电流（mA）
            uint16_t sl_time;       // 碰撞检测时间（ms）
            bool auto_home;              // 上电自动回零：true-使能，false-禁用
            bool sync_flag;         // 同步标志：true-等待同步触发，false-立即执行
        } home;
#endif

#if MOTOR_CMD_FAST
        // 快速位置模式 - 设置参数
        struct {
        #if CURRENT_FIRMWARE == FIRMWARE_EMM
            uint16_t vel;           // 速度（RPM）
            uint8_t acc;            // 加速度（档位）
        #elif CURRENT_FIRMWARE == FIRMWARE_X
            uint16_t acc;           // 加速加速度（RPM/S）
            uint16_t dec;           // 减速加速度（RPM/S）
            uint16_t vel;           // 最大速度（RPM）
            uint16_t max_current;   // 最大电流（mA）
        #endif
            uint8_t mode;           // 运动模式：0-相对上一目标，1-绝对位置，2-相对当前位置
            bool sync;              // 同步标志：true-立即执行，false-等待同步触发
        } fast_set;

        // 快速位置模式 - 发送位置
        struct {
            int32_t pos;        // 目标位置（Emm：脉冲数，X：角度）
        } fast_send;
#endif
    } p;
} MotorCtrl_t;

/**
 * @brief 参数命令子结构体
 */
typedef struct {
    MotorParamType_t type;        // 参数类型

    // 子命令
    union {
#if MOTOR_PARAM_BASIC
        // 基本参数
        struct {
            uint8_t micro_step;    // 细分值
            uint8_t motor_type;    // 电机类型：0-1.8°，1-0.9°
            uint8_t firmware;      // 固件类型：0-X固件，1-Emm固件
            uint8_t ctrl_mode;     // 控制模式：0-开环，1-闭环FOC
            uint8_t dir;           // 电机正方向：0-CW，其他-CCW
            bool save;             // 存储标志：true-存储，false-不存储
        } basic;
#endif

#if MOTOR_PARAM_CURRENT
        // 电流参数
        struct {
            uint16_t open_current;      // 开环工作电流（mA）
            uint16_t close_current;     // 闭环最大电流（mA）
            bool save;                 // 存储标志：true-存储，false-不存储
        } current;
#endif

#if MOTOR_PARAM_PID
        // PID参数
        struct {
            uint32_t kp;                // PID比例系数
            uint32_t ki;                // PID积分系数
            uint32_t kd;                // PID微分系数
            uint32_t integral_limit;    // 积分限幅/刚性系数
            bool save;                  // 存储标志：true-存储，false-不存储
        } pid;
#endif

#if MOTOR_PARAM_PROTECT
        // 保护参数
        struct {
            uint16_t temp_threshold;        // 过热保护阈值（℃）
            uint16_t current_threshold;     // 过流保护阈值（mA）
            uint16_t protect_time;          // 保护检测时间（ms）
            uint32_t heartbeat_time;        // 心跳保护时间（ms）
            uint16_t pos_window;            // 位置到达窗口
            uint16_t collision_angle;       // 碰撞回零角度（0.1°）
            bool save;                      // 存储标志：true-存储，false-不存储
        } protect;
#endif

#if MOTOR_PARAM_COMM
        // 通信参数
        struct {
            uint8_t uart_baudrate;      // 串口波特率
            uint8_t can_baudrate;       // CAN波特率
            uint8_t verify_mode;        // 校验方式
            uint8_t response_mode;      // 应答方式
            bool save;                  // 存储标志：true-存储，false-不存储
        } comm;
#endif
    } p;
} MotorParam_t;

/**
 * @brief 电机命令结构体
 */
typedef struct {
    MotorOpType_t op_type;      // 操作类型
    uint8_t motor_id;           // 电机ID

    // 操作类型
    union {
        MotorCtrl_t ctrl;       // 控制命令
        MotorParam_t param;     // 参数命令
    } type;
} MotorCmd_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_CMD_H__ */