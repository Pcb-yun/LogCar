/**
 * @brief 舵机配置
 * @author MIKE
 */

 #ifndef __SERVO_CFG_H__
 #define __SERVO_CFG_H__
 
#define SERVO_ADVANCED_MODE 1   // 高级模式
#define SERVO_DLC 0             // 舵机其他功能
#define SERVO_ASYNC 0           // 异步命令
#define SERVO_SYNC 0            // 同步命令
#define SERVO_MONITOR 1        // 监控命令
#define SERVO_SYNC_MONITOR 0    // 同步监控命令
#define SERVO_PING 1            // Ping命令

#define SERVO_TIMEOUT_MS 100    // 串口通讯超时设置
#define SERVO_MAX_COUNT 2       // 最大舵机数量

#endif
