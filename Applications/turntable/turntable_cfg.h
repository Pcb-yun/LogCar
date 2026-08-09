#ifndef __TURNTABLE_CFG_H__
#define __TURNTABLE_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 转盘传感器类型
 */
#define TCS230 0
#define PRI 1

#define TURNTABLE_SENSOR TCS230  // 转盘传感器类型

#define TURNTABLE_SERVO_ID 1    // 转盘模块舵机ID
#define TURNTABLE_INIT -10.0f   // 转盘模块初始角度
#define TURNTABLE_ID_0_ANGLE 26.0f  // 转盘模块ID0角度
#define TURNTABLE_ANGLE 36.0f  // 转盘模块角度间隔
#define TURNTABLE_CLOSE_OFFSET 18.0f  // 转盘模块关闭角度偏移量
#define TURNTABLE_ID_MAX 5  // 转盘模块ID最大数量
#define TURNTABLE_DIST_GOODS_THRESHOLD_MM   30  // 物料检测距离阈值(mm)
#define TURNTABLE_DIST_AWARD_THRESHOLD_MM   20  // 奖杯检测距离阈值(mm)
#define TURNTABLE_POLL_INTERVAL_MS    20  // 物料检测轮询周期(ms)
#define TURNTABLE_SETTLE_MS           500  // 转盘运动到位后的稳定等待时间(ms)
#define TURNTABLE_COLOR_READ_TIMEOUT_MS 1000  // 颜色读取超时时间(ms)

#define TURNTABLE_ITEM_MAX            TURNTABLE_ID_MAX

/* ==================== 出栈配置 ==================== */

#define TURNTABLE_PAUSE_TIMEOUT_MS    3000  // 出栈占用转盘时, 等待入栈当前动作结束的超时时间(ms)
#define TURNTABLE_POP_SETTLE_MS       300  // 出栈: 旋转到出料口后的稳定等待时间(ms)
#define TURNTABLE_POP_POLL_MS         50  // 出栈: 检测物料是否离开转盘的轮询间隔(ms)
#define TURNTABLE_POP_TIMEOUT_MS      3000  // 出栈: 等待物料到达/离开出料口的超时时间(ms)

#ifdef __cplusplus
}
#endif

#endif  /* __TURNTABLE_CFG_H__ */
