/**
 * @file servo_driver.c
 * @brief Fashion Star总线伺服舵机FreeRTOS驱动层
 */

#include "servo_driver.h"
#include <string.h>
#include <math.h>

osSemaphoreId_t ServoUartRxSemHandle;
extern UART_HandleTypeDef huart3;

static uint8_t ServoRecvBuf[SERVO_UART_RECV_BUF_SIZE + 1];
// static uint8_t ServoSendBuf[SERVO_UART_SEND_BUF_SIZE + 1];
static uint16_t ringHead = 0;
static uint16_t ringTail = 0;
static uint16_t ringSize = 0;

FSUS_sync_servo SyncArray[20];
ServoData servodata[20];

/**
 * @brief 初始化环形缓冲区
 * @param capacity 缓冲区容量
 */
void RingBuffer_Init(uint16_t capacity) {
    ringHead = 0;
    ringTail = 0;
    ringSize = capacity + 1;
    memset(ServoRecvBuf, 0, sizeof(ServoRecvBuf));
}

/**
 * @brief 重置环形缓冲区
 */
void RingBuffer_Reset(void) {
    ringHead = 0;
    ringTail = 0;
}

/**
 * @brief 获取环形缓冲区已用字节数
 * @return 已用字节数
 */
static uint16_t GetByteUsed(void) {
    if (ringHead <= ringTail) {
        return ringTail - ringHead;
    } else {
        return ringSize - (ringHead - ringTail);
    }
}

/**
 * @brief 获取环形缓冲区空闲字节数
 * @return 空闲字节数
 */
uint16_t RingBuffer_GetByteUsed(void) {
    return GetByteUsed();
}

/**
 * @brief 获取环形缓冲区空闲字节数
 * @return 空闲字节数
 */
static uint16_t GetByteFree(void) {
    return ringSize - 1 - GetByteUsed();
}

/**
 * @brief 判断环形缓冲区是否已满
 * @return 1 已满
 * @return 0 未满
 */
static uint8_t IsFull(void) {
    return GetByteFree() == 0;
}

// static uint16_t GetCapacity(void) {
//     return ringSize - 1;
// }

/**
 * @brief 从环形缓冲区读取一个字节
 * @return 读取到的字节
 */
uint8_t RingBuffer_ReadByte(void) {
    if (ringHead == ringTail) {
        return 0;
    }
    ringHead = (ringHead + 1) % ringSize;
    uint8_t value = ServoRecvBuf[ringHead];
    ServoRecvBuf[ringHead] = 0;
    return value;
}

/**
 * @brief 从环形缓冲区读取一个字节数组
 * @param dest 目标数组
 * @param size 数组大小
 */
void RingBuffer_ReadByteArray(uint8_t* dest, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        dest[i] = RingBuffer_ReadByte();
    }
}

/**
 * @brief 向环形缓冲区写入一个字节
 * @param value 待写入的字节
 */
static void Push(uint8_t value) {
    if (IsFull()) {
        return;
    }
    ringTail = (ringTail + 1) % ringSize;
    ServoRecvBuf[ringTail] = value;
}

/**
 * @brief 向环形缓冲区写入一个字节
 * @param value 待写入的字节
 */
void RingBuffer_Push(uint8_t value) {
    Push(value);
}

/**
 * @brief 向环形缓冲区写入一个字节
 * @param value 待写入的字节
 */
void RingBuffer_WriteByte(uint8_t value) {
    Push(value);
}

/**
 * @brief 向环形缓冲区写入一个字节数组
 * @param src 源数组
 * @param size 数组大小
 */
void RingBuffer_WriteByteArray(uint8_t* src, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        Push(src[i]);
    }
}

/**
 * @brief 向环形缓冲区写入一个16位整数
 * @param value 待写入的16位整数
 */
void RingBuffer_WriteShort(int16_t value) {
    uint8_t *p = (uint8_t *)&value;
    RingBuffer_WriteByteArray(p, 2);
}

/**
 * @brief 向环形缓冲区写入一个16位无符号整数
 * @param value 待写入的16位无符号整数
 */
void RingBuffer_WriteUShort(uint16_t value) {
    uint8_t *p = (uint8_t *)&value;
    RingBuffer_WriteByteArray(p, 2);
}

/**
 * @brief 向环形缓冲区写入一个32位整数
 * @param value 待写入的32位整数
 */
void RingBuffer_WriteLong(int32_t value) {
    uint8_t *p = (uint8_t *)&value;
    RingBuffer_WriteByteArray(p, 4);
}

/**
 * @brief 向环形缓冲区写入一个32位无符号整数
 * @param value 待写入的32位无符号整数
 */
void RingBuffer_WriteULong(uint32_t value) {
    uint8_t *p = (uint8_t *)&value;
    RingBuffer_WriteByteArray(p, 4);
}

/**
 * @brief 从环形缓冲区读取一个字节
 * @param index 字节索引
 * @return 读取到的字节
 */
uint8_t RingBuffer_PeekByte(uint16_t index) {
    if (index >= GetByteUsed()) return 0;
    uint16_t rbIdx = (ringHead + index + 1) % ringSize;
    return ServoRecvBuf[rbIdx];
}

/**
 * @brief 从环形缓冲区读取一个16位无符号整数
 * @param index 字节索引
 * @return 读取到的16位无符号整数
 */
uint16_t RingBuffer_PeekUShort(uint16_t index) {
    if (index + 1 >= GetByteUsed()) return 0;
    uint16_t rbIdx = (ringHead + index + 1) % ringSize;
    return (ServoRecvBuf[rbIdx] | (ServoRecvBuf[(rbIdx + 1) % ringSize] << 8));
}

/**
 * @brief 计算环形缓冲区中的校验和
 * @return 校验和
 */
uint8_t RingBuffer_GetChecksum(void) {
    uint16_t nByte = GetByteUsed();
    uint32_t bSum = 0;
    for (uint16_t i = 0; i < nByte; i++) {
        uint16_t rbIdx = (ringHead + i + 1) % ringSize;
        bSum = (bSum + ServoRecvBuf[rbIdx]) % 256;
    }
    return (uint8_t)bSum;
}

/**
 * @brief 初始化伺服驱动器
 */
void ServoDriver_Init(void) {
    RingBuffer_Init(SERVO_UART_RECV_BUF_SIZE);
    Servo_Uart_Receive_IT((uint8_t *)&ServoRecvBuf[0], 1);
}

static uint8_t rcv_temp;

/**
 * @brief 从串口接收数据
 * @param data 接收数据指针
 * @param size 接收数据大小
 */
void Servo_Uart_Receive_IT(uint8_t* data, uint16_t size) {
    HAL_UART_Receive_IT(&huart3, data, size);
}

/**
 * @brief 串口接收完成回调函数
 */
void Servo_Uart_RxCpltCallback(void) {
    ringTail = (ringTail + 1) % ringSize;
    osSemaphoreRelease(ServoUartRxSemHandle);
    Servo_Uart_Receive_IT(&rcv_temp, 1);
}

/**
 * @brief 向串口发送数据
 * @param data 发送数据指针
 * @param size 发送数据大小
 */
void Servo_Uart_Send(uint8_t* data, uint16_t size) {
    HAL_UART_Transmit_IT(&huart3, data, size);
}

/**
 * @brief 将FSUS包转换入环形缓冲区
 * @param pkg FSUS包指针
 */
void FSUS_Package2RingBuffer(PackageTypeDef *pkg) {
    uint8_t checksum;
    RingBuffer_Reset();
    RingBuffer_WriteUShort(pkg->header);
    RingBuffer_WriteByte(pkg->cmdId);

    if (pkg->isSync || pkg->size > 255) {
        RingBuffer_WriteByte(0xFF);
        RingBuffer_WriteUShort(pkg->size);
    } else {
        RingBuffer_WriteByte((uint8_t)pkg->size);
    }

    RingBuffer_WriteByteArray(pkg->content, pkg->size);
    checksum = RingBuffer_GetChecksum();
    RingBuffer_WriteByte(checksum);
}

/**
 * @brief 计算FSUS包的校验和
 * @param pkg FSUS包指针
 * @return 校验和
 */
uint8_t FSUS_CalcChecksum(PackageTypeDef *pkg) {
    RingBuffer_Reset();
    FSUS_Package2RingBuffer(pkg);
    return RingBuffer_GetChecksum();
}

/**
 * @brief 检查FSUS包是否有效
 * @param pkg FSUS包指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_IsValidResponsePackage(PackageTypeDef *pkg) {
    if (pkg->header != FSUS_PACK_RESPONSE_HEADER)
        return FSUS_STATUS_WRONG_RESPONSE_HEADER;
    if (pkg->cmdId > FSUS_CMD_NUM)
        return FSUS_STATUS_UNKOWN_CMD_ID;
    if (pkg->size > (FSUS_PACK_RESPONSE_MAX_SIZE - 6))
        return FSUS_STATUS_SIZE_TOO_BIG;
    if (FSUS_CalcChecksum(pkg) != pkg->checksum)
        return FSUS_STATUS_CHECKSUM_ERROR;
    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送FSUS包
 * @param cmdId 命令ID
 * @param size 数据大小
 * @param content 数据指针
 * @param isSync 是否同步
 */
void FSUS_SendPackage_Common(uint8_t cmdId, uint16_t size, uint8_t *content, uint8_t isSync) {
    PackageTypeDef pkg = {
        .header = FSUS_PACK_REQUEST_HEADER,
        .cmdId = cmdId,
        .size = size,
        .isSync = isSync
    };
    memcpy(pkg.content, content, size);
    FSUS_Package2RingBuffer(&pkg);

    uint8_t sendBuf[FSUS_PACK_RESPONSE_MAX_SIZE + 10];
    uint16_t sendSize = 0;

    sendBuf[sendSize++] = (uint8_t)(pkg.header & 0xFF);
    sendBuf[sendSize++] = (uint8_t)((pkg.header >> 8) & 0xFF);
    sendBuf[sendSize++] = pkg.cmdId;

    if (pkg.isSync || pkg.size > 255) {
        sendBuf[sendSize++] = 0xFF;
        sendBuf[sendSize++] = (uint8_t)(pkg.size & 0xFF);
        sendBuf[sendSize++] = (uint8_t)((pkg.size >> 8) & 0xFF);
    } else {
        sendBuf[sendSize++] = (uint8_t)pkg.size;
    }

    for (uint16_t i = 0; i < pkg.size; i++) {
        sendBuf[sendSize++] = pkg.content[i];
    }

    uint8_t checksumCalc = 0;
    for (uint16_t i = 0; i < sendSize; i++) {
        checksumCalc = (uint8_t)((checksumCalc + sendBuf[i]) % 256);
    }
    sendBuf[sendSize++] = checksumCalc;

    Servo_Uart_Send(sendBuf, sendSize);
}

/**
 * @brief 接收FSUS包
 * @param pkg FSUS包指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_RecvPackage(PackageTypeDef *pkg) {
    uint16_t header = 0;
    uint32_t timeout = osKernelGetTickCount() + pdMS_TO_TICKS(FSUS_TIMEOUT_MS);

    while (osKernelGetTickCount() < timeout) {
        if (GetByteUsed() < 5) {
            osSemaphoreAcquire(ServoUartRxSemHandle, pdMS_TO_TICKS(10));
            continue;
        }

        header = RingBuffer_PeekUShort(0);
        if (header != FSUS_PACK_RESPONSE_HEADER) {
            RingBuffer_ReadByte();
            continue;
        }

        uint16_t size;
        if (RingBuffer_PeekByte(3) == 0xFF) {
            size = RingBuffer_PeekUShort(4);
            pkg->isSync = 1;
        } else {
            size = RingBuffer_PeekByte(3);
            pkg->isSync = 0;
        }

        uint16_t totalLen = 5 + size + (pkg->isSync ? 2 : 0);
        if (GetByteUsed() < totalLen) {
            osSemaphoreAcquire(ServoUartRxSemHandle, pdMS_TO_TICKS(10));
            continue;
        }

        pkg->header = RingBuffer_ReadUShort();
        pkg->cmdId = RingBuffer_ReadByte();
        if (pkg->isSync) {
            RingBuffer_ReadByte();
            pkg->size = RingBuffer_ReadUShort();
        } else {
            pkg->size = RingBuffer_ReadByte();
        }
        RingBuffer_ReadByteArray(pkg->content, pkg->size);
        pkg->checksum = RingBuffer_ReadByte();

        return FSUS_IsValidResponsePackage(pkg);
    }
    return FSUS_STATUS_TIMEOUT;
}

/**
 * @brief 接收同步FSUS包
 * @param pkg FSUS包指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_sync_RecvPackage(PackageTypeDef *pkg) {
    pkg->status = 0;
    uint8_t bIdx = 0;
    uint16_t header = 0;
    uint8_t byte;
    uint32_t timeout = osKernelGetTickCount() + pdMS_TO_TICKS(FSUS_TIMEOUT_MS);

    while (osKernelGetTickCount() < timeout) {
        if (GetByteUsed() == 0) {
            osSemaphoreAcquire(ServoUartRxSemHandle, pdMS_TO_TICKS(10));
            continue;
        }
        byte = RingBuffer_ReadByte();

        if ((pkg->status & FSUS_RECV_FLAG_HEADER) == 0) {
            if (header == 0 && byte == 0x05) {
                header = byte;
            } else if (header == 0x05 && byte == 0x1C) {
                pkg->header = 0x1C05;
                pkg->status |= FSUS_RECV_FLAG_HEADER;
                header = 0;
            } else {
                header = 0;
            }
            continue;
        }
        if ((pkg->status & FSUS_RECV_FLAG_CMD_ID) == 0) {
            pkg->cmdId = byte;
            if (pkg->cmdId != 22) {
                return FSUS_STATUS_UNKOWN_CMD_ID;
            }
            pkg->status |= FSUS_RECV_FLAG_CMD_ID;
            continue;
        }
        if ((pkg->status & FSUS_RECV_FLAG_SIZE) == 0) {
            pkg->size = byte;
            if (pkg->size != 16) {
                return FSUS_STATUS_SIZE_TOO_BIG;
            }
            pkg->status |= FSUS_RECV_FLAG_SIZE;
            continue;
        }
        if ((pkg->status & FSUS_RECV_FLAG_CONTENT) == 0) {
            pkg->content[bIdx++] = byte;
            if (bIdx >= pkg->size) {
                pkg->status |= FSUS_RECV_FLAG_CONTENT;
            }
            continue;
        }
        if ((pkg->status & FSUS_RECV_FLAG_CHECKSUM) == 0) {
            pkg->checksum = byte;
            if (FSUS_CalcChecksum(pkg) != pkg->checksum) {
                return FSUS_STATUS_CHECKSUM_ERROR;
            }
            return FSUS_STATUS_SUCCESS;
        }
    }
    return FSUS_STATUS_TIMEOUT;
}

/**
 * @brief 从环形缓冲区读取16位无符号整数
 * @return 无符号整数
 */
uint16_t RingBuffer_ReadUShort(void) {
    uint16_t value;
    uint8_t *p = (uint8_t *)&value;
    RingBuffer_ReadByteArray(p, 2);
    return value;
}

/**
 * @brief 发送Ping命令
 * @param servo_id 伺服机ID
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_Ping(uint8_t servo_id) {
    uint8_t statusCode;
    uint8_t ehcoServoId;

    FSUS_SendPackage_Common(FSUS_CMD_PING, 1, &servo_id, 0);

    PackageTypeDef pkg;
    statusCode = FSUS_RecvPackage(&pkg);
    if (statusCode == FSUS_STATUS_SUCCESS) {
        ehcoServoId = (uint8_t)pkg.content[0];
        if (ehcoServoId != servo_id) {
            return FSUS_STATUS_ID_NOT_MATCH;
        }
    }
    return statusCode;
}

/**
 * @brief 发送ResetUserData命令
 * @param servo_id 伺服机ID
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_ResetUserData(uint8_t servo_id) {
    const uint8_t size = 1;
    FSUS_STATUS statusCode;

    FSUS_SendPackage_Common(FSUS_CMD_RESET_USER_DATA, size, &servo_id, 0);

    PackageTypeDef pkg;
    statusCode = FSUS_RecvPackage(&pkg);
    if (statusCode == FSUS_STATUS_SUCCESS) {
        uint8_t result = (uint8_t)pkg.content[1];
        if (result == 1) {
            return FSUS_STATUS_SUCCESS;
        } else {
            return FSUS_STATUS_FAIL;
        }
    }
    return statusCode;
}

/**
 * @brief 发送ReadData命令
 * @param servo_id 伺服机ID
 * @param address 地址
 * @param value 数据指针
 * @param size 数据大小指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_ReadData(uint8_t servo_id, uint8_t address, uint8_t *value, uint8_t *size) {
    FSUS_STATUS statusCode;
    uint8_t buffer[2] = {servo_id, address};

    FSUS_SendPackage_Common(FSUS_CMD_READ_DATA, 2, buffer, 0);

    PackageTypeDef pkg;
    statusCode = FSUS_RecvPackage(&pkg);
    if (statusCode == FSUS_STATUS_SUCCESS) {
        *size = pkg.size - 2;
        for (int i = 0; i < *size; i++) {
            value[i] = pkg.content[i + 2];
        }
    }
    return statusCode;
}

/**
 * @brief 发送WriteData命令
 * @param servo_id 伺服机ID
 * @param address 地址
 * @param value 数据指针
 * @param size 数据大小
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_WriteData(uint8_t servo_id, uint8_t address, uint8_t *value, uint8_t size) {
    FSUS_STATUS statusCode;
    uint8_t buffer[size + 2];
    buffer[0] = servo_id;
    buffer[1] = address;
    for (int i = 0; i < size; i++) {
        buffer[i + 2] = value[i];
    }

    FSUS_SendPackage_Common(FSUS_CMD_WRITE_DATA, size + 2, buffer, 0);

    PackageTypeDef pkg;
    statusCode = FSUS_RecvPackage(&pkg);
    if (statusCode == FSUS_STATUS_SUCCESS) {
        uint8_t result = pkg.content[2];
        if (result == 1) {
            statusCode = FSUS_STATUS_SUCCESS;
        } else {
            statusCode = FSUS_STATUS_FAIL;
        }
    }
    return statusCode;
}

/**
 * @brief 发送SetServoAngle命令
 * @param servo_id 伺服机ID
 * @param angle 角度
 * @param interval 间隔
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SetServoAngle(uint8_t servo_id, float angle, uint16_t interval, uint16_t power) {
    const uint8_t size = 7;
    uint8_t buffer[size + 1];

    if (angle > 180.0f) {
        angle = 180.0f;
    } else if (angle < -180.0f) {
        angle = -180.0f;
    }

    buffer[0] = servo_id;
    buffer[1] = (uint8_t)((int16_t)(10 * angle) & 0xFF);
    buffer[2] = (uint8_t)(((int16_t)(10 * angle) >> 8) & 0xFF);
    buffer[3] = (uint8_t)(interval & 0xFF);
    buffer[4] = (uint8_t)((interval >> 8) & 0xFF);
    buffer[5] = (uint8_t)(power & 0xFF);
    buffer[6] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_ROTATE, size, buffer + 1, 0);

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送SetServoAngleByInterval命令
 * @param servo_id 伺服机ID
 * @param angle 角度
 * @param interval 间隔
 * @param t_acc 加速度
 * @param t_dec 减速度
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SetServoAngleByInterval(uint8_t servo_id, float angle, uint16_t interval,
                                           uint16_t t_acc, uint16_t t_dec, uint16_t power) {
    const uint8_t size = 11;
    uint8_t buffer[size + 1];

    if (angle > 180.0f) {
        angle = 180.0f;
    } else if (angle < -180.0f) {
        angle = -180.0f;
    }
    if (t_acc < 20) t_acc = 20;
    if (t_dec < 20) t_dec = 20;

    buffer[0] = servo_id;
    buffer[1] = (uint8_t)((int16_t)(10 * angle) & 0xFF);
    buffer[2] = (uint8_t)(((int16_t)(10 * angle) >> 8) & 0xFF);
    buffer[3] = (uint8_t)(interval & 0xFF);
    buffer[4] = (uint8_t)((interval >> 8) & 0xFF);
    buffer[5] = (uint8_t)(t_acc & 0xFF);
    buffer[6] = (uint8_t)((t_acc >> 8) & 0xFF);
    buffer[7] = (uint8_t)(t_dec & 0xFF);
    buffer[8] = (uint8_t)((t_dec >> 8) & 0xFF);
    buffer[9] = (uint8_t)(power & 0xFF);
    buffer[10] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_ANGLE_BY_INTERVAL, size, buffer + 1, 0);

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送SetServoAngleByVelocity命令
 * @param servo_id 伺服机ID
 * @param angle 角度
 * @param velocity 速度
 * @param t_acc 加速度
 * @param t_dec 减速度
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SetServoAngleByVelocity(uint8_t servo_id, float angle, float velocity,
                                          uint16_t t_acc, uint16_t t_dec, uint16_t power) {
    const uint8_t size = 11;
    uint8_t buffer[size + 1];

    if (angle > 180.0f) {
        angle = 180.0f;
    } else if (angle < -180.0f) {
        angle = -180.0f;
    }
    if (velocity < 1.0f) velocity = 1.0f;
    else if (velocity > 750.0f) velocity = 750.0f;
    if (t_acc < 20) t_acc = 20;
    if (t_dec < 20) t_dec = 20;

    buffer[0] = servo_id;
    buffer[1] = (uint8_t)((int16_t)(10 * angle) & 0xFF);
    buffer[2] = (uint8_t)(((int16_t)(10 * angle) >> 8) & 0xFF);
    buffer[3] = (uint8_t)((uint16_t)(10 * velocity) & 0xFF);
    buffer[4] = (uint8_t)(((uint16_t)(10 * velocity) >> 8) & 0xFF);
    buffer[5] = (uint8_t)(t_acc & 0xFF);
    buffer[6] = (uint8_t)((t_acc >> 8) & 0xFF);
    buffer[7] = (uint8_t)(t_dec & 0xFF);
    buffer[8] = (uint8_t)((t_dec >> 8) & 0xFF);
    buffer[9] = (uint8_t)(power & 0xFF);
    buffer[10] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_ANGLE_BY_VELOCITY, size, buffer + 1, 0);

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送QueryServoAngle命令
 * @param servo_id 伺服机ID
 * @param angle 角度指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_QueryServoAngle(uint8_t servo_id, float *angle) {
    const uint8_t size = 1;
    uint8_t ehcoServoId;
    int16_t echoAngle;

    FSUS_SendPackage_Common(FSUS_CMD_READ_ANGLE, size, &servo_id, 0);

    PackageTypeDef pkg;
    uint8_t statusCode = FSUS_RecvPackage(&pkg);
    if (statusCode == FSUS_STATUS_SUCCESS) {
        ehcoServoId = (uint8_t)pkg.content[0];
        if (ehcoServoId != servo_id) {
            return FSUS_STATUS_ID_NOT_MATCH;
        }
        echoAngle = (int16_t)(pkg.content[1] | (pkg.content[2] << 8));
        *angle = (float)(echoAngle / 10.0);
    }
    return statusCode;
}

/**
 * @brief 发送SetServoAngleMTurn命令
 * @param servo_id 伺服机ID
 * @param angle 角度
 * @param interval 间隔
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SetServoAngleMTurn(uint8_t servo_id, float angle, uint32_t interval, uint16_t power) {
    const uint8_t size = 11;
    uint8_t buffer[size + 1];

    if (angle > 368640.0f) {
        angle = 368640.0f;
    } else if (angle < -368640.0f) {
        angle = -368640.0f;
    }
    if (interval > 4096000) {
        interval = 4096000;
    }

    buffer[0] = servo_id;
    int32_t angle10 = (int32_t)(10 * angle);
    buffer[1] = (uint8_t)(angle10 & 0xFF);
    buffer[2] = (uint8_t)((angle10 >> 8) & 0xFF);
    buffer[3] = (uint8_t)((angle10 >> 16) & 0xFF);
    buffer[4] = (uint8_t)((angle10 >> 24) & 0xFF);
    buffer[5] = (uint8_t)(interval & 0xFF);
    buffer[6] = (uint8_t)((interval >> 8) & 0xFF);
    buffer[7] = (uint8_t)((interval >> 16) & 0xFF);
    buffer[8] = (uint8_t)((interval >> 24) & 0xFF);
    buffer[9] = (uint8_t)(power & 0xFF);
    buffer[10] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_ANGLE_MTURN, size, buffer + 1, 0);

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送SetServoAngleMTurnByInterval命令
 * @param servo_id 伺服机ID
 * @param angle 角度
 * @param interval 间隔
 * @param t_acc 加速度
 * @param t_dec 减速度
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SetServoAngleMTurnByInterval(uint8_t servo_id, float angle, uint32_t interval,
                                                uint16_t t_acc, uint16_t t_dec, uint16_t power) {
    const uint8_t size = 15;
    uint8_t buffer[size + 1];

    if (angle > 368640.0f) {
        angle = 368640.0f;
    } else if (angle < -368640.0f) {
        angle = -368640.0f;
    }
    if (interval > 4096000) {
        interval = 4096000;
    }
    if (t_acc < 20) t_acc = 20;
    if (t_dec < 20) t_dec = 20;

    buffer[0] = servo_id;
    int32_t angle10 = (int32_t)(10 * angle);
    buffer[1] = (uint8_t)(angle10 & 0xFF);
    buffer[2] = (uint8_t)((angle10 >> 8) & 0xFF);
    buffer[3] = (uint8_t)((angle10 >> 16) & 0xFF);
    buffer[4] = (uint8_t)((angle10 >> 24) & 0xFF);
    buffer[5] = (uint8_t)(interval & 0xFF);
    buffer[6] = (uint8_t)((interval >> 8) & 0xFF);
    buffer[7] = (uint8_t)((interval >> 16) & 0xFF);
    buffer[8] = (uint8_t)((interval >> 24) & 0xFF);
    buffer[9] = (uint8_t)(t_acc & 0xFF);
    buffer[10] = (uint8_t)((t_acc >> 8) & 0xFF);
    buffer[11] = (uint8_t)(t_dec & 0xFF);
    buffer[12] = (uint8_t)((t_dec >> 8) & 0xFF);
    buffer[13] = (uint8_t)(power & 0xFF);
    buffer[14] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_ANGLE_MTURN_BY_INTERVAL, size, buffer + 1, 0);

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送SetServoAngleMTurnByVelocity命令
 * @param servo_id 伺服机ID
 * @param angle 角度
 * @param velocity 速度
 * @param t_acc 加速度
 * @param t_dec 减速度
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SetServoAngleMTurnByVelocity(uint8_t servo_id, float angle, float velocity,
                                                uint16_t t_acc, uint16_t t_dec, uint16_t power) {
    const uint8_t size = 13;
    uint8_t buffer[size + 1];

    if (angle > 368640.0f) {
        angle = 368640.0f;
    } else if (angle < -368640.0f) {
        angle = -368640.0f;
    }
    if (velocity < 1.0f) velocity = 1.0f;
    else if (velocity > 750.0f) velocity = 750.0f;
    if (t_acc < 20) t_acc = 20;
    if (t_dec < 20) t_dec = 20;

    buffer[0] = servo_id;
    int32_t angle10 = (int32_t)(10 * angle);
    buffer[1] = (uint8_t)(angle10 & 0xFF);
    buffer[2] = (uint8_t)((angle10 >> 8) & 0xFF);
    buffer[3] = (uint8_t)((angle10 >> 16) & 0xFF);
    buffer[4] = (uint8_t)((angle10 >> 24) & 0xFF);
    buffer[5] = (uint8_t)((uint16_t)(10 * velocity) & 0xFF);
    buffer[6] = (uint8_t)(((uint16_t)(10 * velocity) >> 8) & 0xFF);
    buffer[7] = (uint8_t)(t_acc & 0xFF);
    buffer[8] = (uint8_t)((t_acc >> 8) & 0xFF);
    buffer[9] = (uint8_t)(t_dec & 0xFF);
    buffer[10] = (uint8_t)((t_dec >> 8) & 0xFF);
    buffer[11] = (uint8_t)(power & 0xFF);
    buffer[12] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_ANGLE_MTURN_BY_VELOCITY, size, buffer + 1, 0);

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送QueryServoAngleMTurn命令
 * @param servo_id 伺服机ID
 * @param angle 角度指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_QueryServoAngleMTurn(uint8_t servo_id, float *angle) {
    const uint8_t size = 1;
    uint8_t ehcoServoId;
    int32_t echoAngle;

    FSUS_SendPackage_Common(FSUS_CMD_QUERY_SERVO_ANGLE_MTURN, size, &servo_id, 0);

    PackageTypeDef pkg;
    uint8_t statusCode = FSUS_RecvPackage(&pkg);
    if (statusCode == FSUS_STATUS_SUCCESS) {
        ehcoServoId = (uint8_t)pkg.content[0];
        if (ehcoServoId != servo_id) {
            return FSUS_STATUS_ID_NOT_MATCH;
        }
        echoAngle = (int32_t)(pkg.content[1] | (pkg.content[2] << 8) |
                               (pkg.content[3] << 16) | (pkg.content[4] << 24));
        *angle = (float)(echoAngle / 10.0);
    }
    return statusCode;
}

/**
 * @brief 发送DampingMode命令
 * @param servo_id 伺服机ID
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_DampingMode(uint8_t servo_id, uint16_t power) {
    const uint8_t size = 3;
    uint8_t buffer[size + 1];

    buffer[0] = servo_id;
    buffer[1] = (uint8_t)(power & 0xFF);
    buffer[2] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_DAMPING, size, buffer + 1, 0);
    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送ServoAngleReset命令
 * @param servo_id 伺服机ID
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_ServoAngleReset(uint8_t servo_id) {
    FSUS_SendPackage_Common(FSUS_CMD_RESERT_SERVO_ANGLE_MTURN, 1, &servo_id, 0);
    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送SetOriginPoint命令
 * @param servo_id 伺服机ID
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SetOriginPoint(uint8_t servo_id) {
    FSUS_SendPackage_Common(FSUS_CMD_SET_ORIGIN_POINT, 2, &servo_id, 0);
    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送BeginAsync命令
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_BeginAsync(void) {
    uint8_t buffer[1] = {0};
    FSUS_SendPackage_Common(FSUS_CMD_BEGIN_ASYNC, 0, buffer + 1, 0);
    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送EndAsync命令
 * @param mode 模式
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_EndAsync(uint8_t mode) {
    uint8_t buffer[2] = {mode, 0};
    FSUS_SendPackage_Common(FSUS_CMD_END_ASYNC, 1, buffer + 1, 0);
    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送ServoMonitor命令
 * @param servo_id 伺服机ID
 * @param servoData 伺服机数据指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_ServoMonitor(uint8_t servo_id, ServoData servoData[]) {
    const uint8_t size = 1;
    uint8_t buffer[size + 1];
    double temp;

    buffer[0] = servo_id;
    FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_ReadData, size, buffer + 1, 0);

    PackageTypeDef pkg;
    FSUS_STATUS status = FSUS_RecvPackage(&pkg);

    if (status != FSUS_STATUS_SUCCESS) {
        return status;
    }

    servoData[0].id = pkg.content[0];
    servoData[0].voltage = (int16_t)((pkg.content[2] << 8) | pkg.content[1]);
    servoData[0].current = (int16_t)((pkg.content[4] << 8) | pkg.content[3]);
    servoData[0].power = (int16_t)((pkg.content[6] << 8) | pkg.content[5]);
    servoData[0].temperature = (int16_t)((pkg.content[8] << 8) | pkg.content[7]);
    temp = (double)servoData[0].temperature;
    servoData[0].temperature = (int16_t)(1 / (log(temp / (4096.0f - temp)) / 3435.0f + 1 / (273.15 + 25)) - 273.15);
    servoData[0].status = pkg.content[9];
    servoData[0].angle = (int32_t)((pkg.content[13] << 24) | (pkg.content[12] << 16) |
                                   (pkg.content[11] << 8) | pkg.content[10]);
    servoData[0].angle = (float)(servoData[0].angle / 10.0f);
    servoData[0].circle_count = (int16_t)((pkg.content[15] << 8) | pkg.content[14]);

    pkg.status = 0;

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送StopOnControlMode命令
 * @param servo_id 伺服机ID
 * @param mode 模式
 * @param power 功率
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_StopOnControlMode(uint8_t servo_id, uint8_t mode, uint16_t power) {
    const uint8_t size = 4;
    uint8_t buffer[size + 1];

    buffer[0] = servo_id;
    buffer[1] = mode | 0x10;
    buffer[2] = (uint8_t)(power & 0xFF);
    buffer[3] = (uint8_t)((power >> 8) & 0xFF);

    FSUS_SendPackage_Common(FSUS_CMD_CONTROL_MODE_STOP, size, buffer + 1, 0);

    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送SyncServoMonitor命令
 * @param servo_count 伺服机数量
 * @param servoData 伺服机数据指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SyncServoMonitor(uint8_t servo_count, ServoData servoData[]) {
    PackageTypeDef pkg;
    FSUS_STATUS status;
    double temp;
    uint8_t packet_count = 0;

    while (packet_count < servo_count) {
        status = FSUS_sync_RecvPackage(&pkg);

        if (status != FSUS_STATUS_SUCCESS) {
            return status;
        }

        servoData[packet_count].id = pkg.content[0];
        servoData[packet_count].voltage = (int16_t)((pkg.content[2] << 8) | pkg.content[1]);
        servoData[packet_count].current = (int16_t)((pkg.content[4] << 8) | pkg.content[3]);
        servoData[packet_count].power = (int16_t)((pkg.content[6] << 8) | pkg.content[5]);
        servoData[packet_count].temperature = (int16_t)((pkg.content[8] << 8) | pkg.content[7]);
        temp = (double)servoData[packet_count].temperature;
        servoData[packet_count].temperature = (int16_t)(1 / (log(temp / (4096.0f - temp)) / 3435.0f + 1 / (273.15 + 25)) - 273.15);
        servoData[packet_count].status = pkg.content[9];
        servoData[packet_count].angle = (int32_t)((pkg.content[13] << 24) | (pkg.content[12] << 16) |
                                                  (pkg.content[11] << 8) | pkg.content[10]);
        servoData[packet_count].angle = (float)(servoData[packet_count].angle / 10.0f);
        servoData[packet_count].circle_count = (int16_t)((pkg.content[15] << 8) | pkg.content[14]);

        packet_count++;
        pkg.status = 0;
    }

    RingBuffer_Reset();
    return FSUS_STATUS_SUCCESS;
}

/**
 * @brief 发送SyncCommand命令
 * @param servo_count 伺服机数量
 * @param ServoMode 伺服机模式
 * @param servoSync 伺服机同步数据指针
 * @return FSUS_STATUS 状态
 */
FSUS_STATUS FSUS_SyncCommand(uint8_t servo_count, uint8_t ServoMode, FSUS_sync_servo servoSync[]) {
    uint8_t buffer[3 + servo_count * 15];
    uint16_t size;

    switch (ServoMode) {
        case MODE_SET_SERVO_ANGLE:
            size = 3 + servo_count * 7;
            buffer[0] = 8;
            buffer[1] = 7;
            buffer[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                uint16_t idx = 3 + i * 7;
                buffer[idx] = servoSync[i].id;
                int16_t angle10 = (int16_t)(servoSync[i].angle > 180.0f ? 1800 : (servoSync[i].angle < -180.0f ? -1800 : (int16_t)(10 * servoSync[i].angle)));
                buffer[idx + 1] = (uint8_t)(angle10 & 0xFF);
                buffer[idx + 2] = (uint8_t)((angle10 >> 8) & 0xFF);
                buffer[idx + 3] = (uint8_t)(servoSync[i].interval_single & 0xFF);
                buffer[idx + 4] = (uint8_t)((servoSync[i].interval_single >> 8) & 0xFF);
                buffer[idx + 5] = (uint8_t)(servoSync[i].power & 0xFF);
                buffer[idx + 6] = (uint8_t)((servoSync[i].power >> 8) & 0xFF);
            }
            break;

        case MODE_SET_SERVO_ANGLE_BY_INTERVAL:
            size = 3 + servo_count * 11;
            buffer[0] = 11;
            buffer[1] = 11;
            buffer[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                uint16_t idx = 3 + i * 11;
                buffer[idx] = servoSync[i].id;
                int16_t angle10 = (int16_t)(servoSync[i].angle > 180.0f ? 1800 : (servoSync[i].angle < -180.0f ? -1800 : (int16_t)(10 * servoSync[i].angle)));
                buffer[idx + 1] = (uint8_t)(angle10 & 0xFF);
                buffer[idx + 2] = (uint8_t)((angle10 >> 8) & 0xFF);
                buffer[idx + 3] = (uint8_t)(servoSync[i].interval_single & 0xFF);
                buffer[idx + 4] = (uint8_t)((servoSync[i].interval_single >> 8) & 0xFF);
                buffer[idx + 5] = (uint8_t)(servoSync[i].t_acc & 0xFF);
                buffer[idx + 6] = (uint8_t)((servoSync[i].t_acc >> 8) & 0xFF);
                buffer[idx + 7] = (uint8_t)(servoSync[i].t_dec & 0xFF);
                buffer[idx + 8] = (uint8_t)((servoSync[i].t_dec >> 8) & 0xFF);
                buffer[idx + 9] = (uint8_t)(servoSync[i].power & 0xFF);
                buffer[idx + 10] = (uint8_t)((servoSync[i].power >> 8) & 0xFF);
            }
            break;

        case MODE_SET_SERVO_ANGLE_BY_VELOCITY:
            size = 3 + servo_count * 11;
            buffer[0] = 12;
            buffer[1] = 11;
            buffer[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                uint16_t idx = 3 + i * 11;
                buffer[idx] = servoSync[i].id;
                int16_t angle10 = (int16_t)(servoSync[i].angle > 180.0f ? 1800 : (servoSync[i].angle < -180.0f ? -1800 : (int16_t)(10 * servoSync[i].angle)));
                buffer[idx + 1] = (uint8_t)(angle10 & 0xFF);
                buffer[idx + 2] = (uint8_t)((angle10 >> 8) & 0xFF);
                buffer[idx + 3] = (uint8_t)((uint16_t)(10.0f * servoSync[i].velocity) & 0xFF);
                buffer[idx + 4] = (uint8_t)(((uint16_t)(10.0f * servoSync[i].velocity) >> 8) & 0xFF);
                buffer[idx + 5] = (uint8_t)(servoSync[i].t_acc & 0xFF);
                buffer[idx + 6] = (uint8_t)((servoSync[i].t_acc >> 8) & 0xFF);
                buffer[idx + 7] = (uint8_t)(servoSync[i].t_dec & 0xFF);
                buffer[idx + 8] = (uint8_t)((servoSync[i].t_dec >> 8) & 0xFF);
                buffer[idx + 9] = (uint8_t)(servoSync[i].power & 0xFF);
                buffer[idx + 10] = (uint8_t)((servoSync[i].power >> 8) & 0xFF);
            }
            break;

        case MODE_SET_SERVO_ANGLE_MTURN:
            size = 3 + servo_count * 11;
            buffer[0] = 13;
            buffer[1] = 11;
            buffer[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                uint16_t idx = 3 + i * 11;
                buffer[idx] = servoSync[i].id;
                int32_t angle10 = (int32_t)(servoSync[i].angle > 368640.0f ? 3686400 : (servoSync[i].angle < -368640.0f ? -3686400 : (int32_t)(10 * servoSync[i].angle)));
                buffer[idx + 1] = (uint8_t)(angle10 & 0xFF);
                buffer[idx + 2] = (uint8_t)((angle10 >> 8) & 0xFF);
                buffer[idx + 3] = (uint8_t)((angle10 >> 16) & 0xFF);
                buffer[idx + 4] = (uint8_t)((angle10 >> 24) & 0xFF);
                buffer[idx + 5] = (uint8_t)(servoSync[i].interval_multi & 0xFF);
                buffer[idx + 6] = (uint8_t)((servoSync[i].interval_multi >> 8) & 0xFF);
                buffer[idx + 7] = (uint8_t)((servoSync[i].interval_multi >> 16) & 0xFF);
                buffer[idx + 8] = (uint8_t)((servoSync[i].interval_multi >> 24) & 0xFF);
                buffer[idx + 9] = (uint8_t)(servoSync[i].power & 0xFF);
                buffer[idx + 10] = (uint8_t)((servoSync[i].power >> 8) & 0xFF);
            }
            break;

        case MODE_SET_SERVO_ANGLE_MTURN_BY_INTERVAL:
            size = 3 + servo_count * 15;
            buffer[0] = 14;
            buffer[1] = 15;
            buffer[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                uint16_t idx = 3 + i * 15;
                buffer[idx] = servoSync[i].id;
                int32_t angle10 = (int32_t)(servoSync[i].angle > 368640.0f ? 3686400 : (servoSync[i].angle < -368640.0f ? -3686400 : (int32_t)(10 * servoSync[i].angle)));
                buffer[idx + 1] = (uint8_t)(angle10 & 0xFF);
                buffer[idx + 2] = (uint8_t)((angle10 >> 8) & 0xFF);
                buffer[idx + 3] = (uint8_t)((angle10 >> 16) & 0xFF);
                buffer[idx + 4] = (uint8_t)((angle10 >> 24) & 0xFF);
                buffer[idx + 5] = (uint8_t)(servoSync[i].interval_multi & 0xFF);
                buffer[idx + 6] = (uint8_t)((servoSync[i].interval_multi >> 8) & 0xFF);
                buffer[idx + 7] = (uint8_t)((servoSync[i].interval_multi >> 16) & 0xFF);
                buffer[idx + 8] = (uint8_t)((servoSync[i].interval_multi >> 24) & 0xFF);
                buffer[idx + 9] = (uint8_t)(servoSync[i].t_acc & 0xFF);
                buffer[idx + 10] = (uint8_t)((servoSync[i].t_acc >> 8) & 0xFF);
                buffer[idx + 11] = (uint8_t)(servoSync[i].t_dec & 0xFF);
                buffer[idx + 12] = (uint8_t)((servoSync[i].t_dec >> 8) & 0xFF);
                buffer[idx + 13] = (uint8_t)(servoSync[i].power & 0xFF);
                buffer[idx + 14] = (uint8_t)((servoSync[i].power >> 8) & 0xFF);
            }
            break;

        case MODE_SET_SERVO_ANGLE_MTURN_BY_VELOCITY:
            size = 3 + servo_count * 13;
            buffer[0] = 15;
            buffer[1] = 13;
            buffer[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                uint16_t idx = 3 + i * 13;
                buffer[idx] = servoSync[i].id;
                int32_t angle10 = (int32_t)(servoSync[i].angle > 368640.0f ? 3686400 : (servoSync[i].angle < -368640.0f ? -3686400 : (int32_t)(10 * servoSync[i].angle)));
                buffer[idx + 1] = (uint8_t)(angle10 & 0xFF);
                buffer[idx + 2] = (uint8_t)((angle10 >> 8) & 0xFF);
                buffer[idx + 3] = (uint8_t)((angle10 >> 16) & 0xFF);
                buffer[idx + 4] = (uint8_t)((angle10 >> 24) & 0xFF);
                buffer[idx + 5] = (uint8_t)((uint16_t)(10.0f * servoSync[i].velocity) & 0xFF);
                buffer[idx + 6] = (uint8_t)(((uint16_t)(10.0f * servoSync[i].velocity) >> 8) & 0xFF);
                buffer[idx + 7] = (uint8_t)(servoSync[i].t_acc & 0xFF);
                buffer[idx + 8] = (uint8_t)((servoSync[i].t_acc >> 8) & 0xFF);
                buffer[idx + 9] = (uint8_t)(servoSync[i].t_dec & 0xFF);
                buffer[idx + 10] = (uint8_t)((servoSync[i].t_dec >> 8) & 0xFF);
                buffer[idx + 11] = (uint8_t)(servoSync[i].power & 0xFF);
                buffer[idx + 12] = (uint8_t)((servoSync[i].power >> 8) & 0xFF);
            }
            break;

        case MODE_Query_SERVO_Monitor:
            size = 3 + servo_count;
            buffer[0] = 22;
            buffer[1] = 1;
            buffer[2] = servo_count;
            for (int i = 0; i < servo_count; i++) {
                buffer[3 + i] = servoSync[i].id;
            }
            break;

        default:
            return FSUS_STATUS_ERRO;
    }

    if (size <= 255) {
        FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_SyncCommand, (uint8_t)size, buffer + 1, 0);
    } else {
        FSUS_SendPackage_Common(FSUS_CMD_SET_SERVO_SyncCommand, size, buffer + 1, 1);
    }

    if (ServoMode == MODE_Query_SERVO_Monitor) {
        FSUS_SyncServoMonitor(servo_count, servodata);
    }

    return FSUS_STATUS_SUCCESS;
}
