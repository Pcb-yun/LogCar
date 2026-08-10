/**
 * @file turntable_cfg.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 转盘模块配置头文件
 */

#ifndef __TURNTABLE_CFG_H__
#define __TURNTABLE_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define TURNTABLE_SERVO_ID 1                // 转盘模块舵机ID
#define TURNTABLE_ID_0_ANGLE 24.0f          // 转盘模块ID 0 对应的角度
#define TURNTABLE_ANGLE 37.0f               // 转盘模块入库间隔角度
#define TURNTABLE_CLOSE_OFFSET 18.0f        // 转盘模块关闭角度偏移
#define TURNTABLE_ID_MAX 5                  // 转盘模块最大ID
#define TURNTABLE_POLL_INTERVAL_MS 20       // 转盘模块轮询间隔(ms)
#define TURNTABLE_SETTLE_MS 500             // 转盘模块运动到位后的稳定等待时间(ms)


#ifdef __cplusplus
}
#endif

#endif  /* __TURNTABLE_CFG_H__ */
