/**
 * @file step_cfg.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机用户配置头文件
 */

#ifndef __STEP_CFG_H__
#define __STEP_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CURRENT_FIRMWARE         FIRMWARE_EMM       // 当前使用的固件版本
#define CURRENT_MOTOR_MODEL      MOTOR_MODEL_X42S   // 当前使用的电机型号

#define PACKET_TIMEOUT 100      // 多包数据超时时间，单位：ms


/**
 * @brief 电机状态结构体配置宏
 */
#define MOTOR_STATUS_ELECTRICAL   1      // 电气参数
#define MOTOR_STATUS_MOTION       1      // 运动状态
#define MOTOR_STATUS_ENCODER      1      // 编码器信息
#define MOTOR_STATUS_STATUS       1      // 状态标志
#define MOTOR_STATUS_SYSTEM       1      // 系统信息
#define MOTOR_STATUS_CONTROL      1      // 控制参数
#define MOTOR_STATUS_PROTECTION   1      // 保护参数
#define MOTOR_STATUS_BATCH        1      // 批量参数
#define MOTOR_STATUS_COMM         1      // 通讯参数

/**
 * @brief 电机控制命令结构体配置宏
 */
#define MOTOR_CMD_ENABLE         1      // 使能控制命令
#define MOTOR_CMD_VELOCITY       1      // 速度模式命令
#define MOTOR_CMD_POSITION       1      // 位置模式命令
#define MOTOR_CMD_TORQUE         1      // 力矩模式命令
#define MOTOR_CMD_STOP           1      // 停止命令
#define MOTOR_CMD_HOME           1      // 回零命令
#define MOTOR_CMD_FAST           1      // 快速位置模式命令

/**
 * @brief 电机参数配置结构体配置宏
 */
#define MOTOR_PARAM_BASIC       1      // 基本参数
#define MOTOR_PARAM_CURRENT     1      // 电流参数
#define MOTOR_PARAM_PID         1      // PID参数
#define MOTOR_PARAM_PROTECT     1      // 保护参数
#define MOTOR_PARAM_COMM        1      // 通信参数



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_CFG_H__ */
