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

#define TURNTABLE_SENSOR TCS230

/**
 * @brief 转盘模块舵机设置
 */
#define TURNTABLE_SERVO_ID 1                                /** 转盘模块舵机ID */
#define TURNTABLE_INIT -10.0f                               /** 转盘模块初始角度 */
#define TURNTABLE_ID_0_ANGLE 26.0f                          /** 转盘模块ID 0 对应的角度 */
#define TURNTABLE_ANGLE 36.0f                               /** 转盘模块角度角度 */
#define TURNTABLE_CLOSE_OFFSET 18.0f                        /** 转盘模块关闭角度偏移 */
#define TURNTABLE_ID_MAX 5                                  /** 转盘模块最大ID */
#define TURNTABLE_DIST_GOODS_THRESHOLD_MM   30              /** 转盘模块距离物料阈值(mm) */
#define TURNTABLE_POLL_INTERVAL_MS    20                    /** 转盘模块轮询间隔(ms) */
#define TURNTABLE_SETTLE_MS           500                   /** 转盘模块运动到位后的稳定等待时间(ms) */
#define TURNTABLE_COLOR_READ_TIMEOUT_MS 1000                /** 颜色读取超时时间(ms) */
#define TURNTABLE_ITEM_MAX            TURNTABLE_ID_MAX      /** 自动分拣最大存储物料数量 */

#ifdef __cplusplus
}
#endif

#endif  /* __TURNTABLE_CFG_H__ */
