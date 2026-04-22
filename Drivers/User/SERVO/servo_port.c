/**
 * @file servo_port.c
 * @author (your name)
 * @brief 总线舵机驱动 FreeRTOS 适配层源文件
 */

#include "servo_port.h"
#include "usart.h"
#include "log.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "freeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "Events.h"
#include <string.h>
#include <stdlib.h>

/* 舵机 ID 列表  */
const uint8_t ServoIDList[SERVO_COUNT] = {1, 2, 3, 4, 5, 6};

/* 串口对象 */
static Usart_DataTypeDef servoUsart;

/**
 * @brief 舵机单圈角度控制命令（Shell接口）
 * @param argc 参数个数，必须为3或4
 * @param argv 参数数组
 *            argv[1]: 舵机ID (1~254)
 *            argv[2]: 目标角度 (-180°~180°，单圈范围)
 *            argv[3]: 运动时间（可选，单位ms，默认1000ms）
 * @note 控制舵机转动到指定角度，通过消息队列发送命令给舵机控制任务
 */
static void Servo_Angle_Shell(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        logPrintln("Usage: angle <id> <angle> [time_ms]");
        logPrintln("  id: 1~254, angle: -180~180 (single turn)");
        return;
    }
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_SET_ANGLE,
        .servoId = (uint8_t)atoi(argv[1]),
        .angle = (float)atof(argv[2]),
        .interval = (argc == 4) ? (uint16_t)atoi(argv[3]) : 1000,
        .power = 1000  // 默认功率
    };
    osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0);
    logPrintln("Servo %d set angle %.1f° in %d ms", cmd.servoId, cmd.angle, cmd.interval);
}

/**
 * @brief 舵机多圈角度控制命令(Shell接口)
 * @param argc 参数个数，必须为3或4
 * @param argv 参数数组
 *            argv[1]: 舵机ID (1~254)
 *            argv[2]: 目标角度 (-368640°~368640°，多圈范围)
 *            argv[3]: 运动时间（可选，单位ms，默认1000ms）
 * @note 控制舵机转动到指定多圈角度位置，支持多圈连续旋转
 */
static void Servo_Mturn_Shell(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        logPrintln("Usage: mturn <id> <angle> [time_ms]");
        logPrintln("  id: 1~254, angle: -368640~368640 (multi-turn)");
        return;
    }
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_SET_ANGLE_MTURN,
        .servoId = (uint8_t)atoi(argv[1]),
        .angle = (float)atof(argv[2]),
        .interval = (argc == 4) ? (uint16_t)atoi(argv[3]) : 1000,
        .power = 1000
    };
    osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0);
    logPrintln("Servo %d multi-turn angle %.1f° in %d ms", cmd.servoId, cmd.angle, cmd.interval);
}

/**
 * @brief 舵机阻尼模式控制命令(Shell接口)
 * @param argc 参数个数，必须为2或3
 * @param argv 参数数组
 *            argv[1]: 舵机ID (1~254)
 *            argv[2]: 阻尼功率（可选，单位mW，默认500mW）
 * @note 使舵机进入阻尼模式，可在外力作用下被动转动，功率越大阻力越大
 */
static void Servo_Damping_Shell(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        logPrintln("Usage: damping <id> [power_mW]");
        return;
    }
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_DAMPING,
        .servoId = (uint8_t)atoi(argv[1]),
        .power = (argc == 3) ? (uint16_t)atoi(argv[2]) : 500
    };
    osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0);
    logPrintln("Servo %d enter damping mode (power %d mW)", cmd.servoId, cmd.power);
}

/**
 * @brief 舵机停止命令(Shell接口)
 * @param argc 参数个数，必须为2
 * @param argv 参数数组，argv[1]为舵机ID
 * @note 停止指定舵机的当前运动，舵机进入停止状态
 */
static void Servo_Stop_Shell(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: stop <id>");
        return;
    }
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_STOP,
        .servoId = (uint8_t)atoi(argv[1])
    };
    osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0);
    logPrintln("Servo %d stop", cmd.servoId);
}

/**
 * @brief 舵机原点设置命令(Shell接口)
 * @param argc 参数个数，必须为2
 * @param argv 参数数组，argv[1]为舵机ID
 * @note 将舵机当前位置设置为机械原点，通过消息队列发送命令给舵机控制任务
 */
static void Servo_ResetOrigin_Shell(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: origin <id>");
        return;
    }
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_RESET_ORIGIN,
        .servoId = (uint8_t)atoi(argv[1])
    };
    osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0);
    logPrintln("Servo %d set current position as origin", cmd.servoId);
}

/**
 * @brief 舵机多圈角度复位命令(Shell接口)
 * @param argc 参数个数，必须为2
 * @param argv 参数数组，argv[1]为舵机ID
 * @note 复位舵机的多圈角度值，通过消息队列发送命令给舵机控制任务
 */
static void Servo_ResetAngle_Shell(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: reset_angle <id>");
        return;
    }
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_RESET_ANGLE,
        .servoId = (uint8_t)atoi(argv[1])
    };
    osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0);
    logPrintln("Servo %d multi-turn angle reset", cmd.servoId);
}

/**
 * @brief 舵机监控数据显示命令(Shell接口)
 * @note 实时循环显示所有舵机的监控数据，按Ctrl+C退出
 *       - 显示内容：ID、角度、电压、电流、温度、状态
 *       - 采用动态刷新方式，数据从消息队列获取
 */
static void Servo_View_Shell(void) {
    ServoData_t data[SERVO_COUNT];
    extern Shell shell;
    char ch;

    logPrintln("Servo Monitor Viewer - Press ^C to exit");
    logPrintln("ID  Angle(°)  Volt(mV)  Curr(mA)  Temp(°C)  Status");

    for (;;) {
        // 从队列获取最新数据（非阻塞）
        if (osMessageQueueGet(Servo_DataHandle, data, NULL, 50) == osOK) {
            logPrintln("\033[1A\033[2K\r");  // 清除上一行
            for (int i = 0; i < SERVO_COUNT; i++) {
                logPrintln("%2d  %7.1f  %8d  %8d  %7.1f  0x%02X",
                    data[i].id, data[i].angle, data[i].voltage,
                    data[i].current, data[i].temperature, data[i].status);
            }
            logPrintln("\033[%dA", SERVO_COUNT); // 光标上移
        }
        // 检查退出键
        if (shell.read(&ch, 1) == 1) {
            if (ch == 0x03) break;  // ^C
        }
        osDelay(20);
    }
    logPrintln("\033[J");  // 清除下方显示
}

/**
 * @brief 舵机连通性测试命令(Shell接口)
 * @param argc 参数个数
 * @param argv 参数数组，argv[1]为舵机ID
 * @note 向指定舵机发送Ping命令，检测舵机是否在线响应
 */
static void Servo_Ping_Shell(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: ping <id>");
        return;
    }
    uint8_t id = (uint8_t)atoi(argv[1]);
    FSUS_STATUS ret = FSUS_Ping(&servoUsart, id);
    if (ret == FSUS_STATUS_SUCCESS) {
        logPrintln("Servo %d responded", id);
    } else {
        logPrintln("Servo %d no response (error %d)", id, ret);
    }
}

ShellCommand ServoGroup[] = {
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN, angle, Servo_Angle_Shell, set servo angle),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN, mturn, Servo_Mturn_Shell, set multi-turn angle),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN, damping, Servo_Damping_Shell, enter damping mode),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN, stop, Servo_Stop_Shell, stop servo),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN, origin, Servo_ResetOrigin_Shell, set origin point),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN, reset_angle, Servo_ResetAngle_Shell, reset multi-turn angle),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC, view, Servo_View_Shell, view real-time servo data),
    SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN, ping, Servo_Ping_Shell, ping servo),
    SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN),
servo, ServoGroup, servo control commands);

/**
 * @brief 初始化舵机串口及环形缓冲区
 * @note 初始化发送和接收环形缓冲区，并使能DMA接收
 *       - 发送缓冲区：256字节
 *       - 接收缓冲区：1024字节
 */
static void Servo_Usart_Init(void) {
    // 初始化串口环形缓冲区
    static uint8_t txBuf[256];
    static uint8_t rxBuf[1024];
    RingBuffer_Init(servoUsart.sendBuf, sizeof(txBuf), txBuf);
    RingBuffer_Init(servoUsart.recvBuf, sizeof(rxBuf), rxBuf);
    servoUsart.huartX = &ServoUsart;
    
    // 使能串口接收
    HAL_UART_Receive_DMA(&ServoUsart, rxBuf, sizeof(rxBuf));
}

/**
 * @brief 执行舵机控制命令
 * @param cmd 舵机命令结构体指针，包含命令类型、舵机ID、角度、速度等参数
 * @note 支持多种命令类型：角度控制、多圈角度控制、速度控制、阻尼模式、停止、复位等
 */
static void Servo_ExecuteCommand(const ServoCmd_t *cmd) {
    switch (cmd->cmdType) {
        case SERVO_CMD_SET_ANGLE:
            FSUS_SetServoAngle(&servoUsart, cmd->servoId, cmd->angle, 
                               cmd->interval, cmd->power);
            break;
        case SERVO_CMD_SET_ANGLE_MTURN:
            FSUS_SetServoAngleMTurn(&servoUsart, cmd->servoId, cmd->angle,
                                    cmd->interval, cmd->power);
            break;
        case SERVO_CMD_SET_ANGLE_BY_VELOCITY:
            FSUS_SetServoAngleByVelocity(&servoUsart, cmd->servoId, cmd->angle,
                                         cmd->velocity, cmd->t_acc, cmd->t_dec, cmd->power);
            break;
        case SERVO_CMD_DAMPING:
            FSUS_DampingMode(&servoUsart, cmd->servoId, cmd->power);
            break;
        case SERVO_CMD_STOP:
            FSUS_StopOnControlMode(&servoUsart, cmd->servoId, 0, 0);
            break;
        case SERVO_CMD_RESET_ORIGIN:
            FSUS_SetOriginPoint(&servoUsart, cmd->servoId);
            break;
        case SERVO_CMD_RESET_ANGLE:
            FSUS_ServoAngleReset(&servoUsart, cmd->servoId);
            break;
        default:
            break;
    }
}

/**
 * @brief 舵机模块读取所有舵机监控数据
 * @param dataArray 舵机数据输出数组，长度应为 SERVO_COUNT
 */
static void Servo_ReadAllMonitorData(ServoData_t *dataArray) {
    // 使用驱动中已有的全局 servodata 数组（位于 fashion_star_uart_servo.c）
    extern ServoData servodata[];
    
    // 同步读取所有舵机数据 (需要先发送同步命令)
    FSUS_sync_servo syncServos[SERVO_COUNT];
    for (int i = 0; i < SERVO_COUNT; i++) {
        syncServos[i].id = ServoIDList[i];
    }
    FSUS_SyncCommand(&servoUsart, SERVO_COUNT, MODE_Query_SERVO_Monitor, syncServos);
    
    // 等待接收完成（驱动内部会调用 FSUS_SyncServoMonitor）
    // 数据会被填充到全局 servodata 中
    
    // 复制到输出数组
    for (int i = 0; i < SERVO_COUNT; i++) {
        dataArray[i].id = servodata[i].id;
        dataArray[i].angle = servodata[i].angle;
        dataArray[i].voltage = servodata[i].voltage;
        dataArray[i].current = servodata[i].current;
        dataArray[i].power = servodata[i].power;
        dataArray[i].temperature = servodata[i].temperature;
        dataArray[i].status = servodata[i].status;
        dataArray[i].circleCount = servodata[i].circle_count;
    }
}

/**
 * @brief 舵机单圈角度控制接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @param angle 目标角度，范围-180°-180°（单圈绝对位置）
 * @param interval_ms 运动时间，单位ms（0-65535）
 * @param power_mW 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令，阻塞等待直到消息成功放入队列
 */
bool Servo_SetAngle(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_SET_ANGLE,
        .servoId = id,
        .angle = angle,
        .interval = interval_ms,
        .power = power_mW
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机多圈角度控制接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @param angle 目标角度，范围-368640°-368640°（多圈绝对位置，约±1024圈）
 * @param interval_ms 运动时间，单位ms（0-65535）
 * @param power_mW 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令，阻塞等待直到消息成功放入队列
 */
bool Servo_SetAngleMultiTurn(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_SET_ANGLE_MTURN,
        .servoId = id,
        .angle = angle,
        .interval = interval_ms,
        .power = power_mW
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机速度模式角度控制接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @param angle 目标角度，范围-368640°-368640°（多圈绝对位置，约±1024圈）
 * @param velocity_rpm 目标转速，单位rpm
 * @param t_acc_ms 加速时间，单位ms（0-65535）
 * @param t_dec_ms 减速时间，单位ms（0-65535）
 * @param power_mW 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令，阻塞等待直到消息成功放入队列
 */
bool Servo_SetAngleByVelocity(uint8_t id, float angle, int16_t velocity_rpm, 
                               uint16_t t_acc_ms, uint16_t t_dec_ms, uint16_t power_mW) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_SET_ANGLE_BY_VELOCITY,
        .servoId = id,
        .angle = angle,
        .velocity = velocity_rpm,
        .t_acc = t_acc_ms,
        .t_dec = t_dec_ms,
        .power = power_mW
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机阻尼模式控制接口 （非阻塞式API）
 * @param id 舵机ID (1-254)
 * @param power_mW 阻尼功率，单位mW（0-1000）
 * @return 命令发送结果，true=发送成功，false=发送失败
 * @note 通过消息队列发送命令，阻塞等待直到消息成功放入队列
 */
bool Servo_SetDampingMode(uint8_t id, uint16_t power_mW) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_DAMPING,
        .servoId = id,
        .power = power_mW
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机紧急停止接口（立即停止运动） （非阻塞式API）
 * @param id 舵机ID (1-254)
 * @return 命令发送结果，true=发送成功，false=发送失败
 * @note 通过消息队列发送命令，阻塞等待直到消息成功放入队列
 */
bool Servo_Stop(uint8_t id) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_STOP,
        .servoId = id
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机设置原点接口（校准当前位置为零点）
 * @param id 舵机ID (1-254)
 * @return 命令发送结果，true=发送成功，false=发送失败
 * @note 通过消息队列发送命令，阻塞等待直到消息成功放入队列
 */
bool Servo_SetOrigin(uint8_t id) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_RESET_ORIGIN,
        .servoId = id
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机多圈角度重置接口（重置多圈角度计数）
 * @param id 舵机ID (1-254)
 * @return 命令发送结果，true=发送成功，false=发送失败
 * @note 通过消息队列发送命令，阻塞等待直到消息成功放入队列
 */
bool Servo_ResetMultiTurnAngle(uint8_t id) {
    ServoCmd_t cmd = {
        .cmdType = SERVO_CMD_RESET_ANGLE,
        .servoId = id
    };
    return osMessageQueuePut(Servo_CmdHandle, &cmd, 0, 0) == osOK;
}

/**
 * @brief 舵机运行状态监测数据获取接口
 * @param id 舵机ID (1-254)
 * @param outData 输出参数，用于存储获取到的舵机监测数据
 * @return 数据获取结果，true=获取成功，false=获取失败/无对应ID数据
 * @note 从数据消息队列读取舵机状态数据，超时时间100ms，匹配ID后输出数据
 */
bool Servo_GetMonitorData(uint8_t id, ServoData_t *outData) {
    ServoData_t data[SERVO_COUNT];
    if (osMessageQueueGet(Servo_DataHandle, data, NULL, 100) == osOK) {
        for (int i = 0; i < SERVO_COUNT; i++) {
            if (data[i].id == id) {
                memcpy(outData, &data[i], sizeof(ServoData_t));
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief 所有舵机运行状态监测数据批量获取接口
 * @param outArray 输出数组，用于存储所有舵机的监测数据
 * @param maxCount 输出数组的最大容量，需不小于SERVO_COUNT
 * @return 数据获取结果，true=获取成功，false=数组容量不足或队列读取失败
 * @note 从数据消息队列读取所有舵机状态数据，超时时间100ms，需确保数组容量足够
 */
bool Servo_GetAllMonitorData(ServoData_t *outArray, uint8_t maxCount) {
    if (maxCount < SERVO_COUNT) return false;
    return (osMessageQueueGet(Servo_DataHandle, outArray, NULL, 100) == osOK);
}

/**
 * @brief 多舵机同步角度控制接口（批量设置单圈角度）
 * @param ids 舵机ID数组指针
 * @param angles 对应舵机的目标角度数组指针
 * @param count 待控制的舵机数量
 * @param interval_ms 运动时间，单位ms（0-65535）
 * @param power_mW 输出功率，单位mW（0-1000）
 * @note 循环调用单舵机角度设置接口，批量下发控制命令
 */
void Servo_SetMultiAngles(const uint8_t *ids, const float *angles, 
                           uint8_t count, uint16_t interval_ms, uint16_t power_mW) {
    for (uint8_t i = 0; i < count; i++) {
        Servo_SetAngle(ids[i], angles[i], interval_ms, power_mW);
    }
}

/**
 * @brief 所有舵机紧急停止接口（批量停止）
 * @note 遍历所有舵机ID列表，循环调用单舵机停止接口，实现全部舵机立即停止
 */
void Servo_StopAll(void) {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        Servo_Stop(ServoIDList[i]);
    }
}

/**
 * @brief 舵机在线状态检测接口
 * @param id 舵机ID (1-254)
 * @return 在线状态，true=舵机在线响应，false=舵机离线无响应
 * @note 通过发送Ping指令检测舵机通信状态，返回指令执行结果
 */
bool Servo_IsAlive(uint8_t id) {
    return (FSUS_Ping(&servoUsart, id) == FSUS_STATUS_SUCCESS);
}

/**
 * @brief 等待舵机停止运动接口（阻塞式，角度稳定判定）
 * @param id 舵机ID (1-254)
 * @param timeout_ms 最大等待超时时间，单位ms
 * @note 循环读取舵机角度数据，角度变化小于0.1°判定为已停止，每20ms采样一次
 * @note 达到超时时间后自动退出等待
 */
void Servo_WaitForStop(uint8_t id, uint32_t timeout_ms) {
    float lastAngle = 0, currentAngle = 0;
    uint32_t startTick = xTaskGetTickCount();
    bool firstSample = true;
    
    while ((xTaskGetTickCount() - startTick) < pdMS_TO_TICKS(timeout_ms)) {
        ServoData_t data;
        if (Servo_GetMonitorData(id, &data)) {
            currentAngle = data.angle;
            if (!firstSample && (fabs(currentAngle - lastAngle) < 0.1f)) {
                break;  // 角度稳定，运动完成
            }
            lastAngle = currentAngle;
            firstSample = false;
        }
        osDelay(20);
    }
}

/**
 * @brief 舵机平滑分段运动控制接口（分段插值实现平滑移动）
 * @param id 舵机ID (1-254)
 * @param targetAngle 目标角度，范围-368640°-368640°
 * @param totalTimeMs 总运动时间，单位ms
 * @param segments 分段运动步数（分段越多运动越平滑）
 * @param power_mW 输出功率，单位mW（0-1000）
 * @note 读取当前角度作为起点，均分角度与时间，分段下发角度指令实现平滑运动
 * @note 若获取舵机当前状态失败，直接退出函数
 */
void Servo_SmoothMove(uint8_t id, float targetAngle, uint16_t totalTimeMs, 
                       uint8_t segments, uint16_t power_mW) {
    ServoData_t data;
    if (!Servo_GetMonitorData(id, &data)) return;
    
    float startAngle = data.angle;
    float delta = targetAngle - startAngle;
    uint16_t segmentTime = totalTimeMs / segments;
    
    for (uint8_t i = 1; i <= segments; i++) {
        float intermediateAngle = startAngle + delta * i / segments;
        Servo_SetAngle(id, intermediateAngle, segmentTime, power_mW);
        osDelay(segmentTime);
    }
}

/**
 * @brief 舵机模块控制任务
 * @param argument 任务参数
 */
void Servo_Ctrl_Task(void *argument) {
    (void)argument;
    ServoCmd_t cmd;
    
    // 等待系统初始化完成
    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    
    for (;;) {
        // 阻塞等待命令
        if (osMessageQueueGet(Servo_CmdHandle, &cmd, NULL, osWaitForever) == osOK) {
            Servo_ExecuteCommand(&cmd);
        }
    }
}

/**
 * @brief 舵机模块监控任务
 * @param argument 任务参数
 */
void Servo_Mon_Task(void *argument) {
    (void)argument;
    ServoData_t monitorData[SERVO_COUNT];
    
    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);
    
    for (;;) {
        Servo_ReadAllMonitorData(monitorData);
        
        // 将数据发送到队列（覆盖旧数据）
        osMessageQueueReset(Servo_DataHandle);
        osMessageQueuePut(Servo_DataHandle, monitorData, 0, 0);
        
        osDelay(SERVO_MONITOR_PERIOD);
    }
}

/**
 * @brief 舵机模块初始化函数
 */
void Servo_Init(void) {
    // 初始化串口
    Servo_Usart_Init();
    
    logPrintln("Servo port initialized");
}

