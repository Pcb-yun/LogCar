/**
 * @file servo_driver.c
 * @author MIKE
 * @brief Fashion Star总线伺服舵机FreeRTOS驱动层
 */

#include "servo_driver.h"
#include "log.h"
#include "stream_buffer.h"
#include "usart.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Events.h"

static uint8_t Servo_CalcChecksum(Package_t *pkg);
static SERVO_STATUS Servo_SendPackage_Common(uint8_t cmdId, uint16_t size, uint8_t *content, uint8_t isSync);
StreamBufferHandle_t Servo_Rx_StreamHandle = NULL;

#if SERVO_ASYNC || SERVO_SYNC || SERVO_MONITOR || SERVO_SYNC_MONITOR
ServoData servodata[SERVO_MAX_COUNT];
#endif

/**
 * @brief 从接收流缓冲区读取一个字节
 * @param byte 读取到的字节
 * @param timeout_ms 超时时间(ms)
 * @return SERVO_STATUS 状态码
 */
static SERVO_STATUS Servo_ReadByte(uint8_t *byte,uint32_t timeout_ms){
    size_t n = xStreamBufferReceive(Servo_Rx_StreamHandle,byte,1,pdMS_TO_TICKS(timeout_ms));
    return n == 1 ? SERVO_STATUS_SUCCESS : SERVO_STATUS_TIMEOUT;
}

/*舵机进阶模式*/
#if SERVO_ADVANCED_MODE
/**
 * @brief 设置伺服角度(指定周期)
 * @param servo_id 伺服ID
 * @param angle 角度
 * @param interval 间隔时间
 * @param t_acc 加速度时间
 * @param t_dec 减速度时间
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SetServoAngleByInterval(uint8_t servo_id,
				float angle, uint16_t interval, uint16_t t_acc,
				uint16_t t_dec, uint16_t  power){

	const uint8_t size = 11;

	// 数值约束
	if(angle > 180.0f){
		angle = 180.0f;
	}else if(angle < -180.0f){
		angle = -180.0f;
	}
	if (t_acc < 20){
		t_acc = 20;
	}
	if (t_dec < 20){
		t_dec = 20;
	}
    int16_t scaledAngle = (int16_t)(10*angle);

	// 协议打包
    uint8_t content[size];
	content[0] = servo_id;                           // 1字节：舵机ID
	content[1] = scaledAngle & 0xFF;                 // 角度低字节
	content[2] = (scaledAngle >> 8) & 0xFF;          // 角度高字节
	content[3] = interval & 0xFF;                    // interval低字节
	content[4] = (interval >> 8) & 0xFF;             // interval高字节
	content[5] = t_acc & 0xFF;                       // t_acc低字节
	content[6] = (t_acc >> 8) & 0xFF;                // t_acc高字节
	content[7] = t_dec & 0xFF;                       // t_dec低字节
	content[8] = (t_dec >> 8) & 0xFF;                // t_dec高字节
	content[9] = power & 0xFF;                       // power低字节
	content[10] = (power >> 8) & 0xFF;               // power高字节

	// 发送请求包
	Servo_SendPackage_Common(SERVO_CMD_SET_ANGLE_BY_INTERVAL, size, content,0);

	return SERVO_STATUS_SUCCESS;
}

/**
 * @brief 设置伺服角度(指定转速)
 * @param servo_id 伺服ID
 * @param angle 角度
 * @param velocity 速度
 * @param t_acc 加速度时间
 * @param t_dec 减速度时间
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SetServoAngleByVelocity(uint8_t servo_id,
				float angle, float velocity, uint16_t t_acc,
				uint16_t t_dec, uint16_t  power){
	// 创建环形缓冲队列
	const uint8_t size = 11;

	// 数值约束
	if(angle > 180.0f){
		angle = 180.0f;
	}else if(angle < -180.0f){
		angle = -180.0f;
	}
	if(velocity < 1.0f){
		velocity = 1.0f;
	}else if(velocity > 750.0f){
		velocity = 750.0f;
	}
	if(t_acc < 20){
		t_acc = 20;
	}
	if(t_dec < 20){
		t_dec = 20;
	}
	int16_t scaledAngle = (int16_t)(10*angle);
	uint16_t scaledVelocity = (uint16_t)(10*velocity);

	// 协议打包
    uint8_t content[size];
	content[0] = servo_id;                           // 1字节：舵机ID
	content[1] = scaledAngle & 0xFF;                 // 角度低字节
	content[2] = (scaledAngle >> 8) & 0xFF;          // 角度高字节
	content[3] = scaledVelocity & 0xFF;                    // velocity低字节
	content[4] = (scaledVelocity >> 8) & 0xFF;             // velocity高字节
	content[5] = t_acc & 0xFF;                       // t_acc低字节
	content[6] = (t_acc >> 8) & 0xFF;                // t_acc高字节
	content[7] = t_dec & 0xFF;                       // t_dec低字节
	content[8] = (t_dec >> 8) & 0xFF;                // t_dec高字节
	content[9] = power & 0xFF;                       // power低字节
	content[10] = (power >> 8) & 0xFF;               // power高字节

	// 发送请求包
	Servo_SendPackage_Common(SERVO_CMD_SET_ANGLE_BY_VELOCITY, size, content,0);

	return SERVO_STATUS_SUCCESS;
}

/**
 * @brief 设置伺服角度(多圈模式, 指定周期)
 * @param servo_id 伺服ID
 * @param angle 角度
 * @param interval 间隔时间
 * @param t_acc 加速度时间
 * @param t_dec 减速度时间
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SetServoAngleMTurnByInterval(uint8_t servo_id, float angle,
		    	uint32_t interval,  uint16_t t_acc,  uint16_t t_dec, uint16_t power){

	const uint8_t size = 15;

	// 数值约束
	if(angle > 368640.0f){
		angle = 368640.0f;
	}else if(angle < -368640.0f){
		angle = -368640.0f;
	}
	if(interval > 4096000){
		interval = 4096000;
	}
	if(t_acc < 20){
		t_acc = 20;
	}
	if(t_dec < 20){
		t_dec = 20;
	}

    int32_t scaledAngle = (int32_t)(10 * angle);

	// 协议打包
    uint8_t content[size];
	content[0] = servo_id;                                    // 1字节：舵机ID
	content[1] = scaledAngle & 0xFF;                          // 角度字节0
	content[2] = (scaledAngle >> 8) & 0xFF;                   // 角度字节1
	content[3] = (scaledAngle >> 16) & 0xFF;                  // 角度字节2
	content[4] = (scaledAngle >> 24) & 0xFF;                  // 角度字节3
	content[5] = interval & 0xFF;                             // interval字节0
	content[6] = (interval >> 8) & 0xFF;                      // interval字节1
	content[7] = (interval >> 16) & 0xFF;                     // interval字节2
	content[8] = (interval >> 24) & 0xFF;                     // interval字节3
	content[9] = t_acc & 0xFF;                                // t_acc低字节
	content[10] = (t_acc >> 8) & 0xFF;                        // t_acc高字节
	content[11] = t_dec & 0xFF;                               // t_dec低字节
	content[12] = (t_dec >> 8) & 0xFF;                        // t_dec高字节
	content[13] = power & 0xFF;                               // power低字节
	content[14] = (power >> 8) & 0xFF;                        // power高字节

	// 发送请求包
    return Servo_SendPackage_Common(SERVO_CMD_SET_ANGLE_MTURN_BY_INTERVAL, size, content,0);
}

/**
 * @brief 设置舵机的角度(多圈模式, 指定转速)
 * @param servo_id 伺服ID
 * @param angle 角度
 * @param velocity 转速
 * @param t_acc 加速度时间
 * @param t_dec 减速度时间
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SetServoAngleMTurnByVelocity(uint8_t servo_id, float angle,
			    float velocity, uint16_t t_acc,  uint16_t t_dec, uint16_t power){
	// 创建环形缓冲队列
	const uint8_t size = 13;

	// 数值约束
	if(angle > 368640.0f){
		angle = 368640.0f;
	}else if(angle < -368640.0f){
		angle = -368640.0f;
	}
	if(velocity < 1.0f){
		velocity = 1.0f;
	}else if(velocity > 750.0f){
		velocity = 750.0f;
	}
	if(t_acc < 20){
		t_acc = 20;
	}
	if(t_dec < 20){
		t_dec = 20;
	}
    int32_t scaledAngle = (int32_t)(10 * angle);
    int16_t scaledVelocity = (int16_t)(10 * velocity);

	// 协议打包
    uint8_t content[size];
	content[0] = servo_id;                                    // 1字节：舵机ID
	content[1] = scaledAngle & 0xFF;                          // 角度字节0
	content[2] = (scaledAngle >> 8) & 0xFF;                   // 角度字节1
	content[3] = (scaledAngle >> 16) & 0xFF;                  // 角度字节2
	content[4] = (scaledAngle >> 24) & 0xFF;                  // 角度字节3
	content[5] = scaledVelocity & 0xFF;                       // 速度低字节
	content[6] = (scaledVelocity >> 8) & 0xFF;                // 速度高字节
	content[7] = t_acc & 0xFF;                                // t_acc低字节
	content[8] = (t_acc >> 8) & 0xFF;                         // t_acc高字节
	content[9] = t_dec & 0xFF;                                // t_dec低字节
	content[10] = (t_dec >> 8) & 0xFF;                        // t_dec高字节
	content[11] = power & 0xFF;                               // power低字节
	content[12] = (power >> 8) & 0xFF;                        // power高字节

    // 发送请求包
    return Servo_SendPackage_Common(SERVO_CMD_SET_ANGLE_MTURN_BY_VELOCITY, size, content,0);
}

#endif
/*舵机其他功能*/
#if SERVO_DLC
/**
 * @brief 重置舵机的用户资料
 * @param servo_id 伺服ID
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_ResetUserData(uint8_t servo_id){
	const uint8_t size = 1;
	SERVO_STATUS statusCode;
	// 发送请求包
	Servo_SendPackage_Common(SERVO_CMD_RESET_USER_DATA, size, &servo_id,0);
	// 接收重置结果
	Package_t pkg;
	statusCode = Servo_RecvPackage(&pkg);
	if (statusCode == SERVO_STATUS_SUCCESS){
		// 成功的接收到反馈数据
		// 读取反馈数据中的result
		uint8_t result = (uint8_t)pkg.content[1];
		if (result == 1){
			return SERVO_STATUS_SUCCESS;
		}else{
			return SERVO_STATUS_FAIL;
		}
	}
	return statusCode;
}

/**
 * @brief 读取伺服数据
 * @param servo_id 伺服ID
 * @param address 地址
 * @param value 数据指针
 * @param size 数据大小指针
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_ReadData(uint8_t servo_id,  uint8_t address, uint8_t *value, uint8_t *size){
    SERVO_STATUS statusCode;
    uint8_t buffer[2] = {servo_id, address};
    Servo_SendPackage_Common(SERVO_CMD_READ_DATA, 2, buffer,0);
    Package_t pkg;
    statusCode = Servo_RecvPackage(&pkg);
    if(statusCode == SERVO_STATUS_SUCCESS){
        // 读取数据
		// 读取数据是多少个位
		*size = pkg.size - 2; // content的长度减去servo_id跟address的长度
		// 数据拷贝
		for (int i=0; i<*size; i++){
			value[i] = pkg.content[i+2];
		}
    }
    return statusCode;
}

/**
 * @brief 写入伺服数据
 * @param servo_id 伺服ID
 * @param address 地址
 * @param value 数据指针
 * @param size 数据大小
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_WriteData(uint8_t servo_id, uint8_t address, uint8_t *value, uint8_t size){
	SERVO_STATUS statusCode;
	// 构造content
	uint8_t buffer[size+2]; // 舵机ID + 地址位Address + 数据byte数
	buffer[0] = servo_id;
	buffer[1] = address;
	// 拷贝数据
	for (int i=0; i<size; i++){
		buffer[i+2] = value[i];
	}
	// 发送请求数据
	Servo_SendPackage_Common(SERVO_CMD_WRITE_DATA, size+2, buffer,0);
	// 接收返回信息
	Package_t pkg;
	statusCode = Servo_RecvPackage(&pkg);
	if (statusCode == SERVO_STATUS_SUCCESS){
		uint8_t result = pkg.content[2];
		if(result == 1){
			statusCode = SERVO_STATUS_SUCCESS;
		}else{
			statusCode = SERVO_STATUS_FAIL;
		}
	}
	return statusCode;
}

/**
 * @brief 查询伺服角度
 * @param servo_id 伺服ID
 * @param angle 角度指针
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_QueryServoAngle(uint8_t servo_id, float *angle){
	const uint8_t size = 1; // 请求包content的长度
	uint8_t ehcoServoId;
	int16_t echoAngle;

	// 发送舵机角度请求包
	Servo_SendPackage_Common(SERVO_CMD_READ_ANGLE, size, &servo_id,0);
	// 接收返回的Ping
	Package_t pkg;
	uint8_t statusCode = Servo_RecvPackage(&pkg);
	if (statusCode == SERVO_STATUS_SUCCESS){
		// 成功的获取到舵机角度回读数据
		ehcoServoId = (uint8_t)pkg.content[0];
		// 检测舵机ID是否匹配
		if (ehcoServoId != servo_id){
			// 反馈得到的舵机ID号不匹配
			return SERVO_STATUS_ID_NOT_MATCH;
		}

		// 提取舵机角度
		echoAngle = (int16_t)(pkg.content[1] | (pkg.content[2] << 8));
		*angle = (float)echoAngle / 10.0f;
	}
  return statusCode;
}

/**
 * @brief 查询舵机的角度(多圈模式)
 * @param servo_id 伺服ID
 * @param angle 角度指针
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_QueryServoAngleMTurn(uint8_t servo_id, float *angle){
	// 创建环形缓冲队列
	const uint8_t size = 1; // 请求包content的长度
	uint8_t ehcoServoId;
	int32_t echoAngle;

	// 发送舵机角度请求包
	Servo_SendPackage_Common(SERVO_CMD_QUERY_ANGLE_MTURN, size, &servo_id,0);
	// 接收返回的Ping
	Package_t pkg;
	uint8_t statusCode = Servo_RecvPackage(&pkg);
	if (statusCode == SERVO_STATUS_SUCCESS){
		// 成功的获取到舵机角度回读数据
		ehcoServoId = (uint8_t)pkg.content[0];
		// 检测舵机ID是否匹配
		if (ehcoServoId != servo_id){
			// 反馈得到的舵机ID号不匹配
			return SERVO_STATUS_ID_NOT_MATCH;
		}

		// 提取舵机角度
		echoAngle = (int32_t)(pkg.content[1] | (pkg.content[2] << 8) |  (pkg.content[3] << 16) | (pkg.content[4] << 24));
		*angle = (float)echoAngle / 10.0f;
	}
  return statusCode;
}

/**
 * @brief 舵机阻尼模式
 * @param servo_id 伺服ID
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_DampingMode(uint8_t servo_id, uint16_t power){
	const uint8_t size = 3; // 请求包content的长度

	// 构造content
    uint8_t content[size];
	content[0] = servo_id;
	content[1] = power & 0xFF;
	content[2] = (power >> 8) & 0xFF;
    // 发送请求包
    return Servo_SendPackage_Common(SERVO_CMD_DAMPING, size, content,0);
}

/**
 * @brief 舵机重置多圈角度圈数
 * @param servo_id 伺服ID
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_ResetServoMTurnAngle(uint8_t servo_id){
    SERVO_STATUS statusCode; // 状态码
	Package_t pkg;

	Servo_SendPackage_Common(SERVO_CMD_RESET_ANGLE_MTURN, 1, &servo_id,0);
	// 接收返回的Ping
	statusCode = Servo_RecvPackage(&pkg);
	return statusCode;
}

/**
 * @brief 零点设置 仅适用于无刷磁编码舵机
 * @param servo_id 伺服ID
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SetOriginPoint(uint8_t servo_id){
	SERVO_STATUS statusCode; // 状态码
	Package_t pkg;

	Servo_SendPackage_Common(SERVO_CMD_SET_ORIGIN_POINT, 1, &servo_id,0);
	// 接收返回的Ping
	statusCode = Servo_RecvPackage(&pkg);
	return statusCode;
}
#endif
/*异步命令*/
#if SERVO_ASYNC
/**
 * @brief 舵机开始异步命令
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_BeginAsync(void)
{
	const uint8_t size = 0; // 请求包content的长度
	// 构造content
	uint8_t *content = NULL;
		// 发送请求包
	Servo_SendPackage_Common(SERVO_CMD_BEGIN_ASYNC, size, content,0);
	return SERVO_STATUS_SUCCESS;
}

/**
 * @brief 舵机结束异步命令
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_EndAsync(uint8_t mode)
{
/*mode
	0:执行存储的命令
	1:取消存储的命令
*/
	const uint8_t size = 1; // 请求包content的长度
	uint8_t content[size];
	content[0] = mode;
		// 发送请求包
	Servo_SendPackage_Common(SERVO_CMD_END_ASYNC, size, content,0);
	return SERVO_STATUS_SUCCESS;
}
#endif
/*同步命令*/
#if SERVO_SYNC
/**
 * @brief 舵机同步命令选择模式控制函数
 * @param servo_count 舵机同步数量
 * @param ServoMode 同步命令模式选择
 * @param servoSync[] 舵机参数结构体数组
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SyncCommand(uint8_t servo_count, uint8_t ServoMode, Sync_ServoData servoSync[]) {

    uint16_t size;
    uint8_t *content = NULL;

    switch (ServoMode) {
        case MODE_SET_SERVO_ANGLE:
            /* 同步命令设置舵机的角度(单圈模式）*/
            size = 3 + servo_count * 7;
            content = (uint8_t *)pvPortMalloc(size);
            if (content == NULL) return SERVO_STATUS_FAIL;

            content[0] = 8;     // 指令标识
            content[1] = 7;     // 参数长度
            content[2] = servo_count;  // 舵机数量

            for (int i = 0; i < servo_count; i++) {
                int16_t angle;
                if (servoSync[i].angle > 180.0f) {
                    angle = (int16_t)(10 * 180.0f);
                } else if (servoSync[i].angle < -180.0f) {
                    angle = (int16_t)(10 * -180.0f);
                } else {
                    angle = (int16_t)(10 * servoSync[i].angle);
                }
                uint16_t interval = servoSync[i].interval_single;
                uint16_t power = servoSync[i].power;

                uint8_t *p = content + 3 + i * 7;
                p[0] = servoSync[i].id;
                p[1] = angle & 0xFF;
                p[2] = (angle >> 8) & 0xFF;
                p[3] = interval & 0xFF;
                p[4] = (interval >> 8) & 0xFF;
                p[5] = power & 0xFF;
                p[6] = (power >> 8) & 0xFF;
            }
            break;

        case MODE_SET_SERVO_ANGLE_BY_INTERVAL:
            /* 同步命令设置舵机的角度(单圈模式，指定周期) */
            size = 3 + servo_count * 11;
            content = (uint8_t *)pvPortMalloc(size);
            if (content == NULL) return SERVO_STATUS_FAIL;

            content[0] = 11;
            content[1] = 11;
            content[2] = servo_count;

            for (int i = 0; i < servo_count; i++) {
                int16_t angle;
                if (servoSync[i].angle > 180.0f) {
                    angle = (int16_t)(10 * 180.0f);
                } else if (servoSync[i].angle < -180.0f) {
                    angle = (int16_t)(10 * -180.0f);
                } else {
                    angle = (int16_t)(10 * servoSync[i].angle);
                }

                uint8_t *p = content + 3 + i * 11;
                p[0] = servoSync[i].id;
                p[1] = angle & 0xFF;
                p[2] = (angle >> 8) & 0xFF;
                p[3] = servoSync[i].interval_single & 0xFF;
                p[4] = (servoSync[i].interval_single >> 8) & 0xFF;
                p[5] = servoSync[i].t_acc & 0xFF;
                p[6] = (servoSync[i].t_acc >> 8) & 0xFF;
                p[7] = servoSync[i].t_dec & 0xFF;
                p[8] = (servoSync[i].t_dec >> 8) & 0xFF;
                p[9] = servoSync[i].power & 0xFF;
                p[10] = (servoSync[i].power >> 8) & 0xFF;
            }
            break;

        case MODE_SET_SERVO_ANGLE_BY_VELOCITY:
            /* 同步命令设置舵机的角度(单圈模式，指定转速) */
            size = 3 + servo_count * 11;
            content = (uint8_t *)pvPortMalloc(size);
            if (content == NULL) return SERVO_STATUS_FAIL;

            content[0] = 12;
            content[1] = 11;
            content[2] = servo_count;

            for (int i = 0; i < servo_count; i++) {
                int16_t angle;
                if (servoSync[i].angle > 180.0f) {
                    angle = (int16_t)(10 * 180.0f);
                } else if (servoSync[i].angle < -180.0f) {
                    angle = (int16_t)(10 * -180.0f);
                } else {
                    angle = (int16_t)(10 * servoSync[i].angle);
                }
                uint16_t velocity = (uint16_t)(10.0f * servoSync[i].velocity);

                uint8_t *p = content + 3 + i * 11;
                p[0] = servoSync[i].id;
                p[1] = angle & 0xFF;
                p[2] = (angle >> 8) & 0xFF;
                p[3] = velocity & 0xFF;
                p[4] = (velocity >> 8) & 0xFF;
                p[5] = servoSync[i].t_acc & 0xFF;
                p[6] = (servoSync[i].t_acc >> 8) & 0xFF;
                p[7] = servoSync[i].t_dec & 0xFF;
                p[8] = (servoSync[i].t_dec >> 8) & 0xFF;
                p[9] = servoSync[i].power & 0xFF;
                p[10] = (servoSync[i].power >> 8) & 0xFF;
            }
            break;

        case MODE_SET_SERVO_ANGLE_MTURN:
            /* 同步命令设置舵机的角度(多圈模式) */
            size = 3 + servo_count * 11;
            content = (uint8_t *)pvPortMalloc(size);
            if (content == NULL) return SERVO_STATUS_FAIL;

            content[0] = 13;
            content[1] = 11;
            content[2] = servo_count;

            for (int i = 0; i < servo_count; i++) {
                int32_t angle;
                if (servoSync[i].angle > 368640.0f) {
                    angle = (int32_t)(10 * 368640.0f);
                } else if (servoSync[i].angle < -368640.0f) {
                    angle = (int32_t)(10 * -368640.0f);
                } else {
                    angle = (int32_t)(10 * servoSync[i].angle);
                }

                uint8_t *p = content + 3 + i * 11;
                p[0] = servoSync[i].id;
                p[1] = angle & 0xFF;
                p[2] = (angle >> 8) & 0xFF;
                p[3] = (angle >> 16) & 0xFF;
                p[4] = (angle >> 24) & 0xFF;
                p[5] = servoSync[i].interval_multi & 0xFF;
                p[6] = (servoSync[i].interval_multi >> 8) & 0xFF;
                p[7] = (servoSync[i].interval_multi >> 16) & 0xFF;
                p[8] = (servoSync[i].interval_multi >> 24) & 0xFF;
                p[9] = servoSync[i].power & 0xFF;
                p[10] = (servoSync[i].power >> 8) & 0xFF;
            }
            break;

        case MODE_SET_SERVO_ANGLE_MTURN_BY_INTERVAL:
            /* 同步命令设置舵机的角度(多圈模式, 指定周期) */
            size = 3 + servo_count * 15;
            content = (uint8_t *)pvPortMalloc(size);
            if (content == NULL) return SERVO_STATUS_FAIL;

            content[0] = 14;
            content[1] = 15;
            content[2] = servo_count;

            for (int i = 0; i < servo_count; i++) {
                int32_t angle;
                if (servoSync[i].angle > 368640.0f) {
                    angle = (int32_t)(10 * 368640.0f);
                } else if (servoSync[i].angle < -368640.0f) {
                    angle = (int32_t)(10 * -368640.0f);
                } else {
                    angle = (int32_t)(10 * servoSync[i].angle);
                }

                uint8_t *p = content + 3 + i * 15;
                p[0] = servoSync[i].id;
                p[1] = angle & 0xFF;
                p[2] = (angle >> 8) & 0xFF;
                p[3] = (angle >> 16) & 0xFF;
                p[4] = (angle >> 24) & 0xFF;
                p[5] = servoSync[i].interval_multi & 0xFF;
                p[6] = (servoSync[i].interval_multi >> 8) & 0xFF;
                p[7] = (servoSync[i].interval_multi >> 16) & 0xFF;
                p[8] = (servoSync[i].interval_multi >> 24) & 0xFF;
                p[9] = servoSync[i].t_acc & 0xFF;
                p[10] = (servoSync[i].t_acc >> 8) & 0xFF;
                p[11] = servoSync[i].t_dec & 0xFF;
                p[12] = (servoSync[i].t_dec >> 8) & 0xFF;
                p[13] = servoSync[i].power & 0xFF;
                p[14] = (servoSync[i].power >> 8) & 0xFF;
            }
            break;

        case MODE_SET_SERVO_ANGLE_MTURN_BY_VELOCITY:
            /* 同步命令设置舵机的角度(多圈模式, 指定转速) */
            size = 3 + servo_count * 13;
            content = (uint8_t *)pvPortMalloc(size);
            if (content == NULL) return SERVO_STATUS_FAIL;

            content[0] = 15;
            content[1] = 13;
            content[2] = servo_count;

            for (int i = 0; i < servo_count; i++) {
                int32_t angle;
                if (servoSync[i].angle > 368640.0f) {
                    angle = (int32_t)(10 * 368640.0f);
                } else if (servoSync[i].angle < -368640.0f) {
                    angle = (int32_t)(10 * -368640.0f);
                } else {
                    angle = (int32_t)(10 * servoSync[i].angle);
                }
                uint16_t velocity = (uint16_t)(10.0f * servoSync[i].velocity);

                uint8_t *p = content + 3 + i * 13;
                p[0] = servoSync[i].id;
                p[1] = angle & 0xFF;
                p[2] = (angle >> 8) & 0xFF;
                p[3] = (angle >> 16) & 0xFF;
                p[4] = (angle >> 24) & 0xFF;
                p[5] = velocity & 0xFF;
                p[6] = (velocity >> 8) & 0xFF;
                p[7] = servoSync[i].t_acc & 0xFF;
                p[8] = (servoSync[i].t_acc >> 8) & 0xFF;
                p[9] = servoSync[i].t_dec & 0xFF;
                p[10] = (servoSync[i].t_dec >> 8) & 0xFF;
                p[11] = servoSync[i].power & 0xFF;
                p[12] = (servoSync[i].power >> 8) & 0xFF;
            }
            break;

        case MODE_Query_SERVO_Monitor:
            /* 同步命令读取舵机的数据 */
            size = 3 + servo_count;
            content = (uint8_t *)pvPortMalloc(size);
            if (content == NULL) return SERVO_STATUS_FAIL;

            content[0] = 22;
            content[1] = 1;
            content[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                content[3 + i] = servoSync[i].id;
            }
            break;

        default:
            return SERVO_STATUS_ERROR;
    }

    // 发送请求包（根据size是否超过255决定isSync）
    uint8_t isSync = (size > 255) ? 1 : 0;
    Servo_SendPackage_Common(SERVO_CMD_SET_SERVO_SyncCommand, size, content, isSync);

    // 如果是监控模式，接收响应数据
    if (ServoMode == MODE_Query_SERVO_Monitor) {
        Servo_SyncServoMonitor(servo_count, servodata);
    }

    // 释放动态分配的内存
    vPortFree(content);

    return SERVO_STATUS_SUCCESS;
}
#endif

/*Ping命令*/
#if SERVO_PING
static SERVO_STATUS Servo_SendPackage_Common(uint8_t cmdId, uint16_t size, uint8_t *content, uint8_t isSync);
/**
 * @brief 验证伺服数据包有效性
 * @param pkg 数据包指针
 * @return SERVO_STATUS 状态码
 */
static SERVO_STATUS Servo_IsValidResponsePackage(Package_t *pkg) {
    if (pkg->header != SERVO_PACK_RESPONSE_HEADER)
        return SERVO_STATUS_WRONG_RESPONSE_HEADER;
    if (pkg->cmdId > SERVO_CMD_NUM)
        return SERVO_STATUS_UNKNOWN_CMD_ID;
    if (pkg->size > SERVO_PACK_RESPONSE_MAX_SIZE)
        return SERVO_STATUS_SIZE_TOO_BIG;
    if (Servo_CalcChecksum(pkg) != pkg->checksum)
        return SERVO_STATUS_CHECKSUM_ERROR;
    return SERVO_STATUS_SUCCESS;
}

/**
 * @brief 接收伺服数据包
 * @param pkg 数据包指针
 * @return SERVO_STATUS 状态码
 */
static SERVO_STATUS Servo_RecvPackage(Package_t *pkg) {
    uint8_t byte  = 0;
    uint8_t header_byte1 = 0;
    uint32_t startTime = osKernelGetTickCount();

    memset(pkg, 0, sizeof(Package_t));

    while ((osKernelGetTickCount() - startTime) < 100) {
     /* 1. 查找帧头 —— [CHANGE] 内层循环加总超时检查，防止死循环 */
        while ((osKernelGetTickCount() - startTime) < 100 &&
               Servo_ReadByte(&byte, 10) == SERVO_STATUS_SUCCESS)
        {
            if (header_byte1 == 0 && byte == 0x05) {
                header_byte1 = byte;
            } else if (header_byte1 == 0x05 && byte == 0x1C) {
                pkg->header = 0x1C05;
                break;
            } else {
                header_byte1 = 0;
            }
        }

        if (pkg->header != SERVO_PACK_RESPONSE_HEADER) {
            continue;
        }

        /* 2. 接收 cmdId */
        if (Servo_ReadByte(&pkg->cmdId, 10) != SERVO_STATUS_SUCCESS) {
            pkg->header = 0;
            continue;
        }

        /* 3. 接收 size（判断同步模式） */
        if (Servo_ReadByte(&byte, 10) != SERVO_STATUS_SUCCESS) {
            pkg->header = 0;
            continue;
        }

        if (byte == 0xFF) {
            /* 同步模式：size 占 2 字节 */
            pkg->isSync = 1;
            uint8_t sizeLow, sizeHigh;
            if (Servo_ReadByte(&sizeLow, 10) != SERVO_STATUS_SUCCESS)  { pkg->header = 0; continue; }
            if (Servo_ReadByte(&sizeHigh, 10) != SERVO_STATUS_SUCCESS) { pkg->header = 0; continue; }
            pkg->size = sizeLow | (sizeHigh << 8);
        } else {
            /* 普通模式：size 占 1 字节 */
            pkg->isSync = 0;
            pkg->size = byte;
        }

        /* 4. 检查 size 有效性 */
        if (pkg->size == 0 || pkg->size > SERVO_PACK_RESPONSE_MAX_SIZE) {
            pkg->header = 0;
            continue;
        }

        /* 5. [CHANGE] content 一次性批量读取 */
        size_t received = xStreamBufferReceive(
            Servo_Rx_StreamHandle,
            pkg->content,
            pkg->size,
            pdMS_TO_TICKS(10)
        );
        if (received < pkg->size) {
            pkg->header = 0;
            continue;
        }

        /* 6. 接收 checksum */
        if (Servo_ReadByte(&pkg->checksum, 10) != SERVO_STATUS_SUCCESS) {
            pkg->header = 0;
            continue;
        }

        /* 7. 验证包有效性 */
        return Servo_IsValidResponsePackage(pkg);
    }

    return SERVO_STATUS_TIMEOUT;

}

/**
 * @brief 舵机通讯检测
 * @param servo_id 伺服ID
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_Ping(uint8_t servo_id){
	SERVO_STATUS statusCode; // 状态码
	uint8_t ehcoServoId; // PING得到的舵机ID
	// printf("[PING]Send Ping Package\r\n");
	// 发送请求包
	Servo_SendPackage_Common(SERVO_CMD_PING, 1, &servo_id,0);
	// 接收返回的Ping
	Package_t pkg = {0};
	statusCode = Servo_RecvPackage(&pkg);
	if(statusCode == SERVO_STATUS_SUCCESS){
		// 进一步检查ID号是否匹配
		ehcoServoId = (uint8_t)pkg.content[0];
		if (ehcoServoId != servo_id){
			// 反馈得到的舵机ID号不匹配
			return SERVO_STATUS_ID_NOT_MATCH;
		}
	}
	return statusCode;
}
#endif

/*监控命令*/
#if SERVO_MONITOR
/**
 * @brief 舵机单个数据监控
 * @param servo_id 伺服ID
 * @param servodata 伺服数据指针
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_Monitor(uint8_t servo_id, ServoData servodata[]) {

	// 创建环形缓冲队列
	const uint8_t size = 1;

    uint8_t content[size];
	content[0] = servo_id;

	double temp;//温度数据转换
	Servo_SendPackage_Common(SERVO_CMD_SET_SERVO_ReadData,(uint8_t)size, content,0);

    Package_t pkg;
    SERVO_STATUS status=Servo_RecvPackage(&pkg);

   if (status != SERVO_STATUS_SUCCESS) {
       return status;  // 如果接收失败，返回错误状态
       }

        // 解析当前数据包内容
        servodata[0].id = pkg.content[0];
        servodata[0].voltage = (int16_t)((pkg.content[2] << 8) | pkg.content[1]);
        servodata[0].current = (int16_t)((pkg.content[4] << 8) | pkg.content[3]);
        servodata[0].power = (int16_t)((pkg.content[6] << 8) | pkg.content[5]);
        servodata[0].temperature = (int16_t)((pkg.content[8] << 8) | pkg.content[7]);
				temp = (double)servodata[0].temperature;
				servodata[0].temperature = 1 / (log(temp / (4096.0f - temp)) / 3435.0f + 1 / (273.15 + 25)) - 273.15;
        servodata[0].status = pkg.content[9];
        servodata[0].angle = (int32_t)((pkg.content[13] << 24) | (pkg.content[12] << 16) | (pkg.content[11] << 8) | pkg.content[10]);
        servodata[0].angle = (float)(servodata[0].angle/10.0f);
			  servodata[0].circle_count = (int16_t)((pkg.content[15] << 8) | pkg.content[14]);

        // 重置 pkg 状态以接收下一组数据
        pkg.status = 0;

    return SERVO_STATUS_SUCCESS;
}
#endif
/*同步监控命令*/
#if SERVO_SYNC_MONITOR
/**
 * @brief 同步接收伺服数据包（用于监控模式）
 * @param pkg 数据包指针
 * @return SERVO_STATUS 状态码
 */
static SERVO_STATUS Servo_Sync_RecvPackage(Package_t *pkg) {
    extern StreamBufferHandle_t Servo_Rx_StreamHandle;
    uint8_t byte;
    uint8_t header = 0;
    uint8_t bIdx = 0;
    uint32_t startTime = osKernelGetTickCount();

    // 初始化包状态
    pkg->status = 0;

    // 超时等待（100ms）
    while((osKernelGetTickCount() - startTime) < SERVO_TIMEOUT_MS) {

        // 从队列获取一个字节
        if(StreamBufferReceive(Servo_Rx_StreamHandle, &byte, 10) != osOK) {
            continue;
        }

        // 帧头同步检测：严格匹配 0x05 0x1C
        if ((pkg->status & SERVO_RECV_FLAG_HEADER) == 0) {
            if (header == 0 && byte == 0x05) {
                header = byte;  // 帧头第1字节
            } else if (header == 0x05 && byte == 0x1C) {
                pkg->header = 0x1C05;  // 帧头检测成功
                pkg->status |= SERVO_RECV_FLAG_HEADER;
                header = 0;  // 重置帧头
            } else {
                header = 0;  // 帧头检测失败，重置
            }
            continue;
        }

        // 解析 cmdId
        if ((pkg->status & SERVO_RECV_FLAG_CMD_ID) == 0) {
            pkg->cmdId = byte;
            if (pkg->cmdId != 22) {  // 检查cmdId必须为22
                return SERVO_STATUS_UNKNOWN_CMD_ID;
            }
            pkg->status |= SERVO_RECV_FLAG_CMD_ID;
            continue;
        }

        // 解析 size
        if ((pkg->status & SERVO_RECV_FLAG_SIZE) == 0) {
            pkg->size = byte;
            if (pkg->size != 16) {  // 每组数据16字节
                return SERVO_STATUS_SIZE_TOO_BIG;
            }
            pkg->status |= SERVO_RECV_FLAG_SIZE;
            continue;
        }

        // 读取 content
        if ((pkg->status & SERVO_RECV_FLAG_CONTENT) == 0) {
            pkg->content[bIdx++] = byte;
            if (bIdx >= pkg->size) {
                pkg->status |= SERVO_RECV_FLAG_CONTENT;
            }
            continue;
        }

        // 校验和
        if ((pkg->status & SERVO_RECV_FLAG_CHECKSUM) == 0) {
            pkg->checksum = byte;
            if (Servo_CalcChecksum(pkg) != pkg->checksum) {
                return SERVO_STATUS_CHECKSUM_ERROR;
            }
            return SERVO_STATUS_SUCCESS;
        }
    }

    return SERVO_STATUS_TIMEOUT;
}

/**
 * @brief 同步命令舵机数据解析函数
 * @param usart 串口数据结构体指针
 * @param servo_count 舵机数量
 * @param servodata 舵机数据数组指针
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SyncServoMonitor(uint8_t servo_count, ServoData servodata[]) {
    Package_t pkg;
    SERVO_STATUS status;
    uint8_t packet_count = 0;

    while (packet_count < servo_count) {
        status = Servo_Sync_RecvPackage(&pkg);
        if (status != SERVO_STATUS_SUCCESS) {
            return status;
        }

        servodata[packet_count].id = pkg.content[0];
        servodata[packet_count].voltage = (int16_t)((pkg.content[2] << 8) | pkg.content[1]);
        servodata[packet_count].current = (int16_t)((pkg.content[4] << 8) | pkg.content[3]);
        servodata[packet_count].power = (int16_t)((pkg.content[6] << 8) | pkg.content[5]);

        // 温度转换（原公式，添加保护）
        uint16_t adc_temp = (uint16_t)((pkg.content[8] << 8) | pkg.content[7]);
        if (adc_temp < 4096) {  // 避免除零
            double temp = adc_temp;
            double kelvin = 1.0 / (log(temp / (4096.0 - temp)) / 3435.0 + 1.0 / (273.15 + 25.0));
            servodata[packet_count].temperature = (int16_t)(kelvin - 273.15);
        } else {
            servodata[packet_count].temperature = 0;
        }

        servodata[packet_count].status = pkg.content[9];

        int32_t raw_angle = (int32_t)((pkg.content[13] << 24) |
                                      (pkg.content[12] << 16) |
                                      (pkg.content[11] << 8) |
                                      pkg.content[10]);
        servodata[packet_count].angle = (float)(raw_angle / 10.0f);
        servodata[packet_count].circle_count = (int16_t)((pkg.content[15] << 8) | pkg.content[14]);

        packet_count++;
        memset(&pkg, 0, sizeof(Package_t));
    }

    return SERVO_STATUS_SUCCESS;
}
#endif

/**
 * @brief 发送通用数据包
 * @param cmdId 命令ID
 * @param size 数据大小
 * @param content 数据指针
 * @param isSync 是否同步
 */
static SERVO_STATUS Servo_SendPackage_Common(uint8_t cmdId, uint16_t size, uint8_t *content, uint8_t isSync){
    extern osMessageQueueId_t Servo_Tx_DataHandle;
    Package_t pkg = {
        .header = SERVO_PACK_REQUEST_HEADER,
        .cmdId = cmdId,
        .size = size,
        .isSync = isSync,
    };
    memcpy(pkg.content, content, size);
    pkg.checksum = Servo_CalcChecksum(&pkg);


    // 发送数据到串口队列
    if(osMessageQueuePut(Servo_Tx_DataHandle, &pkg, 0, 50) != osOK){
        // 发送失败
        logError("Failed to send package to Servo");
        return SERVO_STATUS_FAIL;
    } else {
        // 发送成功
        return SERVO_STATUS_SUCCESS;
    }
}

/**
 * @brief 计算伺服数据包校验和
 * @param pkg 数据包指针
 * @return 校验和值
 */
static uint8_t Servo_CalcChecksum(Package_t *pkg) {
    uint32_t sum = 0;
    sum += (pkg->header >> 8) & 0xFF;
    sum += pkg->header & 0xFF;
    sum += pkg->cmdId;
    if (pkg->isSync) {
        sum += 0xFF;
        sum += pkg->size & 0xFF;
        sum += (pkg->size >> 8) & 0xFF;
    } else {
        sum += pkg->size & 0xFF;
    }
    for (uint16_t i = 0; i < pkg->size; i++) {
        sum += pkg->content[i];
    }
    return (uint8_t)(sum % 256);
}

/**
 * @brief 设置伺服角度
 * @param servo_id 伺服ID
 * @param angle 角度
 * @param interval 间隔时间
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SetServoAngle(uint8_t servo_id, float angle, uint16_t interval, uint16_t power){
	const uint8_t size = 7;

	// 数值约束
	if(angle > 180.0f){
		angle = 180.0f;
	}else if(angle < -180.0f){
		angle = -180.0f;
	}

    int16_t scaledAngle = (int16_t)(10*angle);

    uint8_t content[size];
    content[0] = servo_id;                    // 1字节
    content[1] = scaledAngle & 0xFF;          // 角度低字节
    content[2] = (scaledAngle >> 8) & 0xFF;   // 角度高字节
    content[3] = interval & 0xFF;             // interval低字节
    content[4] = (interval >> 8) & 0xFF;      // interval高字节
    content[5] = power & 0xFF;                // power低字节
    content[6] = (power >> 8) & 0xFF;         // power高字节

	return Servo_SendPackage_Common(SERVO_CMD_ROTATE, size, content,0);

}

/**
 * @brief 设置伺服角度(多圈模式)
 * @param servo_id 伺服ID
 * @param angle 角度
 * @param interval 时间间隔
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
SERVO_STATUS Servo_SetServoAngleMTurn(uint8_t servo_id, float angle,
	                                    uint32_t interval, uint16_t power){

	const uint8_t size = 11;

	// 数值约束
	if(angle > 368640.0f){
		angle = 368640.0f;
	}else if(angle < -368640.0f){
		angle = -368640.0f;
	}
	if(interval > 4096000){
		interval = 4096000;
	}

    int32_t scaledAngle = (int32_t)(10*angle);

	// 协议打包
    uint8_t content[size];
    content[0] = servo_id;
    content[1] = scaledAngle & 0xFF;
    content[2] = (scaledAngle >> 8) & 0xFF;
    content[3] = (scaledAngle >> 16) & 0xFF;
    content[4] = (scaledAngle >> 24) & 0xFF;
    content[5] = interval & 0xFF;
    content[6] = (interval >> 8) & 0xFF;
    content[7] = (interval >> 16) & 0xFF;
    content[8] = (interval >> 24) & 0xFF;
    content[9] = power & 0xFF;
    content[10] = (power >> 8) & 0xFF;

    // 发送请求包
    return Servo_SendPackage_Common(SERVO_CMD_SET_ANGLE_MTURN, size, content,0);
}

/**
 * @brief 舵机控制模式停止指令
 * @param servo_id 伺服ID
 * @param mode 指令停止形式
 * @param power 功率
 * @return SERVO_STATUS 状态码
 */
//mode 指令停止形式
//0-停止后卸力(失锁)
//1-停止后保持锁力
//2-停止后进入阻尼状态
SERVO_STATUS Servo_StopOnControlMode(uint8_t servo_id, uint8_t mode, uint16_t power) {
	// 创建环形缓冲队列
	const uint8_t size = 4;
    uint8_t content[size];
	content[0] = servo_id;
	content[1] = mode | 0x10;
	content[2] = power & 0xFF;
	content[3] = (power >> 8) & 0xFF;

    return Servo_SendPackage_Common(SERVO_CMD_CONTROL_MODE_STOP,(uint8_t)size, content,0);
}

/**
 * @brief 将数据包转换为字节流（用于串口发送）
 * @param pkg 数据包指针
 * @param buffer 输出缓冲区
 * @return 字节流长度
 */
static uint16_t Servo_PackageToBytes(Package_t *pkg, uint8_t *buffer) {
     uint16_t offset = 0;

    /* 帧头（小端） */
    buffer[offset++] = pkg->header & 0xFF;
    buffer[offset++] = (pkg->header >> 8) & 0xFF;

    /* 命令 ID */
    buffer[offset++] = pkg->cmdId;

    /* 长度字段 */
    if (pkg->isSync || pkg->size > 255) {
        buffer[offset++] = 0xFF;
        buffer[offset++] = pkg->size & 0xFF;
        buffer[offset++] = (pkg->size >> 8) & 0xFF;
    } else {
        buffer[offset++] = pkg->size & 0xFF;
    }

    /* 内容 */
    for (uint16_t i = 0; i < pkg->size; i++) {
        buffer[offset++] = pkg->content[i];
    }

    /* [CHANGE] 直接用 SendPackage_Common 中已算好的 checksum */
    buffer[offset++] = pkg->checksum;

    return offset;
}

/**
 * @brief 串口发送任务
 * @param argument 任务参数
 */
void Servo_Tx_Task(void *argument) {
    (void)argument;
    extern osMessageQueueId_t Servo_Tx_DataHandle;

    static Package_t pkg;
    static uint8_t txBuffer[256];
    uint16_t len;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for(;;) {
        if(osMessageQueueGet(Servo_Tx_DataHandle, &pkg, NULL, osWaitForever) == osOK) {
            len = Servo_PackageToBytes(&pkg, txBuffer);

            osEventFlagsClear(System_StatusHandle, UART3_TX_IDLE);
            HAL_UART_Transmit_DMA(&huart3, txBuffer, len);
            osEventFlagsWait(System_StatusHandle, UART3_TX_IDLE, osFlagsWaitAny, osWaitForever);
        }
    }
}

/**
 * @brief 舵机模块初始化
 * @note 初始化串口，设置波特率为115200
 */
void Servo_Init(void) {
    MX_USART3_UART_Init();
    Servo_Rx_StreamHandle = xStreamBufferCreate(512, 1);
    configASSERT(Servo_Rx_StreamHandle != NULL);
    osEventFlagsClear(System_StatusHandle, UART3_TX_IDLE);
    osEventFlagsClear(System_StatusHandle, UART3_RX_IDLE);
}
