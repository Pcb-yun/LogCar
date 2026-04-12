/*
 * Fashion Star 总线伺服舵机驱动库
 * Version: v0.0.2
 * UpdateTime: 2024/07/17
 */
#include "fashion_star_uart_servo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//同步命令舵机结构体数组
FSUS_sync_servo SyncArray[20]; // 假设您要控制20个伺服同步
ServoData servodata[20];//假设您要读取20个伺服舵机的数据


// 统一数据包处理
void FSUS_Package2RingBuffer(PackageTypeDef *pkg, RingBufferTypeDef *ringBuf) {
    uint8_t checksum;
    RingBuffer_WriteUShort(ringBuf, pkg->header);// 写入帧头
    RingBuffer_WriteByte(ringBuf, pkg->cmdId);// 写入指令ID
    
    if (pkg->isSync || pkg->size > 255) {
        RingBuffer_WriteByte(ringBuf, 0xFF);
        RingBuffer_WriteUShort(ringBuf, pkg->size);// 写入包的长度
    } else {
        RingBuffer_WriteByte(ringBuf, (uint8_t)pkg->size);// 写入包的长度
    }
    
    RingBuffer_WriteByteArray(ringBuf, pkg->content, pkg->size);// 写入内容主题
    checksum = RingBuffer_GetChecksum(ringBuf);// 计算校验和
    RingBuffer_WriteByte(ringBuf, checksum);// 写入校验和
}

// 计算Package的校验和
uint8_t FSUS_CalcChecksum(PackageTypeDef *pkg) {
    RingBufferTypeDef ringBuf;// 初始化环形队列
    uint8_t pkgBuf[FSUS_PACK_RESPONSE_MAX_SIZE + 5];
    RingBuffer_Init(&ringBuf, sizeof(pkgBuf), pkgBuf);
	// 将Package转换为ringbuffer
	// 在转换的时候,会自动的计算checksum
    FSUS_Package2RingBuffer(pkg, &ringBuf);
	// 获取环形队列队尾的元素(即校验和的位置)
    return RingBuffer_GetValueByIndex(&ringBuf, RingBuffer_GetByteUsed(&ringBuf)-1);
}

// 判断是否为有效的请求头的（数据包验证）
FSUS_STATUS FSUS_IsValidResponsePackage(PackageTypeDef *pkg) {
    if (pkg->header != FSUS_PACK_RESPONSE_HEADER)
        return FSUS_STATUS_WRONG_RESPONSE_HEADER;// 帧头不对
    if (pkg->cmdId > FSUS_CMD_NUM)// 判断控制指令是否有效 指令范围超出
        return FSUS_STATUS_UNKOWN_CMD_ID;
    if (pkg->size > (FSUS_PACK_RESPONSE_MAX_SIZE - 6))  // 调整校验长度
        return FSUS_STATUS_SIZE_TOO_BIG;
    if (FSUS_CalcChecksum(pkg) != pkg->checksum)// 校验和不匹配
        return FSUS_STATUS_CHECKSUM_ERROR;
    return FSUS_STATUS_SUCCESS;// 数据有效
}

// 字节数组转换为数据帧
FSUS_STATUS FSUS_RingBuffer2Package(RingBufferTypeDef *ringBuf, PackageTypeDef *pkg){
    // 申请内存
    pkg = (PackageTypeDef *)malloc(sizeof(PackageTypeDef));
    // 读取帧头
    pkg->header = RingBuffer_ReadUShort(ringBuf);
    // 读取指令ID
    pkg->cmdId = RingBuffer_ReadByte(ringBuf);
    // 读取包的长度
    pkg->size = RingBuffer_ReadByte(ringBuf);
    // 申请参数的内存空间
    // pkg->content = (uint8_t *)malloc(pkg->size);
    // 写入content
    RingBuffer_ReadByteArray(ringBuf, pkg->content, pkg->size);
    // 写入校验和
    pkg->checksum = RingBuffer_ReadByte(ringBuf);
    // 返回当前的数据帧是否为有效反馈数据帧
    return FSUS_IsValidResponsePackage(pkg);
}

// 发送数据包
void FSUS_SendPackage_Common(Usart_DataTypeDef *usart, uint8_t cmdId, uint16_t size, uint8_t *content, uint8_t isSync) {
    PackageTypeDef pkg = {
        .header = FSUS_PACK_REQUEST_HEADER,
        .cmdId = cmdId,
        .size = size,
        .isSync = isSync
    };
    memcpy(pkg.content, content, size);
    FSUS_Package2RingBuffer(&pkg, usart->sendBuf);// 将数据放入发送缓冲区
    Usart_SendAll(usart);// 通过串口将数据发送出去
}

// 接收数据包（统一处理）
FSUS_STATUS FSUS_RecvPackage(Usart_DataTypeDef *usart, PackageTypeDef *pkg) {
    // 简化后的接收逻辑，支持动态长度
    uint16_t header = 0;
    uint32_t timeout = HAL_GetTick() + FSUS_TIMEOUT_MS;
		
		// 如果没有超时
		while (HAL_GetTick() < timeout) {
        if (RingBuffer_GetByteUsed(usart->recvBuf) < 5) continue;

        // 解析头部
        header = RingBuffer_PeekUShort(usart->recvBuf, 0);
        if (header != FSUS_PACK_RESPONSE_HEADER) {
            RingBuffer_ReadByte(usart->recvBuf);
            continue;
        }

        // 提取长度
        uint16_t size;
        if (RingBuffer_PeekByte(usart->recvBuf, 3) == 0xFF) {
            size = RingBuffer_PeekUShort(usart->recvBuf, 4);
            pkg->isSync = 1;
        } else {
            size = RingBuffer_PeekByte(usart->recvBuf, 3);
            pkg->isSync = 0;
        }

        // 检查完整包
//        uint16_t totalLen = pkg->isSync ? 6 + size : 5 + size;
				uint16_t totalLen = 5 + size + (pkg->isSync ? 2 : 0);
        if (RingBuffer_GetByteUsed(usart->recvBuf) < totalLen) continue;

        // 解析完整包
        pkg->header = RingBuffer_ReadUShort(usart->recvBuf);  // header
        pkg->cmdId = RingBuffer_ReadByte(usart->recvBuf);
        if (pkg->isSync) {
            RingBuffer_ReadByte(usart->recvBuf);  // 0xFF
            pkg->size = RingBuffer_ReadUShort(usart->recvBuf);
        } else {
            pkg->size = RingBuffer_ReadByte(usart->recvBuf);
        }
        RingBuffer_ReadByteArray(usart->recvBuf, pkg->content, pkg->size);
        pkg->checksum = RingBuffer_ReadByte(usart->recvBuf);
        					
        return FSUS_IsValidResponsePackage(pkg);
    }
    return FSUS_STATUS_TIMEOUT;
}

FSUS_STATUS FSUS_sync_RecvPackage(Usart_DataTypeDef *usart, PackageTypeDef *pkg)
{
    pkg->status = 0;  // 初始化包状态
    uint8_t bIdx = 0;  // 数据索引
    uint16_t header = 0;  // 帧头检测
		uint8_t byte;
		uint32_t timeout = HAL_GetTick() + FSUS_TIMEOUT_MS;
	// 如果没有超时
	while (HAL_GetTick() < timeout)
	{
        // 如果接收缓冲区为空，继续等待
        if (RingBuffer_GetByteUsed(usart->recvBuf) == 0) {
            continue;
        }
        // 读取一个字节
        byte = RingBuffer_ReadByte(usart->recvBuf);
//        printf("Received byte: 0x%02X\n", byte);
        // 帧头同步检测：严格匹配0x05 0x1C
        if ((pkg->status & FSUS_RECV_FLAG_HEADER) == 0) {
            if (header == 0 && byte == 0x05) {
                header = byte;  // 帧头第1字节
            } else if (header == 0x05 && byte == 0x1C) {
                pkg->header = 0x1C05;  // 帧头检测成功
                pkg->status |= FSUS_RECV_FLAG_HEADER;
                header = 0;  // 重置帧头
            } else {
                header = 0;  // 帧头检测失败，重置
            }
            continue;
        }
        // 解析cmdId
        if ((pkg->status & FSUS_RECV_FLAG_CMD_ID) == 0) {
            pkg->cmdId = byte;
            if (pkg->cmdId != 22) {  // 检查cmdId是否正确
                return FSUS_STATUS_UNKOWN_CMD_ID;
            }
            pkg->status |= FSUS_RECV_FLAG_CMD_ID;
            continue;
        }
        // 解析size
        if ((pkg->status & FSUS_RECV_FLAG_SIZE) == 0) {
            pkg->size = byte;
						if (pkg->size != 16) {  // 每组数据16字节
                return FSUS_STATUS_SIZE_TOO_BIG;
            }
            pkg->status |= FSUS_RECV_FLAG_SIZE;
            continue;
        }
        // 读取内容
        if ((pkg->status & FSUS_RECV_FLAG_CONTENT) == 0) {
            pkg->content[bIdx++] = byte;
            if (bIdx >= pkg->size) {
                pkg->status |= FSUS_RECV_FLAG_CONTENT;
            }
            continue;
        }
        // 校验和
        if ((pkg->status & FSUS_RECV_FLAG_CHECKSUM) == 0) {
            pkg->checksum = byte;
            if (FSUS_CalcChecksum(pkg) != pkg->checksum) {
                return FSUS_STATUS_CHECKSUM_ERROR;
            }
            return FSUS_STATUS_SUCCESS;
        }
    }
    return FSUS_STATUS_TIMEOUT;  // 超时返回
}

// 舵机通讯检测
// 注: 如果没有舵机响应这个Ping指令的话, 就会超时
FSUS_STATUS FSUS_Ping(Usart_DataTypeDef *usart, uint8_t servo_id){
	uint8_t statusCode; // 状态码
	uint8_t ehcoServoId; // PING得到的舵机ID
	// printf("[PING]Send Ping Package\r\n");
	// 发送请求包
	FSUS_SendPackage_Common(usart, FSUS_CMD_PING, 1, &servo_id,0);
	// 接收返回的Ping
	PackageTypeDef pkg;
	statusCode = FSUS_RecvPackage(usart, &pkg);
	if(statusCode == FSUS_STATUS_SUCCESS){
		// 进一步检查ID号是否匹配
		ehcoServoId = (uint8_t)pkg.content[0];
		if (ehcoServoId != servo_id){
			// 反馈得到的舵机ID号不匹配
			return FSUS_STATUS_ID_NOT_MATCH;
		}
	}
	return statusCode;
}

// 重置舵机的用户资料
FSUS_STATUS FSUS_ResetUserData(Usart_DataTypeDef *usart, uint8_t servo_id){
	const uint8_t size = 1;
	FSUS_STATUS statusCode;
	// 发送请求包
	FSUS_SendPackage_Common(usart, FSUS_CMD_RESET_USER_DATA, size, &servo_id,0);
	// 接收重置结果
	PackageTypeDef pkg;
	statusCode = FSUS_RecvPackage(usart, &pkg);
	if (statusCode == FSUS_STATUS_SUCCESS){
		// 成功的接收到反馈数据
		// 读取反馈数据中的result
		uint8_t result = (uint8_t)pkg.content[1];
		if (result == 1){
			return FSUS_STATUS_SUCCESS;
		}else{
			return FSUS_STATUS_FAIL;
		}
	}
	return statusCode;
}

// 读取数据
FSUS_STATUS FSUS_ReadData(Usart_DataTypeDef *usart, uint8_t servo_id,  uint8_t address, uint8_t *value, uint8_t *size){
	FSUS_STATUS statusCode;
	// 构造content
	uint8_t buffer[2] = {servo_id, address};
	// 发送请求数据
	FSUS_SendPackage_Common(usart, FSUS_CMD_READ_DATA, 2, buffer,0);
	// 接收返回信息
	PackageTypeDef pkg;
	statusCode = FSUS_RecvPackage(usart, &pkg);
	if (statusCode == FSUS_STATUS_SUCCESS){
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

// 写入数据
FSUS_STATUS FSUS_WriteData(Usart_DataTypeDef *usart, uint8_t servo_id, uint8_t address, uint8_t *value, uint8_t size){
	FSUS_STATUS statusCode;
	// 构造content
	uint8_t buffer[size+2]; // 舵机ID + 地址位Address + 数据byte数
	buffer[0] = servo_id;
	buffer[1] = address;
	// 拷贝数据
	for (int i=0; i<size; i++){
		buffer[i+2] = value[i];
	}
	// 发送请求数据
	FSUS_SendPackage_Common(usart, FSUS_CMD_WRITE_DATA, size+2, buffer,0);
	// 接收返回信息
	PackageTypeDef pkg;
	statusCode = FSUS_RecvPackage(usart, &pkg);
	if (statusCode == FSUS_STATUS_SUCCESS){
		uint8_t result = pkg.content[2];
		if(result == 1){
			statusCode = FSUS_STATUS_SUCCESS;
		}else{
			statusCode = FSUS_STATUS_FAIL;
		}
	}
	return statusCode;
}


// 设置舵机的角度
// @angle 单位度
// @interval 单位ms
// @power 舵机执行功率 单位mW
//        若power=0或者大于保护值
FSUS_STATUS FSUS_SetServoAngle(Usart_DataTypeDef *usart, uint8_t servo_id, float angle, uint16_t interval, uint16_t power){
	// 创建环形缓冲队列
	const uint8_t size = 7;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
	// 数值约束
	if(angle > 180.0f){
		angle = 180.0f;
	}else if(angle < -180.0f){
		angle = -180.0f;
	}
	// 构造content
	RingBuffer_WriteByte(&ringBuf, servo_id);
	RingBuffer_WriteShort(&ringBuf, (int16_t)(10*angle));
	RingBuffer_WriteUShort(&ringBuf, interval);
	RingBuffer_WriteUShort(&ringBuf, power);
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_ROTATE, size, buffer+1,0);
	
	return FSUS_STATUS_SUCCESS;
}

/* 设置舵机的角度(指定周期) */
FSUS_STATUS FSUS_SetServoAngleByInterval(Usart_DataTypeDef *usart, uint8_t servo_id, 
				float angle, uint16_t interval, uint16_t t_acc, 
				uint16_t t_dec, uint16_t  power){
	// 创建环形缓冲队列
	const uint8_t size = 11;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
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
	
	// 协议打包
	RingBuffer_WriteByte(&ringBuf, servo_id);
	RingBuffer_WriteShort(&ringBuf, (int16_t)(10*angle));
	RingBuffer_WriteUShort(&ringBuf, interval);
	RingBuffer_WriteUShort(&ringBuf, t_acc);
	RingBuffer_WriteUShort(&ringBuf, t_dec);
	RingBuffer_WriteUShort(&ringBuf, power);
	
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_ANGLE_BY_INTERVAL, size, buffer+1,0);
	
	return FSUS_STATUS_SUCCESS;
}

/* 设置舵机的角度(指定转速) */
FSUS_STATUS FSUS_SetServoAngleByVelocity(Usart_DataTypeDef *usart, uint8_t servo_id, \
				float angle, float velocity, uint16_t t_acc, \
				uint16_t t_dec, uint16_t  power){
	// 创建环形缓冲队列
	const uint8_t size = 11;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
	
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
	
	// 协议打包
	RingBuffer_WriteByte(&ringBuf, servo_id);
	RingBuffer_WriteShort(&ringBuf, (int16_t)(10.0f*angle));
	RingBuffer_WriteUShort(&ringBuf, (uint16_t)(10.0f*velocity));
	RingBuffer_WriteUShort(&ringBuf, t_acc);
	RingBuffer_WriteUShort(&ringBuf, t_dec);
	RingBuffer_WriteUShort(&ringBuf, power);
	
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_ANGLE_BY_VELOCITY, size, buffer+1,0);
	
	return FSUS_STATUS_SUCCESS;
}

/* 查询单个舵机的角度信息 angle 单位度 */
FSUS_STATUS FSUS_QueryServoAngle(Usart_DataTypeDef *usart, uint8_t servo_id, float *angle){
	const uint8_t size = 1; // 请求包content的长度
	uint8_t ehcoServoId;
	int16_t echoAngle;
	
	// 发送舵机角度请求包
	FSUS_SendPackage_Common(usart, FSUS_CMD_READ_ANGLE, size, &servo_id,0);
	// 接收返回的Ping
	PackageTypeDef pkg;
	uint8_t statusCode = FSUS_RecvPackage(usart, &pkg);
	if (statusCode == FSUS_STATUS_SUCCESS){
		// 成功的获取到舵机角度回读数据
		ehcoServoId = (uint8_t)pkg.content[0];
		// 检测舵机ID是否匹配
		if (ehcoServoId != servo_id){
			// 反馈得到的舵机ID号不匹配
			return FSUS_STATUS_ID_NOT_MATCH;
		}

		// 提取舵机角度
		echoAngle = (int16_t)(pkg.content[1] | (pkg.content[2] << 8));
		*angle = (float)(echoAngle / 10.0);
	}
  return statusCode;
}	

/* 设置舵机的角度(多圈模式) */
FSUS_STATUS FSUS_SetServoAngleMTurn(Usart_DataTypeDef *usart, uint8_t servo_id, float angle, 
	uint32_t interval, uint16_t power){
	// 创建环形缓冲队列
	const uint8_t size = 11;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
	// 数值约束			
	if(angle > 368640.0f){
		angle = 368640.0f;
	}else if(angle < -368640.0f){
		angle = -368640.0f;
	}
	if(interval > 4096000){
		angle = 4096000;
	}
	// 协议打包
	RingBuffer_WriteByte(&ringBuf, servo_id);
	RingBuffer_WriteLong(&ringBuf, (int32_t)(10*angle));
	RingBuffer_WriteULong(&ringBuf, interval);
	RingBuffer_WriteShort(&ringBuf, power);
	
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_ANGLE_MTURN, size, buffer+1,0);

	return FSUS_STATUS_SUCCESS;
}

/* 设置舵机的角度(多圈模式, 指定周期) */
FSUS_STATUS FSUS_SetServoAngleMTurnByInterval(Usart_DataTypeDef *usart, uint8_t servo_id, float angle, \
			uint32_t interval,  uint16_t t_acc,  uint16_t t_dec, uint16_t power){
	// 创建环形缓冲队列
	const uint8_t size = 15;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
	
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
	// 协议打包
	RingBuffer_WriteByte(&ringBuf, servo_id);
	RingBuffer_WriteLong(&ringBuf, (int32_t)(10*angle));
	RingBuffer_WriteULong(&ringBuf, interval);
	RingBuffer_WriteUShort(&ringBuf, t_acc);
	RingBuffer_WriteUShort(&ringBuf, t_dec);
	RingBuffer_WriteShort(&ringBuf, power);
	
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_ANGLE_MTURN_BY_INTERVAL, size, buffer+1,0);
	
	return FSUS_STATUS_SUCCESS;
}

/* 设置舵机的角度(多圈模式, 指定转速) */
FSUS_STATUS FSUS_SetServoAngleMTurnByVelocity(Usart_DataTypeDef *usart, uint8_t servo_id, float angle, \
			float velocity, uint16_t t_acc,  uint16_t t_dec, uint16_t power){
	// 创建环形缓冲队列
	const uint8_t size = 13;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
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
	// 协议打包
	RingBuffer_WriteByte(&ringBuf, servo_id);
	RingBuffer_WriteLong(&ringBuf, (int32_t)(10.0f*angle));
	RingBuffer_WriteUShort(&ringBuf, (uint16_t)(10.0f*velocity));
	RingBuffer_WriteUShort(&ringBuf, t_acc);
	RingBuffer_WriteUShort(&ringBuf, t_dec);
	RingBuffer_WriteShort(&ringBuf, power);
	
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_ANGLE_MTURN_BY_VELOCITY, size, buffer+1,0);
	
	return FSUS_STATUS_SUCCESS;
}

/* 查询舵机的角度(多圈模式) */
FSUS_STATUS FSUS_QueryServoAngleMTurn(Usart_DataTypeDef *usart, uint8_t servo_id, float *angle){
	// 创建环形缓冲队列
	const uint8_t size = 1; // 请求包content的长度
	uint8_t ehcoServoId;
	int32_t echoAngle;
	
	// 发送舵机角度请求包
	FSUS_SendPackage_Common(usart, FSUS_CMD_QUERY_SERVO_ANGLE_MTURN, size, &servo_id,0);
	// 接收返回的Ping
	PackageTypeDef pkg;
	uint8_t statusCode = FSUS_RecvPackage(usart, &pkg);
	if (statusCode == FSUS_STATUS_SUCCESS){
		// 成功的获取到舵机角度回读数据
		ehcoServoId = (uint8_t)pkg.content[0];
		// 检测舵机ID是否匹配
		if (ehcoServoId != servo_id){
			// 反馈得到的舵机ID号不匹配
			return FSUS_STATUS_ID_NOT_MATCH;
		}
		
		// 提取舵机角度
		echoAngle = (int32_t)(pkg.content[1] | (pkg.content[2] << 8) |  (pkg.content[3] << 16) | (pkg.content[4] << 24));
		*angle = (float)(echoAngle / 10.0);
	}
  return statusCode;
}


/* 舵机阻尼模式 */
FSUS_STATUS FSUS_DampingMode(Usart_DataTypeDef *usart, uint8_t servo_id, uint16_t power){
	const uint8_t size = 3; // 请求包content的长度
	uint8_t buffer[size+1]; // content缓冲区
	RingBufferTypeDef ringBuf; // 创建环形缓冲队列
	RingBuffer_Init(&ringBuf, size, buffer); // 缓冲队列初始化
	// 构造content
	RingBuffer_WriteByte(&ringBuf, servo_id);
	RingBuffer_WriteUShort(&ringBuf, power);
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_DAMPING, size, buffer+1,0);
	return FSUS_STATUS_SUCCESS;
}


// 舵机重置多圈角度圈数
FSUS_STATUS FSUS_ServoAngleReset(Usart_DataTypeDef *usart, uint8_t servo_id){
	uint8_t statusCode; // 状态码

	FSUS_SendPackage_Common(usart, FSUS_CMD_RESERT_SERVO_ANGLE_MTURN, 1, &servo_id,0);
	// 接收返回的Ping

	return statusCode;
}

/*零点设置 仅适用于无刷磁编码舵机*/
FSUS_STATUS FSUS_SetOriginPoint(Usart_DataTypeDef *usart, uint8_t servo_id)
{
	uint8_t statusCode; // 状态码

	FSUS_SendPackage_Common(usart, FSUS_CMD_SET_ORIGIN_POINT, 2, &servo_id,0);
	// 接收返回的Ping

	return statusCode;
}

/* 舵机开始异步命令*/
FSUS_STATUS FSUS_BeginAsync(Usart_DataTypeDef *usart)
{
	const uint8_t size = 0; // 请求包content的长度
	uint8_t buffer[size+1]; // content缓冲区
	RingBufferTypeDef ringBuf; // 创建环形缓冲队列
	RingBuffer_Init(&ringBuf, size, buffer); // 缓冲队列初始化
	// 构造content
	RingBuffer_WriteByte(&ringBuf, 0);
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_BEGIN_ASYNC, size, buffer+1,0);
	return FSUS_STATUS_SUCCESS;
}

/* 舵机结束异步命令*/
FSUS_STATUS FSUS_EndAsync(Usart_DataTypeDef *usart,uint8_t mode)
{
/*mode
	0:执行存储的命令
	1:取消存储的命令
*/
	const uint8_t size = 1; // 请求包content的长度
	uint8_t buffer[size+1]; // content缓冲区
	RingBufferTypeDef ringBuf; // 创建环形缓冲队列
	RingBuffer_Init(&ringBuf, size, buffer); // 缓冲队列初始化
	// 构造content
	RingBuffer_WriteByte(&ringBuf, mode);
	// 发送请求包
	// 注: 因为用的是环形队列 head是空出来的,所以指针需要向后推移一个字节
	FSUS_SendPackage_Common(usart, FSUS_CMD_END_ASYNC, size, buffer+1,0);
	return FSUS_STATUS_SUCCESS;
}

/* 舵机单个数据监控*/
FSUS_STATUS FSUS_ServoMonitor(Usart_DataTypeDef *usart, uint8_t servo_id, ServoData servodata[]) {

	// 创建环形缓冲队列
	const uint8_t size = 1;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
	double temp;//温度数据转换
	RingBuffer_WriteByte(&ringBuf, servo_id);	
	FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_ReadData,(uint8_t)size, buffer + 1,0);

  PackageTypeDef pkg;
  FSUS_STATUS status=FSUS_RecvPackage(usart, &pkg);;

   if (status != FSUS_STATUS_SUCCESS) {
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

    return FSUS_STATUS_SUCCESS;
}


/* 舵机控制模式停止指令*/
//mode 指令停止形式
//0-停止后卸力(失锁)
//1-停止后保持锁力
//2-停止后进入阻尼状态
FSUS_STATUS FSUS_StopOnControlMode(Usart_DataTypeDef *usart, uint8_t servo_id, uint8_t mode, uint16_t power) {
	// 创建环形缓冲队列
	const uint8_t size = 4;
	uint8_t buffer[size+1];
	RingBufferTypeDef ringBuf;
	RingBuffer_Init(&ringBuf, size, buffer);	
	
		RingBuffer_WriteByte(&ringBuf, servo_id);	
		mode = mode | 0x10;
		RingBuffer_WriteByte(&ringBuf, mode);	
		RingBuffer_WriteShort(&ringBuf, power);

		FSUS_SendPackage_Common(usart, FSUS_CMD_CONTROL_MODE_STOP,(uint8_t)size, buffer + 1,0);
	
    return FSUS_STATUS_SUCCESS;
}

/* 同步命令选择模式控制函数*/
// @servo_count 舵机同步数量
// @servomode 同步命令模式选择
// @FSUS_servo_sync servoSync[] 舵机参数结构体数组
FSUS_STATUS FSUS_SyncCommand(Usart_DataTypeDef *usart, uint8_t servo_count, uint8_t ServoMode, FSUS_sync_servo servoSync[]) {

		uint8_t buffer[3 + servo_count * 15]; // 最大可能的缓冲区大小
		uint16_t size;
		RingBufferTypeDef ringBuf;

//		ServoData *servo_data = malloc(servo_count * sizeof(ServoData));

    switch (ServoMode) {
        case MODE_SET_SERVO_ANGLE:
            /* 同步命令设置舵机的角度(单圈模式）*/
						size = 3 + servo_count * 7;
            RingBuffer_Init(&ringBuf, size, buffer);
						RingBuffer_WriteByte(&ringBuf, 8);
            RingBuffer_WriteByte(&ringBuf, 7);
            RingBuffer_WriteByte(&ringBuf, servo_count);
            for (int i = 0; i < servo_count; i++) {
                RingBuffer_WriteByte(&ringBuf, servoSync[i].id);
                if (servoSync[i].angle > 180.0f) {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * 180.0f));
                } else if (servoSync[i].angle < -180.0f) {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * -180.0f));
                } else {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * servoSync[i].angle));
                }
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].interval_single);
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].power);
            }
            break;
				case MODE_SET_SERVO_ANGLE_BY_INTERVAL:
            /* 同步命令设置舵机的角度(单圈模式，指定周期) */
						size = 3 + servo_count * 11;
            RingBuffer_Init(&ringBuf, size, buffer);
            RingBuffer_WriteByte(&ringBuf, 11);
            RingBuffer_WriteByte(&ringBuf, 11);
            RingBuffer_WriteByte(&ringBuf, servo_count);
            for (int i = 0; i < servo_count; i++) {
                RingBuffer_WriteByte(&ringBuf, servoSync[i].id);
                if (servoSync[i].angle > 180.0f) {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * 180.0f));
                } else if (servoSync[i].angle < -180.0f) {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * -180.0f));
                } else {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * servoSync[i].angle));
                }
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].interval_single);
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_acc);
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_dec);
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].power);
            }
            break;
				case MODE_SET_SERVO_ANGLE_BY_VELOCITY:
            /* 同步命令设置舵机的角度(单圈模式，指定转速) */
						size = 3 + servo_count * 11;
            RingBuffer_Init(&ringBuf, size, buffer);
            RingBuffer_WriteByte(&ringBuf,12);
						RingBuffer_WriteByte(&ringBuf,11);
						RingBuffer_WriteByte(&ringBuf,servo_count);
	
						for(int i=0; i<servo_count; i++){
						RingBuffer_WriteByte(&ringBuf, servoSync[i].id);
						if (servoSync[i].angle > 180.0f) {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * 180.0f));
                } else if (servoSync[i].angle < -180.0f) {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * -180.0f));
                } else {
                    RingBuffer_WriteShort(&ringBuf, (int16_t)(10 * servoSync[i].angle));
                }
						RingBuffer_WriteUShort(&ringBuf, (uint16_t)(10.0f*servoSync[i].velocity));
						RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_acc);
            RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_dec);
            RingBuffer_WriteUShort(&ringBuf, servoSync[i].power);
            }
            break;
        case MODE_SET_SERVO_ANGLE_MTURN:
            /* 同步命令设置舵机的角度(多圈模式) */
						size = 3 + servo_count * 11;
            RingBuffer_Init(&ringBuf, size, buffer);
            RingBuffer_WriteByte(&ringBuf, 13);
            RingBuffer_WriteByte(&ringBuf, 11);
            RingBuffer_WriteByte(&ringBuf, servo_count);
            for (int i = 0; i < servo_count; i++) {
                RingBuffer_WriteByte(&ringBuf, servoSync[i].id);
                if (servoSync[i].angle > 368640.0f) {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * 368640.0f));
                } else if (servoSync[i].angle < -368640.0f) {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * -368640.0f));
                } else {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * servoSync[i].angle));
                }
                RingBuffer_WriteULong(&ringBuf, servoSync[i].interval_multi);
                RingBuffer_WriteShort(&ringBuf, servoSync[i].power);
            }
            break;
        case MODE_SET_SERVO_ANGLE_MTURN_BY_INTERVAL:
            /* 同步命令设置舵机的角度(多圈模式, 指定周期) */
						size = 3 + servo_count * 15;
            RingBuffer_Init(&ringBuf, size, buffer);
            RingBuffer_WriteByte(&ringBuf, 14);
            RingBuffer_WriteByte(&ringBuf, 15);
            RingBuffer_WriteByte(&ringBuf, servo_count);
            for (int i = 0; i < servo_count; i++) {
                RingBuffer_WriteByte(&ringBuf, servoSync[i].id);
                if (servoSync[i].angle > 368640.0f) {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * 368640.0f));
                } else if (servoSync[i].angle < -368640.0f) {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * -368640.0f));
                } else {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * servoSync[i].angle));
                }
                RingBuffer_WriteULong(&ringBuf, servoSync[i].interval_multi);
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_acc);
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_dec);
                RingBuffer_WriteShort(&ringBuf, servoSync[i].power);
            }
            break;
        case MODE_SET_SERVO_ANGLE_MTURN_BY_VELOCITY:
            /* 同步命令设置舵机的角度(多圈模式, 指定转速) */ 
						size = 3 + servo_count * 13;
            RingBuffer_Init(&ringBuf, size, buffer);
            RingBuffer_WriteByte(&ringBuf, 15);
            RingBuffer_WriteByte(&ringBuf, 13);
            RingBuffer_WriteByte(&ringBuf, servo_count);
            for (int i = 0; i < servo_count; i++) {
                RingBuffer_WriteByte(&ringBuf, servoSync[i].id);
                if (servoSync[i].angle > 368640.0f) {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * 368640.0f));
                } else if (servoSync[i].angle < -368640.0f) {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * -368640.0f));
                } else {
                    RingBuffer_WriteLong(&ringBuf, (int32_t)(10 * servoSync[i].angle));
                }
                RingBuffer_WriteUShort(&ringBuf, (uint16_t)(10.0f * servoSync[i].velocity));
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_acc);
                RingBuffer_WriteUShort(&ringBuf, servoSync[i].t_dec);
                RingBuffer_WriteShort(&ringBuf, servoSync[i].power);
            }
            break;
				case MODE_Query_SERVO_Monitor:
            /* 同步命令读取舵机的数据*/ 
						size = 3 + servo_count;
						RingBuffer_Init(&ringBuf, size, buffer);
						RingBuffer_WriteByte(&ringBuf, 22);
						RingBuffer_WriteByte(&ringBuf, 1);
						RingBuffer_WriteByte(&ringBuf, servo_count);
						for (int i = 0; i < servo_count; i++)RingBuffer_WriteByte(&ringBuf, servoSync[i].id);		
            break;
						
        default:
            return FSUS_STATUS_ERRO; // 无效的模式
    }

    // 发送请求包
		if(size<=255)    FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_SyncCommand,(uint8_t)size, buffer + 1,0);
		else     FSUS_SendPackage_Common(usart, FSUS_CMD_SET_SERVO_SyncCommand,size, buffer + 1,1);

		if(ServoMode==7)FSUS_SyncServoMonitor(usart, servo_count, servodata);

    return FSUS_STATUS_SUCCESS;
}	
/* 同步命令舵机数据解析函数*/
FSUS_STATUS FSUS_SyncServoMonitor(Usart_DataTypeDef *usart, uint8_t servo_count, ServoData servodata[]) {
    PackageTypeDef pkg;
    FSUS_STATUS status;
		double temp;//温度数据转换
		uint8_t packet_count = 0;
		
		while (packet_count < servo_count) {
        // 调用同步函数接收一个数据包
        status = FSUS_sync_RecvPackage(usart, &pkg);

        if (status != FSUS_STATUS_SUCCESS) {
            return status;  // 如果接收失败，返回错误状态
				}
				// 解析当前数据包内容
        servodata[packet_count].id = pkg.content[0];
        servodata[packet_count].voltage = (int16_t)((pkg.content[2] << 8) | pkg.content[1]);
        servodata[packet_count].current = (int16_t)((pkg.content[4] << 8) | pkg.content[3]);
        servodata[packet_count].power = (int16_t)((pkg.content[6] << 8) | pkg.content[5]);
        servodata[packet_count].temperature = (int16_t)((pkg.content[8] << 8) | pkg.content[7]);
				temp = (double)servodata[packet_count].temperature;
				servodata[packet_count].temperature = 1 / (log(temp / (4096.0f - temp)) / 3435.0f + 1 / (273.15 + 25)) - 273.15;
        servodata[packet_count].status = pkg.content[9];
        servodata[packet_count].angle = (int32_t)((pkg.content[13] << 24) | (pkg.content[12] << 16) | (pkg.content[11] << 8) | pkg.content[10]);
				servodata[packet_count].angle = (float)(servodata[packet_count].angle/10.0f);
        servodata[packet_count].circle_count = (int16_t)((pkg.content[15] << 8) | pkg.content[14]);

				// 完成一组数据包解析，计数加一
				packet_count++;
				
				// 重置 pkg 状态以接收下一组数据
        pkg.status = 0;

			}
		RingBuffer_Reset(usart->recvBuf);
    return FSUS_STATUS_SUCCESS;
}
