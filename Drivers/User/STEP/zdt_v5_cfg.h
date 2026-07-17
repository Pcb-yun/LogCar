/**
 * @file zdt_v5_cfg.h
 * @brief 张大头V5步进电机用户配置
 *
 * zdt-v5-driver - 张大头V5步进电机通用驱动
 * Copyright (C) 2024-2026  Pcb-yun
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * 将本文件复制到工程中，并改名为 zdt_v5_cfg.h
 */

#ifndef __ZDT_V5_CFG_H__
#define __ZDT_V5_CFG_H__

#include "zdt_v5_proto.h"

/** 固件版本选择：
 *  FIRMWARE_EMM
 *  FIRMWARE_X
 */
#define CURRENT_FIRMWARE         FIRMWARE_EMM

/** 电机型号选择：
 *  MOTOR_MODEL_X42
 *  MOTOR_MODEL_Y42
 *  MOTOR_MODEL_X42S
 */
#define CURRENT_MOTOR_MODEL      MOTOR_MODEL_X42S

/**
 * 使用心跳保护
 * 这里只做最小参数读取保证，具体功能需要在应用层自行实现
 */
#define USE_HEARTBEAT 1

/**
 * 电机数量
 * 应用层辅助定义，可不使用
 */
#define ZDT_STEP_NUM 4

/**
 * 仅使用驱动函数
 * 启用后，仅对驱动层进行编译，不使用引擎层
 * 适合裸机环境、简单的调用方式
 */
#define ZDT_ONLY_DRIVER 0


/******************** 设备信息与特殊功能 *********************/
#define MOTOR_READ_VERSION                  0   // 读取固件版本和硬件版本
#define MOTOR_READ_PHASE_PARAMS             0   // 读取相电阻和相电感

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_BROADCAST_READ_ID             0   // 广播读取ID地址
#define MOTOR_READ_OPTION_PARAMS            0   // 读取选项参数状态
#if MOTOR_READ_OPTION_PARAMS
#define MOTOR_DRIVER_MOTOR_TYPE             0   // 电机类型
#define MOTOR_DIRECTION                     0   // 电机运动正方向
#define MOTOR_FIRMWARE_TYPE                 0   // 电机固件类型
#define MOTOR_LOCK_PARAMS                   0   // 锁定参数修改等级
#define MOTOR_DRIVER_CONTROL_MODE           0   // 控制模式（开环/闭环）
#define MOTOR_DRIVER_LOCK_KEY               0   // 锁定按键功能
#define MOTOR_DRIVER_SCALE_10X              0   // 缩小10倍输入（Emm：速度， X：角度）
#endif /* MOTOR_READ_OPTION_PARAMS */
#endif /* CURRENT_MOTOR_MODEL */


/******************** 系统状态参数 *********************/
#define MOTOR_STATUS_READ_BATCH             0   // 批量读取系统状态参数
#if MOTOR_STATUS_READ_BATCH
#if CURRENT_FIRMWARE == FIRMWARE_X
#define MOTOR_STATUS_ENCODER_RAW            0   // 读取编码器原始值
#endif
#endif

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_STATUS_PIN_STATUS             0   // 读取引脚IO电平状态
#define MOTOR_STATUS_TEMPERATURE            0   // 读取驱动温度
#define MOTOR_STATUS_FLAGS_COMBINED         0   // 读取回零+电机状态标志
#if !MOTOR_STATUS_FLAGS_COMBINED
#define MOTOR_STATUS_MOTOR_FLAGS            0   // 读取电机状态标志
#define MOTOR_STATUS_HOME_FLAGS             0   // 读取回零状态标志
#endif
#endif /* CURRENT_MOTOR_MODEL */

#if CURRENT_FIRMWARE == FIRMWARE_X
#define MOTOR_STATUS_BUS_CURRENT            0   // 读取总线电流
#endif

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_STATUS_BATTERY_VOLTAGE        0   // 读取电池电压
#endif

#define MOTOR_STATUS_BUS_VOLTAGE            0   // 读取总线电压
#define MOTOR_STATUS_PHASE_CURRENT          0   // 读取相电流
#define MOTOR_STATUS_ENCODER_VALUE          0   // 读取线性化编码器值
#define MOTOR_STATUS_TARGET_POS             0   // 读取电机目标位置
#define MOTOR_STATUS_SPEED                  0   // 读取电机实时转速
#define MOTOR_STATUS_REAL_POS               0   // 读取电机实时位置
#define MOTOR_STATUS_POS_ERROR              0   // 读取电机位置误差
#define MOTOR_STATUS_INPUT_PULSES           0   // 读取输入脉冲数
#define MOTOR_STATUS_SET_POS                0   // 读取电机实时设定目标位置


/******************** 驱动配置参数 *********************/
#define MOTOR_DRIVER_READ_BATCH             0   // 批量读取驱动配置参数
#if MOTOR_DRIVER_READ_BATCH
#define MOTOR_DRIVER_MICRO_STEP             0   // 细分值
#define MOTOR_DRIVER_FIRMWARE_TYPE          0   // 细分插补
#define MOTOR_DRIVER_OPENLOOP_CURRENT       0   // 开环模式工作电流
#define MOTOR_DRIVER_CLOSEDLOOP_CURRENT     0   // 闭环模式最大电流
#define MOTOR_DRIVER_BAUDRATE               0   // 波特率索引
#define MOTOR_DRIVER_CAN_RATE               0   // CAN速率索引
#define MOTOR_DRIVER_STALL_PROTECT          0   // 堵转保护
#define MOTOR_DRIVER_STALL_SPEED            0   // 堵转保护转速
#define MOTOR_DRIVER_STALL_CURRENT          0   // 堵转保护电流
#define MOTOR_DRIVER_STALL_TIME             0   // 堵转保护延时
#define MOTOR_DRIVER_PULSE_PORT_MODE        0   // 脉冲端口复用模式
#define MOTOR_DRIVER_COMM_PORT_MODE         0   // 通讯端口复用模式
#define MOTOR_DRIVER_EN_PIN_LEVEL           0   // En引脚有效电平
#define MOTOR_DRIVER_DIR_PIN_LEVEL          0   // Dir引脚有效电平
#define MOTOR_DRIVER_AUTO_SCREEN_OFF        0   // 自动息屏
#define MOTOR_DRIVER_COMM_CHECK_MODE        0   // 通讯校验方式
#define MOTOR_DRIVER_CMD_RESPONSE_MODE      0   // 控制命令应答方式

#if CURRENT_FIRMWARE == FIRMWARE_X
#define MOTOR_DRIVER_CURRENT_LOOP_BANDWIDTH 0   // 电流环带宽
#define MOTOR_DRIVER_CLOSEDLOOP_MAX_SPEED   0   // 闭环模式最大速度

#if !MOTOR_READ_OPTION_PARAMS
#define MOTOR_DRIVER_CONTROL_MODE           0   // 控制模式（开环/闭环）
#define MOTOR_DRIVER_LOCK_KEY               0   // 锁定按键功能
#define MOTOR_DRIVER_SCALE_10X              0   // 缩小10倍输入（Emm：速度， X：角度）
#endif
#elif CURRENT_FIRMWARE == FIRMWARE_EMM
#define MOTOR_DRIVER_CLOSEDLOOP_MAX_VOLTAGE 0   // 闭环模式最大输出电压

#if !MOTOR_READ_OPTION_PARAMS
#define MOTOR_DRIVER_MOTOR_TYPE             0   // 电机类型
#endif
#endif /* CURRENT_FIRMWARE */
#endif /* MOTOR_DRIVER_READ_BATCH */

#define MOTOR_DRIVER_POS_WINDOW             0   // 位置到达窗口
#define MOTOR_DRIVER_HOME                   0   // 读取回零参数
#if MOTOR_DRIVER_HOME
#define MOTOR_DRIVER_HOME_WRITE             0   // 修改回零参数
#define MOTOR_DRIVER_HOME_MODE              0   // 回零模式
#define MOTOR_DRIVER_HOME_DIR               0   // 回零方向
#define MOTOR_DRIVER_HOME_SPEED             0   // 回零速度
#define MOTOR_DRIVER_HOME_TIMEOUT           0   // 回零超时
#define MOTOR_DRIVER_AUTO_HOME              0   // 上电自动回零
#define MOTOR_DRIVER_HOME_COLLISION_SPEED   0   // 碰撞回零检测转速
#define MOTOR_DRIVER_HOME_COLLISION_CURRENT 0   // 碰撞回零检测电流
#define MOTOR_DRIVER_HOME_COLLISION_TIME    0   // 碰撞回零检测时间
#endif


/******************** 控制参数读写 *********************/
#define MOTOR_PID_READ                      0   // 读取PID参数
#if MOTOR_PID_READ
#define MOTOR_PID_WRITE                     0   // 修改PID参数
#endif

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_INTEGRAL_LIMIT_READ           0   // 积分限幅/刚性系数
#if MOTOR_INTEGRAL_LIMIT_READ
#define MOTOR_INTEGRAL_LIMIT_WRITE          0   // 修改积分限幅/刚性系数
#endif

#define MOTOR_PROTECT_THRESHOLD_READ        0   // 读取过热过流保护阈值
#if MOTOR_PROTECT_THRESHOLD_READ
#define MOTOR_PROTECT_THRESHOLD_WRITE       0   // 修改过热过流保护阈值
#endif

#define MOTOR_COLLISION_ANGLE_READ          0   // 读取碰撞回零返回角度
#if MOTOR_COLLISION_ANGLE_READ
#define MOTOR_COLLISION_ANGLE_WRITE         0   // 修改碰撞回零返回角度
#endif

#define MOTOR_HEARTBEAT_READ                0   // 读取心跳保护时间
#define MOTOR_HEARTBEAT_WRITE               0   // 修改心跳保护时间

#if USE_HEARTBEAT
#undef MOTOR_HEARTBEAT_WRITE
#define MOTOR_HEARTBEAT_WRITE             true  // 确保函数启用
#undef MOTOR_READ_OPTION_PARAMS
#define MOTOR_READ_OPTION_PARAMS          true  // 确保最小参数读取
#endif
#endif /* CURRENT_MOTOR_MODEL */


/******************** 运动控制命令 *********************/
#define MOTOR_CMD_ENABLE                    1   // 电机使能控制
#define MOTOR_CMD_STOP                      1   // 立即停止
#define MOTOR_POS_MODE_FAST                 0   // 快速位置模式
#define MOTOR_POS_MODE                      1   // 位置模式
#define MOTOR_VELOCITY_MODE                 1   // 速度模式
#define MOTOR_SYNC_TRIGGER                  1   // 触发同步

#if CURRENT_FIRMWARE == FIRMWARE_X
#define MOTOR_POS_MODE_DIRECT               0   // 直通限速位置模式
#define MOTOR_TORQUE_MODE                   0   // 力矩模式

#define MOTOR_POS_MODE_DIRECT_LIMIT         0   // 直通限速位置模式（+最大电流限制）
#define MOTOR_TORQUE_MODE_LIMIT             0   // 力矩模式（+最大速度限制）
#define MOTOR_POS_MODE_LIMIT                0   // 位置模式（+最大电流限制）
#define MOTOR_VELOCITY_MODE_LIMIT           0   // 速度模式（+最大电流限制）
#endif /* CURRENT_FIRMWARE */


#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_MULTI_CMD                     1   // 多电机命令
#endif


/******************** 触发动作命令 *********************/
#define MOTOR_TRIGGER_ENCODER_CALIB         0   // 触发编码器校准
#define MOTOR_TRIGGER_RESET_POS             0   // 当前位置角度清零
#define MOTOR_TRIGGER_CLEAR_PROTECT         0   // 解除堵转/过热/过流保护
#define MOTOR_TRIGGER_FACTORY_RESET         0   // 恢复出厂设置
#define MOTOR_HOME_SET_ZERO                 0   // 设置单圈回零零点位置
#define MOTOR_HOME_TRIGGER                  0   // 触发回零
#define MOTOR_HOME_INTERRUPT                0   // 强制中断回零

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_TRIGGER_RESTART               1   // 重启电机
#endif


/******************** 电机参数设置 *********************/
#define MOTOR_SET_MOTOR_ID                  0   // 修改电机ID/地址
#define MOTOR_SET_MICRO_STEP                0   // 修改细分值
#define MOTOR_SET_POWER_FLAG                0   // 修改掉电标志
#define MOTOR_SET_MOTOR_TYPE                0   // 修改电机类型
#define MOTOR_SET_FIRMWARE_TYPE             0   // 修改固件类型
#define MOTOR_SET_OPENLOOP_CURRENT          0   // 修改开环模式工作电流
#define MOTOR_SET_CLOSEDLOOP_CURRENT        0   // 修改闭环模式最大电流
#define MOTOR_SET_CONTROL_MODE              0   // 修改开环/闭环控制模式
#define MOTOR_SET_DIRECTION                 0   // 修改电机运动正方向
#define MOTOR_SET_LOCK_KEY                  0   // 修改锁定按键功能
#define MOTOR_SET_SCALE_INPUT               0   // 修改缩小倍数输入
#define MOTOR_SET_DRIVER_CONFIG_BATCH       0   // 修改驱动配置参数（批量）

#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_X42S || CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
#define MOTOR_SET_LOCK_PARAMS               0   // 修改锁定参数修改等级
#define MOTOR_POS_WINDOW_WRITE              0   // 修改位置到达窗口
#define MOTOR_DMX512_WRITE                  0   // 修改DMX512协议参数
#define MOTOR_PERIODIC_RETURN               0   // 定时返回信息
#endif

#endif /* __ZDT_V5_CFG_H__ */
