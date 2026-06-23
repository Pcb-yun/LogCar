/**
 * @file nav_config.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航配置头文件
 */

#ifndef __NAV_CONFIG_H__
#define __NAV_CONFIG_H__

// 航向角PID控制器参数（最终对齐用）
#define NAV_YAW_PID_KP            1.1f      // 比例系数（提高响应速度）
#define NAV_YAW_PID_KI            0.05f     // 积分系数
#define NAV_YAW_PID_KD            0.3f      // 微分系数
#define NAV_YAW_PID_OUTPUT_LIMIT  70.0f     // 输出限幅 (deg/s)

// 接近阶段航向控制参数（使车头对准行进方向，消除侧移震荡）
#define NAV_APPROACH_YAW_KP       1.3f      // 接近阶段偏航P增益 (deg/s per deg)
#define NAV_APPROACH_YAW_MAX      85.0f     // 接近阶段最大偏航速度 (deg/s)

// 最终对齐阶段参数
#define NAV_ALIGN_DIST            10.0f     // 进入最终对齐的距离阈值 (cm)
#define NAV_ALIGN_XY_KP           0.5f      // 对齐阶段位置P增益
#define NAV_ALIGN_XY_DEADBAND     0.3f      // 位置死区 (cm)，低于此值强制速度为0

// 导航控制参数
#define NAV_MIN_SPEED             5.0f      // 最低启动速度 (cm/s)

#define NAV_TIME_DECAY_FACTOR     5.0f      // 时间衰减系数
#define NAV_MAX_SENSOR            4         // 最大传感器数据来源数量
#define NAV_CTRL_INTERVAL_MS      2         // 导航控制周期 (ms)



#endif /* __NAV_CONFIG_H__ */
