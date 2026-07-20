/**
 * @file servo_port.c
 * @author MIKE
 * @brief 总线舵机驱动 FreeRTOS 适配层源文件
 */

#include "servo_port.h"
#include "usart.h"
#include "log.h"
#include "stream_buffer.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "freeRTOS.h"
#include "task.h"
#include "Events.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief 舵机单圈角度控制接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @param angle 目标角度，范围-180°-180°（单圈绝对位置）
 * @param interval_ms 运动时间，单位ms（0-65535）
 * @param power_mW 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令
 */
void Servo_ANGLE(uint8_t id, float angle, uint16_t interval_ms, uint16_t power_mW) {
    if (id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
    }
    Servo_SetServoAngle(id,angle,interval_ms,power_mW);

}

/**
 * @brief 舵机轮式运动控制接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @param angle 目标角度，范围-180°-180°（单圈绝对位置）
 * @param interval 运动时间，单位ms（0-65535）
 * @param power 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令
 */
void Servo_MTURN(uint8_t id, float angle,uint32_t interval, uint16_t power){
    if (id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
    }
    Servo_SetServoAngleMTurn(id,angle,interval,power);

}

/**
 * @param id 舵机ID (1-254)
 * @note 通过消息队列发送命令
 */
void Servo_STOP(uint8_t id) {
    if (id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
    }
    Servo_StopOnControlMode(id,1,1);
}

/**
 * @brief 所有舵机紧急停止接口（批量停止）
 * @note 遍历所有舵机ID，循环调用单舵机停止接口，实现全部舵机立即停止
 */
void Servo_STOP_ALL(void) {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) {
        Servo_STOP(i + 1);
    }
}

#if SERVO_PING
/**
 * @brief 舵机Ping接口（非阻塞式API）
 * @param id 舵机ID (1-254)
 * @note 通过消息队列发送命令
 */
void Servo_PING(uint8_t id) {
    if (id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
    }
    int status = Servo_Ping(id);
    switch (status)
    {
    case SERVO_STATUS_SUCCESS:
        logPrintln("Servo %d is ONLINE", id);
        break;
    case SERVO_STATUS_TIMEOUT:
        logPrintln("Servo %d is TIMEOUT", id);
        break;
    default:
        logPrintln("Unknown status: %d", status);
        break;
    }
}
#endif

#if SERVO_ADVANCED_MODE

/**
 * @brief 舵机指定时间角度控制接口（非阻塞式API）
 * @param servo_id 舵机ID (1-254)
 * @param angle 目标角度，范围-180°-180°（单圈绝对位置）
 * @param interval 运动时间，单位ms（0-65535）
 * @param t_acc 加速时间，单位ms
 * @param t_dec 减速时间，单位ms
 * @param power 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令
 */
void Servo_AngleByInterval(uint8_t servo_id, float angle, 
                                            uint16_t interval, uint16_t t_acc,
                                            uint16_t t_dec, uint16_t power) {
    if (servo_id == 0 || servo_id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", servo_id);
    }
    if (angle < -180.0f || angle > 180.0f) {
        logPrintln("Invalid angle: %.2f (must be -180~180)", angle);
    }
    Servo_SetServoAngleByInterval(servo_id, angle, interval, t_acc, t_dec, power);

}

/**
 * @brief 舵机指定速度角度控制接口（非阻塞式API）
 * @param servo_id 舵机ID (1-254)
 * @param angle 目标角度，范围-180°-180°（单圈绝对位置）
 * @param velocity 运动速度，单位°/s
 * @param t_acc 加速时间，单位ms
 * @param t_dec 减速时间，单位ms
 * @param power 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令
 */
void Servo_AngleByVelocity(uint8_t servo_id, float angle, 
                                            float velocity, uint16_t t_acc,
                                            uint16_t t_dec, uint16_t power) {
    if (servo_id == 0 || servo_id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", servo_id);
    }
    if (angle < -180.0f || angle > 180.0f) {
        logPrintln("Invalid angle: %.2f (must be -180~180)", angle);
    }
    if (velocity <= 0.0f) {
        logPrintln("Invalid velocity: %.2f (must be > 0)", velocity);
    }
    Servo_SetServoAngleByVelocity(servo_id, angle, velocity, t_acc, t_dec, power);

}

/**
 * @brief 舵机多圈指定速度角度控制接口（非阻塞式API）
 * @param servo_id 舵机ID (1-254)
 * @param angle 目标角度，范围-36000°-36000°（多圈绝对位置）
 * @param velocity 运动速度，单位°/s
 * @param t_acc 加速时间，单位ms
 * @param t_dec 减速时间，单位ms
 * @param power 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令
 */
void Servo_MTurnByVelocity(uint8_t servo_id, float angle,
                                                 float velocity, uint16_t t_acc,
                                                 uint16_t t_dec, uint16_t power) {
    if (servo_id == 0 || servo_id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", servo_id);
    }
    if (angle < -36000.0f || angle > 36000.0f) {
        logPrintln("Invalid multi-turn angle: %.2f (must be -36000~36000)", angle);
    }
    if (velocity <= 0.0f) {
        logPrintln("Invalid velocity: %.2f (must be > 0)", velocity);
    }
    Servo_SetServoAngleMTurnByInterval(servo_id, angle, velocity, t_acc, t_dec, power);
}

/**
 * @brief 舵机多圈指定时间角度控制接口（非阻塞式API）
 * @param servo_id 舵机ID (1-254)
 * @param angle 目标角度，范围-36000°-36000°（多圈绝对位置）
 * @param interval 运动时间，单位ms（0-4294967295）
 * @param t_acc 加速时间，单位ms
 * @param t_dec 减速时间，单位ms
 * @param power 输出功率，单位mW（0-1000）
 * @note 通过消息队列发送命令
 */
void Servo_MTurnByInterval(uint8_t servo_id, float angle,
                                                 uint32_t interval, uint16_t t_acc,
                                                 uint16_t t_dec, uint16_t power) {
    if (servo_id == 0 || servo_id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", servo_id);
    }
    if (angle < -36000.0f || angle > 36000.0f) {
        logPrintln("Invalid multi-turn angle: %.2f (must be -36000~36000)", angle);
    }
    if (interval == 0) {
        logPrintln("Invalid interval: %u (must be > 0)", interval);
    }
    Servo_SetServoAngleMTurnByVelocity(servo_id, angle, interval, t_acc, t_dec, power);
}

#endif

#if SERVO_MONITOR
void Servo_MONITOR(uint8_t servo_id, ServoData servodata[]){
    Servo_Monitor(servo_id, servodata);
}

#endif

/**
 * @brief 舵机模块Shell命令
 * @param argc 参数个数，必须为2
 * @param argv 参数数组，argv[1]为命令
 */
static void Servo_Shell(int argc, char *argv[]){
    if(argc < 2) {
        logPrintln(SERVO_CMD_HELP);
        return;
    }

    uint8_t id = (uint8_t)atoi(argv[2]);
    float angle = (float)atof(argv[3]);
    float velocity = (float)atof(argv[4]);
    float interval = (float)atof(argv[4]);
    float t_acc = (float)atof(argv[5]);
    float t_dec = (float)atof(argv[6]);

    if(id == 0 || id > 254) {
        logPrintln("Invalid servo ID: %d (must be 1-254)", id);
        return;
    }

    if(strcmp(argv[1], "angle") == 0) {
        Servo_ANGLE(id, angle, 0, 1000);
    } else if(strcmp(argv[1], "mturn") == 0) {
        Servo_MTURN(id, angle, 0, 1000);
    } else if(strcmp(argv[1], "stop") == 0) {
        Servo_STOP(id);
    } else if(strcmp(argv[1], "stopall") == 0) {
        Servo_STOP_ALL();
    }

    #if SERVO_PING
    else if(strcmp(argv[1], "ping") == 0) {
        Servo_PING(id);
    }
    #endif

    #if SERVO_ADVANCED_MODE
    else if(strcmp(argv[1], "angle_vel") == 0) {
        Servo_AngleByVelocity(id, angle, velocity, t_acc, t_dec, 1000);
    }
    else if(strcmp(argv[1], "angle_int") == 0) {
        Servo_AngleByInterval(id, angle, interval, t_acc, t_dec, 1000);
    }
    else if(strcmp(argv[1], "mturn_vel") == 0) {
        Servo_MTurnByVelocity(id, angle, velocity, t_acc, t_dec, 1000);
    }
    else if(strcmp(argv[1], "mturn_int") == 0) {
        Servo_MTurnByInterval(id, angle, interval, t_acc, t_dec, 1000);

    }
    #endif

    #if SERVO_MONITOR
    else if(strcmp(argv[1], "monitor") == 0) {
        static ServoData servodata = {0};
        Servo_MONITOR(id, &servodata);
        logPrintln("Angle: %.2f, Power: %d, Temperature: %d, Status: %d, Circle_Count: %d",
             servodata.angle, servodata.power, 
             servodata.temperature, servodata.status, 
             servodata.circle_count);
    }
    #endif

    else {
        logPrintln("Invalid command: %s", argv[1]);
        logPrintln(SERVO_CMD_HELP);
    }
}

/**
 * @brief 导出舵机模块Shell命令
 */
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
servo, Servo_Shell, servo control commands);
