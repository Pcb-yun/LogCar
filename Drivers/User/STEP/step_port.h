/**
 * @file step_port.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 张大头V5步进电机用户配置头文件
 */

 #ifndef __STEP_PORT_H__
 #define __STEP_PORT_H__

 #ifdef __cplusplus
 extern "C" {
 #endif /* __cplusplus */

 #include <stdint.h>

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


#define CURRENT_FIRMWARE         FIRMWARE_EMM       // 当前使用的固件版本
#define CURRENT_MOTOR_MODEL      MOTOR_MODEL_X42S   // 当前使用的电机型号

/**
 * @brief 电机状态结构体配置宏
 */
#define MOTOR_STATUS_ELECTRICAL   1      // 读取电气参数
#define MOTOR_STATUS_MOTION       1      // 读取运动状态
#define MOTOR_STATUS_ENCODER      1      // 读取编码器信息
#define MOTOR_STATUS_STATUS       1      // 读取状态标志
#define MOTOR_STATUS_SYSTEM       1      // 读取系统信息
#define MOTOR_STATUS_CONTROL      1      // 读取控制参数
#define MOTOR_STATUS_PROTECTION   1      // 读取保护参数

/**
 * @brief 电机控制命令结构体配置宏
 */
#define MOTOR_CMD_ENABLE         1      // 使能控制命令
#define MOTOR_CMD_VELOCITY       1      // 速度模式命令
#define MOTOR_CMD_POSITION       1      // 位置模式命令
#define MOTOR_CMD_TORQUE         1      // 力矩模式命令
#define MOTOR_CMD_STOP           1      // 停止命令
#define MOTOR_CMD_HOME           1      // 回零命令

/**
 * @brief 电机参数配置结构体配置宏
 */
#define MOTOR_PARAM_BASIC       1      // 基本参数
#define MOTOR_PARAM_CURRENT     1      // 电流参数
#define MOTOR_PARAM_PID         1      // PID参数
#define MOTOR_PARAM_PROTECT     1      // 保护参数
#define MOTOR_PARAM_COMM        1      // 通信参数

/**
 * @brief 电机状态结构体
 */
typedef struct {
    uint8_t addr;             // 电机地址

#if MOTOR_STATUS_ELECTRICAL
    uint16_t voltage;         // 总线电压（mV）
    uint16_t current;         // 总线电流（mA）
    uint16_t phase_current;   // 相电流（mA）
    uint8_t temp;             // 温度（℃）
    #if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
    uint16_t battery_voltage; // 电池电压（mV，Y42）
    #endif
#endif

#if MOTOR_STATUS_MOTION
    int16_t vel;              // 当前速度（RPM）
    int32_t pos;              // 当前位置
    int32_t target_pos;       // 目标位置
    int32_t set_pos;          // 实时设定的目标位置
    int32_t pos_error;        // 位置误差
    int32_t input_pulses;     // 输入脉冲数
#endif

#if MOTOR_STATUS_ENCODER
    uint16_t encoder_value;   // 线性化编码器值
#endif

#if MOTOR_STATUS_STATUS
    uint8_t status;           // 电机状态标志
    uint8_t home_status;      // 回零状态标志
    uint8_t pin_status;       // 引脚状态
#endif

#if MOTOR_STATUS_SYSTEM
    uint16_t firmware_version; // 固件版本
    uint8_t hardware_version;  // 硬件版本
    uint8_t motor_id;          // 电机ID
#endif

#if MOTOR_STATUS_CONTROL
    uint32_t pid_kp;           // PID比例系数
    uint32_t pid_ki;           // PID积分系数
    uint32_t pid_kd;           // PID微分系数
    uint16_t pos_window;       // 位置到达窗口
#endif

#if MOTOR_STATUS_PROTECTION
    uint16_t over_temp_threshold;  // 过热保护阈值（℃）
    uint16_t over_current_threshold; // 过流保护阈值（mA）
    uint32_t heartbeat_time;   // 心跳保护时间（ms）
#endif
} MotorStatus_t;

/**
 * @brief 电机控制命令结构体
 */
typedef struct {
    uint8_t addr;             // 电机地址

#if MOTOR_CMD_ENABLE
    uint8_t enable;           // 使能状态：0-不使能，1-使能
    uint8_t sync_flag;        // 同步标志：0-立即执行，1-等待同步触发
#endif

#if MOTOR_CMD_VELOCITY
    uint8_t vel_dir;          // 速度方向：0-CW，1-CCW
    uint16_t vel;             // 速度（RPM）
    uint16_t vel_acc;         // 加速度（RPM/S或档位）
    uint16_t vel_max_current; // 最大电流限制（mA）
#endif

#if MOTOR_CMD_POSITION
    uint8_t pos_dir;          // 位置方向：0-CW，1-CCW
    uint16_t pos_vel;         // 运动速度（RPM）
    uint16_t pos_acc;         // 加速度
    #if CURRENT_FIRMWARE == FIRMWARE_X
    uint16_t pos_dec;         // 减速度（X固件）
    #endif
    int32_t pos_target;       // 目标位置（脉冲数或角度）
    uint8_t pos_mode;         // 运动模式：0-相对上一目标，1-绝对位置，2-相对当前位置
    #if CURRENT_FIRMWARE == FIRMWARE_X
    uint16_t pos_max_current; // 最大电流限制（mA）
    #endif
#endif

#if MOTOR_CMD_TORQUE && CURRENT_FIRMWARE == FIRMWARE_X
    uint8_t torque_dir;       // 力矩方向：0-CW，1-CCW
    uint16_t torque_slope;    // 电流斜率（mA/S）
    uint16_t torque_current;  // 目标电流（mA）
    uint16_t torque_max_vel;  // 最大速度限制（RPM）
#endif

#if MOTOR_CMD_STOP
    uint8_t stop_sync;        // 停止同步标志
#endif

#if MOTOR_CMD_HOME
    uint8_t home_mode;        // 回零模式：0-单圈就近，1-单圈方向，2-无限位碰撞，3-限位回零
    uint8_t home_dir;         // 回零方向：0-CW，1-CCW
    uint16_t home_vel;        // 回零速度（RPM）
    uint32_t home_timeout;    // 回零超时时间（ms）
    uint16_t home_sl_vel;     // 碰撞检测转速（RPM）
    uint16_t home_sl_current; // 碰撞检测电流（mA）
    uint16_t home_sl_time;    // 碰撞检测时间（ms）
    uint8_t home_auto;        // 上电自动回零：0-禁用，1-使能
#endif
} MotorCmd_t;

/**
 * @brief 电机参数配置结构体
 */
typedef struct {
    uint8_t addr;             // 电机地址

#if MOTOR_PARAM_BASIC
    uint8_t motor_id;         // 电机ID/地址
    uint8_t micro_step;       // 细分值
    uint8_t motor_type;       // 电机类型：0-1.8°，1-0.9°
    uint8_t firmware_type;    // 固件类型：0-X固件，1-Emm固件
    uint8_t ctrl_mode;        // 控制模式：0-开环，1-闭环FOC
    uint8_t motor_dir;        // 电机正方向：0-CW，1-CCW
    uint8_t save_flag;        // 存储标志：0-不存储，1-存储
#endif

#if MOTOR_PARAM_CURRENT
    uint16_t open_loop_current;  // 开环工作电流（mA）
    uint16_t closed_loop_current; // 闭环最大电流（mA）
#endif

#if MOTOR_PARAM_PID
    uint32_t pid_kp;          // PID比例系数
    uint32_t pid_ki;          // PID积分系数
    uint32_t pid_kd;          // PID微分系数
    uint32_t integral_limit;  // 积分限幅/刚性系数
#endif

#if MOTOR_PARAM_PROTECT
    uint16_t over_temp_threshold;  // 过热保护阈值（℃）
    uint16_t over_current_threshold; // 过流保护阈值（mA）
    uint16_t protect_time;    // 保护检测时间（ms）
    uint32_t heartbeat_time;  // 心跳保护时间（ms）
    uint16_t pos_window;      // 位置到达窗口
#endif

#if MOTOR_PARAM_COMM
    uint8_t uart_baudrate;    // 串口波特率
    uint8_t can_baudrate;     // CAN波特率
    uint8_t verify_mode;      // 校验方式
    uint8_t response_mode;    // 应答方式
#endif
} MotorParam_t;

/**
 * @brief 麦轮控制结构体
 */
typedef struct {
    MotorCmd_t motors[4];     // 四个电机的控制命令
    uint8_t cmd_count;        // 命令数量
    uint8_t sync_exec;        // 同步执行标志：0-单独执行，1-同步执行
} MecanumCtrl_t;

/**
 * @brief 多电机命令结构体
 */
typedef struct {
    uint8_t broadcast_addr;   // 广播地址（固定为0）
    uint8_t cmd_count;        // 子命令数量
    MotorCmd_t *cmd_list;     // 命令列表指针
} MultiMotorCmd_t;











#define EMM_V5_SEND_CMD(cmd, len)


 #ifdef __cplusplus
 }
 #endif /* __cplusplus */

 #endif /* __STEP_PORT_H__ */
