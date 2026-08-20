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
#define TURNTABLE_ID_0_ANGLE 2.0f          // 转盘模块ID 0 对应的角度
#define TURNTABLE_ANGLE 36.0f               // 转盘模块入库间隔角度
#define TURNTABLE_CLOSE_OFFSET 10.0f        // 转盘模块关闭角度偏移
#define TURNTABLE_ID_MAX 5                  // 转盘模块最大ID
#define TURNTABLE_POLL_INTERVAL_MS 5       // 转盘模块轮询间隔(ms)
#define TURNTABLE_SETTLE_MS 300             // 转盘模块运动到位后的稳定等待时间(ms)
#define TURNTABLE_DIST_THR 55   // 转盘距离物料阈值(mm)
#define TURNTABLE_STO_NUM TURNTABLE_ID_MAX  // 转盘入库信息数量最大值


#ifdef __cplusplus
}
#endif

#endif  /* __TURNTABLE_CFG_H__ */
