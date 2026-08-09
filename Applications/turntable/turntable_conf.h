#ifndef __TURNTABLE_CONF_H__
#define __TURNTABLE_CONF_H__

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
 * @brief 转盘模块初始角度
 */
#define TURNABLE_INIT -10.0f

/**
 * @brief 转盘模块ID0角度
 */
#define TURNABLE_ID_0_ANGLE 26.0f

/**
 * @brief 转盘模块角度间隔
 */
#define TURNABLE_ANGLE 36.0f

/**
 * @brief 转盘模块关闭角度偏移量
 */
#define TURNABLE_CLOSE_OFFSET 18.0f

/**
 * @brief 转盘模块ID最大数量
 */
#define TURNABLE_ID_MAX 5

/**
 * @brief 物料检测距离阈值(mm)
 * @note VL53L0X 测得距离低于该值认为进料口处有物料
 */
#define TURNTABLE_DIST_GOODS_THRESHOLD_MM   100

/**
 * @brief 奖杯检测距离阈值(mm)
 * @note VL53L0X 测得距离低于该值认为进料口处有奖杯(不测颜色)
 * @note 该值必须小于 TURNTABLE_DIST_GOODS_THRESHOLD_MM
 */
#define TURNTABLE_DIST_AWARD_THRESHOLD_MM   20

/**
 * @brief 物料检测轮询周期(ms)
 */
#define TURNTABLE_POLL_INTERVAL_MS    20

/**
 * @brief 转盘运动到位后的稳定等待时间(ms)
 */
#define TURNTABLE_SETTLE_MS           500

/**
 * @brief 自动分拣最大存储物料数量
 */
#define TURNTABLE_ITEM_MAX            TURNABLE_ID_MAX

/* ==================== 出栈配置 ==================== */

/**
 * @brief 出栈占用转盘时, 等待入栈当前动作结束的超时时间(ms)
 * @note 入栈一个物料约需 2×TURNTABLE_SETTLE_MS + 测色时间
 */
#define TURNTABLE_PAUSE_TIMEOUT_MS    3000

/**
 * @brief 出栈: 旋转到出料口后的稳定等待时间(ms)
 */
#define TURNTABLE_POP_SETTLE_MS       300

/**
 * @brief 出栈: 检测物料是否离开转盘的轮询间隔(ms)
 */
#define TURNTABLE_POP_POLL_MS         50

/**
 * @brief 出栈: 等待物料到达/离开出料口的超时时间(ms)
 */
#define TURNTABLE_POP_TIMEOUT_MS      3000

extern float turntable_id[TURNABLE_ID_MAX];

#ifdef __cplusplus
}
#endif

#endif  /* __TURNTABLE_CONF_H__ */
