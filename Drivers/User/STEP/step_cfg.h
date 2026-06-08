/**
 * @file step_cfg.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机用户配置头文件
 */

#ifndef __STEP_CFG_H__
#define __STEP_CFG_H__

#define CURRENT_FIRMWARE         FIRMWARE_EMM       // 当前使用的固件版本
#define CURRENT_MOTOR_MODEL      MOTOR_MODEL_X42S   // 当前使用的电机型号

// 是否使用心跳保护
#define USE_HEARTBEAT 1

// 是否使用查看任务
// 当前程序架构存在问题，除调试阶段否则不建议使用
// 如果使能且启用了大量的功能开关，会严重影响控制性能
#define USE_VIEW 1

/******************** 电机参数 **********************/

#define MOTOR_ELECTRICAL   1      // 电气参数（电压/电流/温度）
#define MOTOR_MOTION       1      // 运动状态（速度/位置/误差/脉冲）
#define MOTOR_ENCODER      0      // 编码器信息（线性值/原始值）
#define MOTOR_STATUS_FLAGS 0      // 状态标志（电机/回零/引脚）
#define MOTOR_SYSTEM       0      // 系统信息（固件/硬件/电阻/电感/选项）
#define MOTOR_CONTROL      0      // 控制参数（PID/积分限幅/位置窗口）
#define MOTOR_PROTECTION   0      // 保护参数（过热/过流/心跳/碰撞角度）
#define MOTOR_CLOG         0      // 堵转保护参数（使能/转速/电流/延时）
#define MOTOR_HOME         0      // 回零参数（模式/方向/速度/超时/碰撞）
#define MOTOR_DRIVER       0      // 驱动配置（控制模式/电机类型/细分/插补/方向/电流/速度/固件）
#define MOTOR_COMM         0      // 通讯参数（波特率/校验/应答）
#define MOTOR_CURRENT      0      // 电流参数（开环/闭环电流）


/******************** 电机控制命令 **********************/

#define MOTOR_CMD_ENABLE   1      // 使能控制
#define MOTOR_CMD_VELOCITY 1      // 速度控制
#define MOTOR_CMD_POSITION 1      // 位置控制
#define MOTOR_CMD_TORQUE   0      // 力矩控制
#define MOTOR_CMD_STOP     1      // 停止
#define MOTOR_CMD_HOME     1      // 回零
#define MOTOR_CMD_FAST     0      // 快速位置


#endif /* __STEP_CFG_H__ */
