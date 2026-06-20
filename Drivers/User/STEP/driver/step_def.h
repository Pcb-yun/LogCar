/**
 * @file step_def.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机配置定义头文件
 */

#ifndef __STEP_DEF_H__
#define __STEP_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

 /**
 * @brief 固件版本定义
 */
#define FIRMWARE_EMM             1   // Emm固件
#define FIRMWARE_X               0   // X固件

/**
 * @brief 电机型号支持宏定义
 */
#define MOTOR_MODEL_X42S         1   // X42S电机
#define MOTOR_MODEL_Y42          0   // Y42电机

/**
 * @brief 电机命令功能码宏定义
 */
#define CMD_READ_BUS_VOLTAGE      0x24  // 读取总线电压
#define CMD_READ_BUS_CURRENT      0x26  // 读取总线电流
#define CMD_READ_PHASE_CURRENT    0x27  // 读取相电流
#define CMD_READ_TEMPERATURE      0x39  // 读取驱动温度
#define CMD_READ_POSITION         0x36  // 读取电机实时位置
#define CMD_READ_TARGET_POSITION  0x33  // 读取电机目标位置
#define CMD_READ_SET_POSITION     0x34  // 读取电机实时设定的目标位置
#define CMD_READ_SPEED            0x35  // 读取电机实时转速
#define CMD_READ_POSITION_ERROR   0x37  // 读取电机位置误差
#define CMD_READ_INPUT_PULSES     0x32  // 读取输入脉冲数
#define CMD_READ_ENCODER_VALUE    0x31  // 读取线性化编码器值
#define CMD_READ_MOTOR_STATUS     0x3A  // 读取电机状态标志
#define CMD_READ_HOME_STATUS      0x3B  // 读取回零状态标志
#define CMD_READ_VERSION_INFO     0x1F  // 读取固件版本和硬件版本
#define CMD_READ_PHASE_PARAMS     0x20  // 读取相电阻和相电感
#define CMD_READ_OPTION_PARAMS    0x1A  // 读取选项参数状态
#define CMD_READ_PID_PARAMS       0x21  // 读取PID参数
#define CMD_READ_HOME_PARAMS      0x22  // 读取回零参数
#define CMD_READ_POSITION_WINDOW  0x41  // 读取位置到达窗口
#define CMD_READ_DMX512_PARAMS    0x49  // 读取DMX512参数
#define CMD_READ_INTEGRAL_LIMIT   0x23  // 读取积分限幅/刚性系数
#define CMD_READ_PROTECT_THRESHOLD 0x13 // 读取过热过流保护检测阈值
#define CMD_READ_HEARTBEAT_TIME   0x16  // 读取心跳保护功能时间
#define CMD_READ_COLLISION_ANGLE  0x3F  // 读取碰撞回零返回角度
#define CMD_READ_SYSTEM_STATUS    0x43  // 读取系统状态参数
#define CMD_READ_DRIVER_CONFIG    0x42  // 读取驱动配置参数
#define CMD_READ_BATTERY_VOLTAGE  0x38  // 读取电池电压（Y42）
#define CMD_READ_STATUS_FLAGS     0x3C  // 读取回零状态+电机状态（Y42）
#define CMD_READ_PIN_STATUS       0x3D  // 读取引脚IO电平状态（Y42）
#define AUX_CODE_SYSTEM_STATUS    0x7A  // 系统状态参数辅助码
#define AUX_CODE_DRIVER_CONFIG    0x6C  // 驱动配置参数辅助码
#define CMD_SET_TIMER_REPORT      0x11  // 设置定时返回
#define END_CODE                  0x6B  // 结束码

/* 触发动作命令 */
#define CMD_TRIGGER_ENCODER_CALIB   0x06  // 触发编码器校准
#define CMD_RESTART_MOTOR           0x08  // 重启电机
#define CMD_RESET_CURPOS_TO_ZERO     0x0A  // 重置当前位置为零
#define CMD_CLEAR_PROTECTION        0x0E  // 解除堵转/过热/过流保护
#define CMD_RESTORE_FACTORY         0x0F  // 恢复出厂设置

/* 运动控制命令 */
#define CMD_MOTOR_ENABLE            0xF3  // 电机使能控制
#define CMD_TORQUE_MODE          0xF5  // 力矩模式控制（X固件）
#define CMD_TORQUE_MODE_LIMIT    0xC5  // 力矩模式限速控制（X固件）
#define CMD_VELOCITY_MODE_LIMIT  0xC6  // 速度模式限电流控制（X固件）
#define CMD_POS_MODE_DIRECT      0xFB  // 直通限速位置模式（X固件）
#define CMD_POS_MODE_DIRECT_LIMIT 0xCB // 直通限速位置模式限电流（X固件）
#define CMD_POS_MODE_TRAPEZOIDAL 0xFD  // 梯形曲线位置模式（X固件）
#define CMD_POS_MODE_TRAPEZOIDAL_LIMIT 0xCD // 梯形曲线位置模式限电流（X固件）
#define CMD_POS_MODE_EMM         0xFD  // 位置模式（Emm固件）
#define CMD_VELOCITY_MODE        0xF6  // 速度模式控制
#define CMD_POS_MODE_FAST_SET    0xF1  // 快速位置模式设定参数
#define CMD_POS_MODE_FAST_SEND   0xFC  // 快速位置模式发送脉冲/位置
#define CMD_STOP_NOW             0xFE  // 立即停止
#define CMD_SYNC_MOTION          0xFF  // 触发多机同步运动

/* 回零命令 */
#define CMD_SET_HOME_ZERO        0x93  // 设置单圈回零的零点位置
#define CMD_TRIGGER_HOME         0x9A  // 触发回零
#define CMD_HOME_INTERRUPT       0x9C  // 强制中断并退出回零操作
#define CMD_SET_HOME_PARAMS      0x4C  // 修改回零参数

/* 多电机命令 */
#define CMD_MULTI_MOTOR          0xAA  // 多电机命令

/* 修改驱动参数命令 */
#define CMD_SET_MOTOR_ID         0xAE  // 修改电机ID/地址
#define CMD_SET_MICRO_STEP       0x84  // 修改细分值
#define CMD_SET_POWER_FLAG       0x50  // 修改掉电标志
#define CMD_SET_MOTOR_TYPE       0xD7  // 修改电机类型
#define CMD_SET_FIRMWARE_TYPE    0xD5  // 修改固件类型
#define CMD_SET_OPENLOOP_CURRENT 0x44  // 修改开环模式工作电流
#define CMD_SET_CLOSEDLOOP_CURRENT 0x45 // 修改闭环模式最大电流
#define CMD_SET_PID_PARAMS       0x4A  // 修改PID参数
#define CMD_SET_DMX512_PARAMS    0xD9  // 修改DMX512协议参数
#define CMD_SET_POS_WINDOW       0xD1  // 修改位置到达窗口
#define CMD_SET_PROTECT_THRESHOLD 0xD3 // 修改过热过流保护阈值
#define CMD_SET_HEARTBEAT_TIME   0x68  // 修改心跳保护时间
#define CMD_SET_INTEGRAL_LIMIT   0x4B  // 修改积分限幅/刚性系数
#define CMD_SET_COLLISION_ANGLE  0x5C  // 修改碰撞回零返回角度
#define CMD_SET_CONTROL_MODE     0x46  // 修改开环/闭环控制模式
#define CMD_SET_LOCK_KEY         0xD0  // 修改锁定按键功能
#define CMD_SET_DIRECTION        0xD4  // 修改电机运动正方向
#define CMD_SET_SCALE_INPUT      0x4F  // 修改缩小倍数输入
#define CMD_SET_LOCK_PARAMS      0xD6  // 修改锁定参数功能
#define CMD_SET_DRIVER_CONFIG    0x48  // 修改驱动配置参数（一次性）
#define CMD_STORE_SPEED_PARAMS   0xF7  // 存储速度参数上电自动运行

/* 辅助码定义 */
#define AUX_CODE_TIMER_REPORT      0x18  // 定时返回辅助码
#define AUX_CODE_ENCODER_CALIB     0x45  // 触发编码器校准辅助码
#define AUX_CODE_RESTART_MOTOR     0x97  // 重启电机辅助码
#define AUX_CODE_RESET_POS         0x6D  // 当前位置清零辅助码
#define AUX_CODE_CLEAR_PROTECT     0x52  // 解除保护辅助码
#define AUX_CODE_RESTORE_FACTORY   0x5F  // 恢复出厂辅助码
#define AUX_CODE_MOTOR_ENABLE      0xAB  // 电机使能辅助码
#define AUX_CODE_STOP_NOW          0x98  // 立即停止辅助码
#define AUX_CODE_SYNC_MOTION       0x66  // 多机同步辅助码
#define AUX_CODE_SET_HOME_ZERO     0x88  // 设置回零零点辅助码
#define AUX_CODE_HOME_INTERRUPT    0x48  // 强制中断回零辅助码
#define AUX_CODE_SET_HOME_PARAMS   0xAE  // 修改回零参数辅助码
#define AUX_CODE_SET_MOTOR_ID      0x4B  // 修改ID辅助码
#define AUX_CODE_SET_MICRO_STEP    0x8A  // 修改细分辅助码
#define AUX_CODE_SET_POWER_FLAG    0x5E  // 修改掉电标志辅助码
#define AUX_CODE_SET_MOTOR_TYPE    0x23  // 修改电机类型辅助码
#define AUX_CODE_SET_FIRMWARE_TYPE 0x69  // 修改固件类型辅助码
#define AUX_CODE_SET_OPENLOOP_CURRENT 0x48  // 修改开环电流辅助码
#define AUX_CODE_SET_CLOSEDLOOP_CURRENT 0x66  // 修改闭环电流辅助码
#define AUX_CODE_SET_CONTROL_MODE  0xA6  // 修改控制模式辅助码
#define AUX_CODE_SET_DIRECTION     0x60  // 修改正方向辅助码
#define AUX_CODE_SET_LOCK_KEY      0xB3  // 修改锁定按键辅助码
#define AUX_CODE_SET_SCALE_INPUT   0x71  // 修改缩小倍数辅助码
#define AUX_CODE_SET_PID_PARAMS    0xC3  // 修改PID参数辅助码
#define AUX_CODE_SET_INTEGRAL_LIMIT 0x57 // 修改积分限幅辅助码
#define AUX_CODE_SET_POS_WINDOW    0x07  // 修改位置到达窗口辅助码
#define AUX_CODE_SET_PROTECT_THRESHOLD 0x56 // 修改保护阈值辅助码
#define AUX_CODE_SET_HEARTBEAT_TIME 0x38 // 修改心跳保护辅助码
#define AUX_CODE_SET_COLLISION_ANGLE 0xAC // 修改碰撞回零角度辅助码
#define AUX_CODE_SET_DRIVER_CONFIG 0xD1  // 修改驱动配置辅助码
#define AUX_CODE_READ_DMX512       0x78  // 读取DMX512辅助码
#define AUX_CODE_SET_LOCK_PARAMS   0x4B  // 修改锁定参数辅助码

/* 系统命令 */
#define CMD_BROADCAST_READ_ID    0x15  // 广播读取ID地址
#define END_CODE                 0x6B  // 通用校验码

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_DEF_H__ */
