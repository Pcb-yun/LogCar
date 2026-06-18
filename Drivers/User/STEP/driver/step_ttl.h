/**
 * @file step_ttl.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 串口处理逻辑层头文件
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
	uint8_t motor_id;			// 电机ID
	bool is_online;				// 是否在线

#if MOTOR_ELECTRICAL
	uint16_t voltage;			// 总线电压 mV

    #if CURRENT_FIRMWARE == FIRMWARE_X
	uint16_t bus_current;		// 总线电流 mA
    #endif

	uint16_t phase_current;		// 相电流 mA
	int8_t temp;				// 驱动温度

	#if CURRENT_MOTOR_MODEL == MOTOR_MODEL_Y42
	uint16_t battery_voltage;	// 电池电压 mV
	#endif

#endif

#if MOTOR_MOTION
	int16_t vel;				// 实时转速 RPM
	int32_t pos;				// 实时位置
	int32_t target_pos;			// 目标位置
	int32_t set_pos;			// 实时设定目标位置
	int32_t pos_error;			// 位置误差
	int32_t input_pulses;		// 输入脉冲数
#endif

#if MOTOR_ENCODER
	uint16_t encoder_raw;		// 编码器原始值
	uint16_t encoder_linear;	// 线性化编码器值 0-65535=0-360°
#endif

#if MOTOR_STATUS_FLAGS
	// 电机状态标志 (CMD_READ_MOTOR_STATUS 0x3A)
	bool ens;					// bit0: Ens_TF - 使能状态标志，0=未使能，1=已使能
	bool prf;					// bit1: Prf_TF - 位置到达标志，0=未到达，1=已到达
	bool cgi;					// bit2: Cgi_TF - 堵转标志，0=未触发，1=已触发
	bool cgp;					// bit3: Cgp_TF - 堵转保护标志
	bool esi_l;					// bit4: Esi_LF - 左限位开关状态，0=低电平，1=高电平
	bool esi_r;					// bit6: Esi_RF - 右限位开关状态
	bool oac;					// bit7: Oac_TF - 掉电标志

	// 回零状态标志 (CMD_READ_HOME_STATUS 0x3B)
	bool enc_rdy;				// bit0: Enc_Rdy - 编码器就绪标志，0=编码器异常，1=编码器正常
	bool cal_rdy;				// bit1: Cal_Rdy - 校准表就绪标志，0=未校准，1=已校准
	bool org_sf;				// bit2: Org_SF - 正在回零标志
	bool org_cf;				// bit3: Org_CF - 回零失败标志
	bool otp_tf;				// bit4: Otp_TF - 过热保护标志
	bool ocp_tf;				// bit7: Ocp_TF - 过流保护标志

	uint8_t pin_status;			// 引脚IO电平
#endif

#if MOTOR_SYSTEM
	uint16_t firmware_version;	// 固件版本
	uint8_t hardware_version;	// 硬件版本
	uint16_t phase_resistance;	// 相电阻 mOhm
	uint16_t phase_inductance;	// 相电感 uH
	uint8_t option_params;		// 选项参数
	uint8_t lock_level;			// 锁定参数等级 0-3
#endif

#if MOTOR_CONTROL
	uint32_t kp;				// Kp
	uint32_t ki;				// Ki
	uint32_t kd;				// Kd
	uint16_t pos_window;		// 位置到达窗口（原始值，÷10=度，如8→0.8°）
	uint32_t integral_limit;	// 积分限幅(Emm)/刚性系数(X)
#endif

#if MOTOR_PROTECTION
	uint16_t temp_threshold;	// 过热阈值
	uint16_t current_threshold;	// 过流阈值 mA
	uint16_t protect_time;		// 过热过流检测时间 ms
	uint32_t heartbeat_time;	// 心跳保护时间 ms
	uint16_t collision_angle;	// 碰撞回零返回角度（0=基于电流，其余÷10=度）
#endif

#if MOTOR_CLOG
	uint8_t clog_enable;		// 堵转保护开关
	uint16_t clog_rpm;			// 堵转检测转速 RPM
	uint16_t clog_current;		// 堵转检测电流 mA
	uint16_t clog_time;			// 堵转检测时间 ms
#endif

#if MOTOR_HOME
	uint8_t home_mode;			// 回零模式 0-5
	uint8_t home_dir;			// 回零方向 0=CW 1=CCW
	uint16_t home_speed;		// 回零速度 RPM
	uint32_t home_timeout;		// 回零超时 ms
	uint8_t home_auto_enable;	// 上电自动回零
	uint16_t collision_rpm;		// 碰撞检测转速 RPM
	uint16_t collision_current;	// 碰撞检测电流 mA
	uint16_t collision_time;	// 碰撞检测时间 ms
#endif

#if MOTOR_DRIVER
	uint8_t control_mode;		// 控制模式 0=开环 1=闭环
	uint8_t motor_type;			// 电机类型 25=0.9° 50=1.8°
	uint8_t motor_direction;	// 电机运动方向 0=CW 1=CCW
	uint8_t micro_step;			// 细分
	uint8_t interpolation;		// 细分插补 0/1
	uint16_t open_current;		// 开环电流 mA
	uint16_t close_current;		// 闭环电流 mA

    #if CURRENT_FIRMWARE == FIRMWARE_EMM
	uint16_t max_output_voltage;// 最大输出电压 (Emm)
    #elif CURRENT_FIRMWARE == FIRMWARE_X
	uint16_t max_speed;			// 最大转速 RPM (X)
    #endif

#endif

#if MOTOR_COMM
	uint8_t uart_baudrate;		// 串口波特率编码 0-8
	uint8_t can_baudrate;		// CAN波特率编码 0-8
	uint8_t verify_mode;		// 校验方式 0-4
	uint8_t response_mode;		// 应答方式 0-4

    #if CURRENT_FIRMWARE == FIRMWARE_X
	uint8_t pos_scale;			// 位置放大100倍 0/1 (X)
    #endif

#endif
} MotorStatus_t;


typedef struct {
    MotorStatus_t motors[4];
} MotorStatusShared_t;

void Motor_Receive(uint8_t *data, uint8_t len);
void Motor_TTL_Send(uint8_t *cmd, uint8_t len);
void Motor_Process_Cmd(MotorCmd_t *cmd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STEP_TTL_H__ */
