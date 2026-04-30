/**
 * @file servo_driver.h
 * @brief Fashion Star总线伺服舵机FreeRTOS驱动层
 */

#ifndef __SERVO_DRIVER_H__
#define __SERVO_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "freertos.h"
#include "Events.h"

extern UART_HandleTypeDef huart3;

#define SERVO_DLC 0 // 舵机其他功能
#define SERVO_ASYNC 0 // 异步命令
#define SERVO_SYNC 0 // 同步命令
#define SERVO_MONITOR 0 // 监控命令
#define SERVO_SYNC_MONITOR 0 // 同步监控命令
#define SERVO_PING 1 // Ping命令

#define SERVO_TIMEOUT_MS 100     // 串口通讯超时设置
#define SERVO_MAX_COUNT 2   // 最大舵机数量

/**
 * @brief Fashion Star总线伺服舵机协议请求头
 */
#define SERVO_PACK_REQUEST_HEADER 0x4c12
#define SERVO_PACK_RESPONSE_HEADER 0x1c05

// 返回的响应数据包最长的长度
#define SERVO_PACK_RESPONSE_MAX_SIZE 350

#define SERVO_CMD_NUM 30

// SERVO控制指令数据
// 注: 一下所有的指令都是针对单个舵机的
#define SERVO_CMD_NUM 30
#define SERVO_CMD_PING 1                               // 舵机通讯检测
#define SERVO_CMD_RESET_USER_DATA 2                    // 重置用户数据
#define SERVO_CMD_READ_DATA 3                          // 单个舵机 读取数据库
#define SERVO_CMD_WRITE_DATA 4                         // 单个舵机 写入数据块
#define SERVO_CMD_READ_BATCH_DATA 5                    // 单个舵机 批次读取(读取一个舵机所有的数据)
#define SERVO_CMD_WRITE_BATCH_DATA 6                   // 单个舵机 批次写入(写入一个舵机所有的数据)
#define SERVO_CMD_SPIN 7                               // 单个舵机 设置轮式模式
#define SERVO_CMD_ROTATE 8                             // 角度控制模式(设置舵机的角度))
#define SERVO_CMD_DAMPING 9                            // 阻尼模式
#define SERVO_CMD_READ_ANGLE 10                        // 舵机角度读取
#define SERVO_CMD_SET_ANGLE_BY_INTERVAL 11       // 角度设置(指定周期)
#define SERVO_CMD_SET_ANGLE_BY_VELOCITY 12       // 角度设置(指定转速)
#define SERVO_CMD_SET_ANGLE_MTURN 13             // 多圈角度设置
#define SERVO_CMD_SET_ANGLE_MTURN_BY_INTERVAL 14 // 多圈角度设置(指定周期)
#define SERVO_CMD_SET_ANGLE_MTURN_BY_VELOCITY 15 // 多圈角度设置(指定转速)
#define SERVO_CMD_QUERY_ANGLE_MTURN 16           // 查询舵机角度(多圈)
#define SERVO_CMD_RESET_ANGLE_MTURN 17          // 重置舵机多圈角度
#define SERVO_CMD_BEGIN_ASYNC 18                       // 开始异步命令
#define SERVO_CMD_END_ASYNC 19                      		// 结束异步命令
#define SERVO_CMD_SET_SERVO_ReadData 22                // 舵机数据监控
#define SERVO_CMD_SET_ORIGIN_POINT 23                  // 设置零点
#define SERVO_CMD_CONTROL_MODE_STOP 24                 // 控制模式停止指令
#define SERVO_CMD_SET_SERVO_SyncCommand 25             // 同步命令

// SERVO状态码
#define SERVO_STATUS uint8_t
#define SERVO_STATUS_SUCCESS 0               // 设置/读取成功
#define SERVO_STATUS_FAIL 1                  // 设置/读取失败
#define SERVO_STATUS_TIMEOUT 2               // 等待超时
#define SERVO_STATUS_WRONG_RESPONSE_HEADER 3 // 响应头不对
#define SERVO_STATUS_UNKNOWN_CMD_ID 4         // 未知的控制指令
#define SERVO_STATUS_SIZE_TOO_BIG 5          // 参数的size大于SERVO_PACK_RESPONSE_MAX_SIZE里面的限制
#define SERVO_STATUS_CHECKSUM_ERROR 6        // 校验和错误
#define SERVO_STATUS_ID_NOT_MATCH 7          // 请求的舵机ID跟反馈回来的舵机ID不匹配
#define SERVO_STATUS_ERROR 8                  // 设置同步模式错误

// 静止状态判断条件
#define SERVO_ANGLE_DEADAREA 2.0f  // 电机角度死区
#define SERVO_WAIT_COUNT_MAX 10000 // 等待重复查询角度的最大次数

/* 舵机只读数据ID及使用说明 (只读)*/
#define SERVO_PARAM_VOLTAGE 1    // 电压 (单位mV)
#define SERVO_PARAM_CURRENT 2    // 电流 (单位mA)
#define SERVO_PARAM_POWER 3      // 功率 (单位mw)
#define SERVO_PARAM_TEMPRATURE 4 // 温度 (单位ADC)

/*
舵机工作状态
BIT[0] - 执行指令置1，执行完成后清零。
BIT[1] - 执行指令错误置1，在下次正确执行后清零。
BIT[2] - 堵转错误置1，解除堵转后清零。
BIT[3] - 电压过高置1，电压恢复正常后清零。
BIT[4] - 电压过低置1，电压恢复正常后清零。
BIT[5] - 电流错误置1，电流恢复正常后清零。
BIT[6] - 功率错误置1，功率恢复正常后清零。
BIT[7] - 温度错误置1，温度恢复正常后清零。
*/
#define SERVO_PARAM_SERVO_STATUS 5 // 舵机工作状态 (字节长度 1)

/* 舵机用户自定义参数的数据ID及使用说明 (可度也可写)*/

/* 此项设置同时具备两个功能
 * 在轮式模式与角度控制模式下
 * 1. 舵机指令是否可以中断 interruptable?
 * 2. 是否产生反馈数据?
 * 0x00(默认)
 *      舵机控制指令执行可以被中断, 新的指令覆盖旧的指令
 *      无反馈数据
 * 0x01
 *      舵机控制指令不可以被中断, 新的指令添加到等候队列里面
 *      等候队列的长度是1, 需要自己在程序里面维护一个队列
 *      当新的控制指令超出了缓冲区的大小之后, 新添加的指令被忽略
 *      指令执行结束之后发送反馈数据
 */
#define SERVO_PARAM_RESPONSE_SWITCH 33

/*
 * 舵机的ID号, (字节长度 1)
 * 取值范围是 0-254
 * 255号为广播地址，不能赋值给舵机 广播地址在PING指令中使用。
 */
#define SERVO_PARAM_SERVO_ID 34

/*
 * 串口通讯的波特率ID  (字节长度 1)
 * 取值范围 [0x01,0x07] , 默认值0x05
 * 0x01-9600,
 * 0x02-19200,
 * 0x03-38400,
 * 0x04-57600,
 * 0x05-115200 (默认波特率),
 * 0x06-250000,
 * 0x07-500000,
 * 波特率设置时即生效
 */
#define SERVO_PARAM_BAUDRATE 36

/* 舵机保护值相关设置, 超过阈值舵机就进入保护模式 */
/*
 * 舵机堵转保护模式  (字节长度 1)
 * 0x00-模式1 降功率到堵轉功率上限
 * 0x01-模式2 释放舵机锁力
 */
#define SERVO_PARAM_STALL_PROTECT 37

/* 舵机堵转功率上限, (单位mW) (字节长度 2) */
#define SERVO_PARAM_STALL_POWER_LIMIT 38

/* 舵机电压下限 (单位mV) (字节长度 2) */
#define SERVO_PARAM_OVER_VOLT_LOW 39

/* 舵机电压上限 (单位mV) (字节长度 2) */
#define SERVO_PARAM_OVER_VOLT_HIGH 40

/* 舵机温度上限 (单位 摄氏度) (字节长度 2) */
#define SERVO_PARAM_OVER_TEMPERATURE 41

/* 舵机功率上限 (单位mW) (字节长度 2) */
#define SERVO_PARAM_OVER_POWER 42

/* 舵机电流上限 (单位mA) (字节长度 2) */
#define SERVO_PARAM_OVER_CURRENT 43

/*
 * 舵机启动加速度处理开关 (字节长度 1)
 * 0x00 不启动加速度处理 (无效设置)
 * 0x01 启用加速度处理(默认值)
 *      舵机梯形速度控制,根据时间t推算加速度a
 *      行程前1/4 加速
 *      行程中间1/2保持匀速
 *      行程后1/4
 */
#define SERVO_PARAM_ACCEL_SWITCH 44

/*
 * 舵机上电锁力开关 (字节长度 1)
 * 0x00 上电舵机释放锁力(默认值)
 * 0x11 上电时刹车
 */
#define SERVO_PARAM_POWER_ON_LOCK_SWITCH 46

/*
 * [轮式模式] 轮式模式刹车开关 (字节长度 1)
 * 0x00 停止时舵机释放锁力(默认)
 * 0x01 停止时刹车
 */
#define SERVO_PARAM_WHEEL_MODE_BRAKE_SWITCH 47

/*
 * [角度模式] 角度限制开关 (字节长度 1)
 * 0x00 关闭角度限制
 * 0x01 开启角度限制
 * 注: 只有角度限制模式开启之后, 角度上限下限才有效
 */
#define SERVO_PARAM_ANGLE_LIMIT_SWITCH 48

/*
 * [角度模式] 舵机上电首次角度设置缓慢执行 (字节长度 1)
 * 0x00 关闭
 * 0x01 开启
 * 开启后更安全
 * 缓慢旋转的时间周期即为下方的”舵机上电启动时间“
 */
#define SERVO_PARAM_SOFT_START_SWITCH 49

/*
 * [角度模式] 舵机上电启动时间 (单位ms)(字节长度 2)
 * 默认值: 0x0bb8
 */
#define SERVO_PARAM_SOFT_START_TIME 50

/*
 * [角度模式] 舵机角度上限 (单位0.1度)(字节长度 2)
 */
#define SERVO_PARAM_ANGLE_LIMIT_HIGH 51

/*
 * [角度模式] 舵机角度下限 (单位0.1度)(字节长度 2)
 */
#define SERVO_PARAM_ANGLE_LIMIT_LOW 52

/*
 * [角度模式] 舵机中位角度偏移 (单位0.1度)(字节长度 2)
 */
#define SERVO_PARAM_ANGLE_MID_OFFSET 53

// 帧头接收完成的标志位
#define SERVO_RECV_FLAG_HEADER 0x01
// 控制指令接收完成的标志位
#define SERVO_RECV_FLAG_CMD_ID 0x02
// 内容长度接收完成的标志位
#define SERVO_RECV_FLAG_SIZE 0x04
// 内容接收完成的标志位
#define SERVO_RECV_FLAG_CONTENT 0x08
// 校验和接收的标志位
#define SERVO_RECV_FLAG_CHECKSUM 0x10

// 数据帧结构体（统一结构）
typedef struct {
    uint16_t header;
    uint8_t cmdId;
    uint16_t size;
    uint8_t content[SERVO_PACK_RESPONSE_MAX_SIZE];
    uint8_t checksum;
    uint8_t status;
    uint8_t isSync;
} PackageTypeDef;

// 舵机的数据结构体
typedef struct {
    uint8_t id;
    int16_t voltage;
    int16_t current;
    int16_t power;
    int16_t temperature;
    uint8_t status;
    float angle;
    int16_t circle_count;
} ServoData;

/*同步命令的舵机设置参数结构体*/
typedef struct {
    uint8_t id;
    float angle;
    float velocity;
    uint16_t interval_single;
    uint32_t interval_multi;
    uint16_t t_acc;
    uint16_t t_dec;
    uint16_t power;
} Sync_ServoData;

/*同步命令的模式选择
* 1：设置舵机的角度
* 2：设置舵机的角度(指定周期)
* 3：设置舵机的角度(指定转速)
* 4：设置舵机的角度(多圈模式)
* 5：设置舵机的角度(多圈模式, 指定周期) 
* 6：设置舵机的角度(多圈模式, 指定转速)
* 7：读取舵机的数据*/
typedef enum {
    MODE_SET_SERVO_ANGLE = 1,
    MODE_SET_SERVO_ANGLE_BY_INTERVAL = 2,
    MODE_SET_SERVO_ANGLE_BY_VELOCITY = 3,
    MODE_SET_SERVO_ANGLE_MTURN = 4,
    MODE_SET_SERVO_ANGLE_MTURN_BY_INTERVAL = 5,
    MODE_SET_SERVO_ANGLE_MTURN_BY_VELOCITY = 6,
    MODE_Query_SERVO_Monitor = 7
} ServoMode;



static SERVO_STATUS Servo_SendPackage_Common(uint8_t cmdId, uint16_t size, uint8_t *content, uint8_t isSync);
static uint8_t Servo_CalcChecksum(PackageTypeDef *pkg);

#if SERVO_DLC
extern ServoData servodata[SERVO_MAX_COUNT];
SERVO_STATUS Servo_SetOriginPoint(uint8_t servo_id);
SERVO_STATUS Servo_ResetUserData(uint8_t servo_id);
SERVO_STATUS Servo_ReadData(uint8_t servo_id,  uint8_t address, uint8_t *value, uint8_t *size);
SERVO_STATUS Servo_WriteData(uint8_t servo_id, uint8_t address, uint8_t *value, uint8_t size);
SERVO_STATUS Servo_SetServoAngleByInterval(uint8_t servo_id, 
				float angle, uint16_t interval, uint16_t t_acc, 
				uint16_t t_dec, uint16_t  power);
SERVO_STATUS Servo_SetServoAngleByVelocity(uint8_t servo_id,
				float angle, float velocity, uint16_t t_acc,
				uint16_t t_dec, uint16_t  power);
SERVO_STATUS Servo_QueryServoAngle(uint8_t servo_id, float *angle);
SERVO_STATUS Servo_SetServoAngleMTurn(uint8_t servo_id, float angle, 
	                                uint32_t interval, uint16_t power);
SERVO_STATUS Servo_SetServoAngleMTurnByInterval(uint8_t servo_id, float angle,
				uint32_t interval,  uint16_t t_acc,  uint16_t t_dec, uint16_t power);
SERVO_STATUS Servo_SetServoAngleMTurnByVelocity(uint8_t servo_id, float angle,
		    	float velocity, uint16_t t_acc,  uint16_t t_dec, uint16_t power);
SERVO_STATUS Servo_QueryServoAngleMTurn(uint8_t servo_id, float *angle);
SERVO_STATUS Servo_DampingMode(uint8_t servo_id, uint16_t power);
SERVO_STATUS Servo_ResetServoMTurnAngle(uint8_t servo_id);

#endif
#if SERVO_ASYNC
SERVO_STATUS Servo_BeginAsync(void);
SERVO_STATUS Servo_EndAsync(uint8_t mode);
#endif
#if SERVO_SYNC
SERVO_STATUS Servo_SyncCommand(uint8_t servo_count, uint8_t ServoMode, Sync_ServoData servoSync []);
#endif
#if SERVO_MONITOR
SERVO_STATUS Servo_Monitor(uint8_t servo_id, ServoData servodata[]);
#endif
#if SERVO_SYNC_MONITOR
static SERVO_STATUS Servo_Sync_RecvPackage(PackageTypeDef *pkg);
SERVO_STATUS Servo_SyncServoMonitor(uint8_t servo_count, ServoData servodata[]);
#endif
#if SERVO_PING
SERVO_STATUS Servo_Ping(uint8_t servo_id);
static SERVO_STATUS Servo_RecvPackage(PackageTypeDef *pkg);
static SERVO_STATUS Servo_IsValidResponsePackage(PackageTypeDef *pkg);
#endif

void Servo_Uart_Send(uint8_t* data, uint16_t size);
SERVO_STATUS Servo_SetServoAngle(uint8_t servo_id, float angle, uint16_t interval, uint16_t power);
SERVO_STATUS Servo_StopOnControlMode(uint8_t servo_id, uint8_t mode, uint16_t power);

#ifdef __cplusplus
}
#endif

#endif
