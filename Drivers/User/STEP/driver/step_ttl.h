/**
 * @file step_ttl.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 串口处理逻辑层头文件
 *
 * 参数分类说明：
 * - 系统状态参数：电机运行时动态变化的数据，通过0x43批量读取
 * - 驱动配置参数：电机掉电持久化存储的配置，通过0x42批量读取
 * - 控制参数：PID控制、保护阈值、回零参数等，无批量读取
 * - 设备信息：设备标识、版本信息、特殊协议参数，无批量读取
 */

#ifndef __STEP_TTL_H__
#define __STEP_TTL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stdbool.h>
#include "step_cmd.h"

/**
 * @brief 电机状态结构体
 */
typedef struct {
	/* ========== 基础信息 ========== */
	uint8_t motor_id;			// 电机ID
	bool is_online;				// 是否在线

	/* ========== 系统状态参数 ========== */
#if MOTOR_STATUS_BUS_VOLTAGE
	uint16_t voltage;			// 总线电压 mV
#endif

#if MOTOR_STATUS_BUS_CURRENT
	uint16_t bus_current;		// 总线电流 mA
#endif

#if MOTOR_STATUS_PHASE_CURRENT
	uint16_t phase_current;		// 相电流 mA
#endif

#if MOTOR_STATUS_TEMPERATURE
	int8_t temp;				// 驱动温度 ℃
#endif

#if MOTOR_STATUS_BATTERY_VOLTAGE
	uint16_t battery_voltage;	// 电池电压 mV
#endif

#if MOTOR_STATUS_SPEED
	int16_t vel;				// 实时转速 RPM
#endif

#if MOTOR_STATUS_REAL_POS
	int32_t pos;				// 实时位置
#endif

#if MOTOR_STATUS_TARGET_POS
	int32_t target_pos;			// 目标位置
#endif

#if MOTOR_STATUS_SET_POS
	int32_t set_pos;			// 实时设定目标位置
#endif

#if MOTOR_STATUS_POS_ERROR
	int32_t pos_error;			// 位置误差
#endif

#if MOTOR_STATUS_INPUT_PULSES
	int32_t input_pulses;		// 输入脉冲数
#endif

#if MOTOR_STATUS_ENCODER_VALUE
	uint16_t encoder_linear;	// 线性化编码器值 0-65535=0-360°
#endif

#if MOTOR_STATUS_ENCODER_RAW
	uint16_t encoder_raw;		// 编码器原始值
#endif

#if MOTOR_STATUS_MOTOR_FLAGS
	bool ens;					// bit0: Ens_TF - 使能状态，0=未使能，1=已使能
	bool prf;					// bit1: Prf_TF - 位置到达，0=未到达，1=已到达
	bool cgi;					// bit2: Cgi_TF - 堵转标志，0=未触发，1=已触发
	bool cgp;					// bit3: Cgp_TF - 堵转保护标志
	bool esi_l;					// bit4: Esi_LF - 左限位开关
	bool esi_r;					// bit6: Esi_RF - 右限位开关
	bool oac;					// bit7: Oac_TF - 掉电标志
#endif

#if MOTOR_STATUS_HOME_FLAGS
	bool enc_rdy;				// bit0: Enc_Rdy - 编码器就绪
	bool cal_rdy;				// bit1: Cal_Rdy - 校准表就绪
	bool org_sf;				// bit2: Org_SF - 正在回零
	bool org_cf;				// bit3: Org_CF - 回零失败
	bool otp_tf;				// bit4: Otp_TF - 过热保护
	bool ocp_tf;				// bit7: Ocp_TF - 过流保护
#endif

#if MOTOR_STATUS_PIN_STATUS
	uint8_t pin_status;			// 引脚IO电平状态
#endif

	/* ========== 驱动配置参数 ========== */
#if MOTOR_DRIVER_LOCK_KEY
	bool lock_key;				// 按键锁定 0=关闭 1=开启
#endif

#if MOTOR_DRIVER_CONTROL_MODE
	bool control_mode;			// 控制模式 0=开环 1=闭环
#endif

#if MOTOR_DRIVER_MICRO_STEP
	uint8_t micro_step;			// 细分
#endif

#if MOTOR_DRIVER_DIRECTION
	uint8_t motor_direction;	// 电机运动方向 0=CW 1=CCW
#endif

#if MOTOR_DRIVER_MOTOR_TYPE
	uint8_t motor_type;			// 电机类型 25=0.9° 50=1.8°
#endif

#if MOTOR_DRIVER_OPENLOOP_CURRENT
	uint16_t open_current;		// 开环电流 mA
#endif

#if MOTOR_DRIVER_CLOSEDLOOP_CURRENT
	uint16_t close_current;		// 闭环电流 mA
#endif

#if MOTOR_DRIVER_FIRMWARE_TYPE
	bool interpolation;			// 细分插补 0/1
#endif

#if MOTOR_DRIVER_POWER_FLAG
	bool power_flag;			// 掉电标志 0=正常 1=掉电保存
#endif

#if MOTOR_DRIVER_BAUDRATE
	uint8_t uart_baudrate;		// 串口波特率编码 0-8
#endif

#if MOTOR_DRIVER_CAN_RATE
	uint8_t can_baudrate;		// CAN波特率编码 0-8
#endif

#if MOTOR_DRIVER_STALL_PROTECT
	bool clog_enable;			// 堵转保护开关 0=关闭 1=开启
#endif

#if MOTOR_DRIVER_STALL_SPEED
	uint16_t clog_rpm;			// 堵转检测转速 RPM
#endif

#if MOTOR_DRIVER_STALL_CURRENT
	uint16_t clog_current;		// 堵转检测电流 mA
#endif

#if MOTOR_DRIVER_STALL_TIME
	uint16_t clog_time;			// 堵转检测时间 ms
#endif

#if MOTOR_DRIVER_POS_WINDOW
	uint16_t pos_window;		// 位置到达窗口（÷10=度，如8→0.8°）
#endif

#if MOTOR_DRIVER_POS_ARRIVE
	bool pos_arrive;			// 位置到达检测 0=关闭 1=开启
#endif

#if MOTOR_DRIVER_PULSE_THRESHOLD
	uint16_t pulse_threshold;	// 脉冲阈值
#endif

/* ========== 设备信息与特殊功能 ========== */
#if MOTOR_READ_VERSION
	uint16_t firmware_version;	// 固件版本
	uint8_t hardware_version;	// 硬件版本
#endif

#if MOTOR_READ_PHASE_PARAMS
	uint16_t phase_resistance;	// 相电阻 mOhm
	uint16_t phase_inductance;	// 相电感 uH
#endif

#if MOTOR_READ_OPTION_PARAMS
	uint8_t option_params;		// 选项参数
	uint8_t lock_level;			// 锁定参数等级 0-3
#endif

	/* ========== 控制参数 ========== */
#if MOTOR_PID_READ
#if CURRENT_FIRMWARE == FIRMWARE_EMM
	uint32_t kp;				// 比例系数Kp
	uint32_t ki;				// 积分系数Ki
	uint32_t kd;				// 微分系数Kd
#elif CURRENT_FIRMWARE == FIRMWARE_X
	uint32_t trapezoidal_kp;		// 梯形曲线位置环Kp
	uint32_t direct_kp;			// 直通限速位置环Kp
	uint32_t vel_kp;			// 速度环Kp
	uint32_t vel_ki;			// 速度环Ki
#endif
#endif

#if MOTOR_DRIVER_INTEGRAL_LIMIT
	uint32_t integral_limit;	// 积分限幅(Emm)/刚性系数(X)
#endif

#if MOTOR_PROTECT_THRESHOLD_READ
	uint16_t temp_threshold;	// 过热阈值 ℃
	uint16_t current_threshold;	// 过流阈值 mA
	uint16_t protect_time;		// 过热过流检测时间 ms
#endif

#if MOTOR_HEARTBEAT_READ
	uint32_t heartbeat_time;	// 心跳保护时间 ms
#endif

#if MOTOR_COLLISION_ANGLE_READ
	uint16_t collision_angle;	// 碰撞回零返回角度（÷10=度）
#endif

#if MOTOR_HOME_READ
	uint8_t home_mode;			// 回零模式 0-5
	uint8_t home_dir;			// 回零方向 0=CW 1=CCW
	uint16_t home_speed;		// 回零速度 RPM
	uint32_t home_timeout;		// 回零超时 ms
#endif

#if MOTOR_DRIVER_HOME_SPEED
	uint16_t sl_vel;			// 碰撞检测转速 RPM
#endif

#if MOTOR_DRIVER_HOME_MODE
	bool home_auto_enable;		// 上电自动回零 0=关闭 1=开启
#endif

#if MOTOR_DRIVER_HOME_DIR
	uint16_t sl_current;		// 碰撞检测电流 mA
#endif

#if MOTOR_DRIVER_HOME_TIMEOUT
	uint16_t sl_time;			// 碰撞检测时间 ms
#endif
} MotorStatus_t;

/**
 * @brief 共享电机状态结构体
 */
typedef struct {
	MotorStatus_t motors[4];
	uint16_t update_time;		// 电机状态更新时间间隔 (ms)
} MotorStatusShared_t;


void Motor_Receive(uint8_t *data, uint8_t len);
void Motor_TTL_Send(uint8_t *cmd, uint8_t len);
void Motor_Process_Cmd(MotorCmd_t *cmd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_TTL_H__ */
