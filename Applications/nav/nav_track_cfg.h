#ifndef __NAV_TRACK_CFG_H__
#define __NAV_TRACK_CFG_H__


#define NAV_TRACK_FORWARD_SPEED 40.0f  // 前进速度 (cm/s)
#define NAV_TRACK_KP 20.0f  // P系数（35导致err=1.5时纯P输出-52.5持续猛转甩头）
#define NAV_TRACK_KI 0.0f  // I系数
#define NAV_TRACK_KD 1.0f  // D系数（探头边界跳变时D会放大抖动，用小值）
#define NAV_TRACK_MAX_YAW_SPEED 100.0f  // 最大转向速度 (deg/s)（150太高，过弯摆头）
#define NAV_TRACK_LINE_POLARITY_POSITIVE 0  // 线极性 (1:黑线输出1, 0:黑线输出0) ← 必须和模块实际一致
#define NAV_TRACK_STEERING_DIR_LEFT 1  // 转向方向 (1:线偏右时右转, -1:反向)
#define NAV_TRACK_RUN_TIME_MS 0  // 巡线运行时间 (ms)（0表示无限循环）

#define NAV_TRACK_SENSOR_NUM       8       // 灰度探头数量
#define NAV_TRACK_UPDATE_MS        10      // 巡线控制周期 (ms)
#define NAV_TRACK_LOST_TIMEOUT_MS  3000    // 连续丢线超时 (ms)，超时后自动停止
#define NAV_TRACK_SEARCH_YAW_RATIO 0.8f    // 丢线搜索转向速度比例
#define NAV_TRACK_YAW_DEADBAND     0.18f    // 转向死区（高速时尽早介入，偏差小于此值不打方向）
#define NAV_TRACK_SPEED_REDUCE_RATIO 0.8f // 转弯减速比例（转向越大减速越多）
#define NAV_TRACK_DERIV_LIMIT_RATIO 0.15f  // 微分项限幅比例（抑制误差反向时的转向猛打）
#define NAV_TRACK_ERROR_ALPHA      0.73f    // 偏差低通滤波系数（高速用小滤波，响应快）

#endif /* __NAV_TRACK_CFG_H__ */