/**
 * @file step_cfg.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机用户配置头文件
 */

#ifndef __STEP_CFG_H__
#define __STEP_CFG_H__

#include "step_def.h"

/** 固件版本选择：
 *  FIRMWARE_EMM
 *  FIRMWARE_X
 */
#define CURRENT_FIRMWARE         FIRMWARE_EMM

/** 电机型号选择：
 *  MOTOR_MODEL_X42S
 *  MOTOR_MODEL_Y42
 */
#define CURRENT_MOTOR_MODEL      MOTOR_MODEL_X42S

// 是否使用心跳保护
#define USE_HEARTBEAT 1

// 是否使用查看功能
#define USE_VIEW 1


/******************** 系统状态参数 *********************/
#define MOTOR_STATUS_READ_BATCH             0   // 批量读取系统状态参数
#if MOTOR_STATUS_READ_BATCH
#if CURRENT_FIRMWARE == FIRMWARE_X
#define MOTOR_STATUS_ENCODER_RAW            1   // 读取编码器原始值
#endif
#endif

#define MOTOR_STATUS_BUS_VOLTAGE            1   // 读取总线电压
#define MOTOR_STATUS_PHASE_CURRENT          1   // 读取相电流
#define MOTOR_STATUS_ENCODER_VALUE          1   // 读取线性化编码器值
#define MOTOR_STATUS_TARGET_POS             1   // 读取电机目标位置
#define MOTOR_STATUS_SPEED                  1   // 读取电机实时转速
#define MOTOR_STATUS_REAL_POS               1   // 读取电机实时位置
#define MOTOR_STATUS_POS_ERROR              1   // 读取电机位置误差
#define MOTOR_STATUS_MOTOR_FLAGS            1   // 读取电机状态标志
#define MOTOR_STATUS_HOME_FLAGS             1   // 读取回零状态标志

#if CURRENT_FIRMWARE == FIRMWARE_X
#define MOTOR_STATUS_BUS_CURRENT            1   // 读取总线电流
#endif

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_STATUS_TEMPERATURE            1   // 读取驱动温度
#define MOTOR_STATUS_FLAGS_COMBINED         1   // 读取回零+电机状态标志
#endif

#define MOTOR_STATUS_INPUT_PULSES           1   // 读取输入脉冲数
#define MOTOR_STATUS_SET_POS                1   // 读取电机实时设定目标位置

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_STATUS_PIN_STATUS             1   // 读取引脚IO电平状态
#endif

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_STATUS_BATTERY_VOLTAGE        1   // 读取电池电压
#endif


/******************** 驱动配置参数 *********************/
#define MOTOR_DRIVER_CONFIG_READ_BATCH      1   // 批量读取驱动配置参数
#if MOTOR_DRIVER_CONFIG_READ_BATCH
#define MOTOR_DRIVER_LOCK_KEY               1   // 锁定按键功能
#define MOTOR_DRIVER_CONTROL_MODE           1   // 控制模式（开环/闭环）
#define MOTOR_DRIVER_MICRO_STEP             1   // 细分值
#define MOTOR_DRIVER_OPENLOOP_CURRENT       1   // 开环模式工作电流
#define MOTOR_DRIVER_CLOSEDLOOP_CURRENT     1   // 闭环模式最大电流
#define MOTOR_DRIVER_BAUDRATE               1   // 波特率
#define MOTOR_DRIVER_CAN_RATE               1   // CAN速率
#define MOTOR_DRIVER_STALL_PROTECT          1   // 堵转保护
#define MOTOR_DRIVER_POS_ARRIVE             1   // 位置到达检测
#define MOTOR_DRIVER_STALL_SPEED            1   // 堵转保护转速
#define MOTOR_DRIVER_STALL_CURRENT          1   // 堵转保护电流
#define MOTOR_DRIVER_STALL_TIME             1   // 堵转保护延时
#define MOTOR_DRIVER_PULSE_THRESHOLD        1   // 脉冲阈值
#define MOTOR_DRIVER_FIRMWARE_TYPE          1   // 固件类型
#define MOTOR_DRIVER_MOTOR_TYPE             1   // 电机类型
#define MOTOR_DRIVER_DIRECTION              1   // 电机运动正方向
#define MOTOR_DRIVER_POWER_FLAG             1   // 掉电标志
#define MOTOR_DRIVER_HOME_SPEED             1   // 回零速度
#define MOTOR_DRIVER_HOME_MODE              1   // 回零模式
#define MOTOR_DRIVER_HOME_DIR               1   // 回零方向
#define MOTOR_DRIVER_HOME_TIMEOUT           1   // 回零超时
#define MOTOR_DRIVER_INTEGRAL_LIMIT         1   // 积分限幅/刚性系数
#endif

#define MOTOR_DRIVER_POS_WINDOW             1   // 位置到达窗口


/******************** 控制参数读写 *********************/
#define MOTOR_PID_READ                      1   // 读取PID参数
#define MOTOR_PID_WRITE                     1   // 修改PID参数
#define MOTOR_HOME_READ                     1   // 读取回零参数
#define MOTOR_HOME_WRITE                    1   // 修改回零参数

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_INTEGRAL_LIMIT_WRITE          1   // 修改积分限幅/刚性系数
#define MOTOR_PROTECT_THRESHOLD_READ        1   // 读取过热过流保护阈值
#define MOTOR_PROTECT_THRESHOLD_WRITE       1   // 修改过热过流保护阈值
#define MOTOR_COLLISION_ANGLE_READ          1   // 读取碰撞回零返回角度
#define MOTOR_COLLISION_ANGLE_WRITE         1   // 修改碰撞回零返回角度

#if USE_HEARTBEAT
#define MOTOR_HEARTBEAT_READ                1   // 读取心跳保护时间
#define MOTOR_HEARTBEAT_WRITE               1   // 修改心跳保护时间
#endif

#endif /* CURRENT_MOTOR_MODEL */


/******************** 设备信息与特殊功能 *********************/
#define MOTOR_READ_VERSION                  1   // 读取固件版本和硬件版本
#define MOTOR_READ_PHASE_PARAMS             1   // 读取相电阻和相电感

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_READ_OPTION_PARAMS            1   // 读取选项参数状态
#define MOTOR_BROADCAST_READ_ID             1   // 广播读取ID地址
#endif


/******************** 运动控制命令 *********************/
#define MOTOR_CMD_ENABLE                    1   // 电机使能控制
#define MOTOR_CMD_STOP                      1   // 立即停止
#define MOTOR_POS_MODE_FAST                 1   // 快速位置模式
#define MOTOR_POS_MODE_TRAPEZOIDAL          1   // 位置模式
#define MOTOR_VELOCITY_MODE                 1   // 速度模式

#if CURRENT_FIRMWARE == FIRMWARE_X
#define MOTOR_POS_MODE_DIRECT               1   // 直通限速位置模式
#define MOTOR_TORQUE_MODE                   1   // 力矩模式

#define MOTOR_POS_MODE_DIRECT_LIMIT         1   // 直通限速位置模式（+最大电流限制）
#define MOTOR_TORQUE_MODE_LIMIT             1   // 力矩模式（+最大速度限制）
#define MOTOR_POS_MODE_TRAPEZOIDAL_LIMIT    1   // 位置模式（+最大电流限制）
#define MOTOR_VELOCITY_MODE_LIMIT           1   // 速度模式（+最大电流限制）
#endif /* CURRENT_FIRMWARE */

#define MOTOR_SYNC_TRIGGER                  1   // 触发多机同步运动

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_MULTI_CMD                     1   // 多电机命令
#endif


/******************** 触发动作命令 *********************/
#define MOTOR_TRIGGER_ENCODER_CALIB         1   // 触发编码器校准
#define MOTOR_TRIGGER_RESET_POS             1   // 当前位置角度清零
#define MOTOR_TRIGGER_CLEAR_PROTECT         1   // 解除堵转/过热/过流保护
#define MOTOR_TRIGGER_FACTORY_RESET         1   // 恢复出厂设置

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_TRIGGER_RESTART               1   // 重启电机
#endif


/******************** 原点回零命令 *********************/
#define MOTOR_HOME_SET_ZERO                 1   // 设置单圈回零零点位置
#define MOTOR_HOME_TRIGGER                  1   // 触发回零
#define MOTOR_HOME_INTERRUPT                1   // 强制中断回零


/******************** 电机参数设置 *********************/
#define MOTOR_SET_MOTOR_ID                  1   // 修改电机ID/地址
#define MOTOR_SET_MICRO_STEP                1   // 修改细分值
#define MOTOR_SET_POWER_FLAG                1   // 修改掉电标志
#define MOTOR_SET_MOTOR_TYPE                1   // 修改电机类型
#define MOTOR_SET_FIRMWARE_TYPE             1   // 修改固件类型
#define MOTOR_SET_OPENLOOP_CURRENT          1   // 修改开环模式工作电流
#define MOTOR_SET_CLOSEDLOOP_CURRENT        1   // 修改闭环模式最大电流
#define MOTOR_SET_CONTROL_MODE              1   // 修改开环/闭环控制模式
#define MOTOR_SET_DIRECTION                 1   // 修改电机运动正方向
#define MOTOR_SET_LOCK_KEY                  1   // 修改锁定按键功能
#define MOTOR_SET_SCALE_INPUT               1   // 修改缩小倍数输入
#define MOTOR_SET_DRIVER_CONFIG_ALL         1   // 修改驱动配置参数（批量）

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_SET_LOCK_PARAMS               1   // 修改锁定修改参数功能
#define MOTOR_POS_WINDOW_WRITE              1   // 修改位置到达窗口
#define MOTOR_DMX512_WRITE                  1   // 修改DMX512协议参数
#define MOTOR_PERIODIC_RETURN               1   // 定时返回信息
#endif

#endif /* __STEP_CFG_H__ */
