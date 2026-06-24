/**
 * @file nav_config.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航配置头文件
 */

#ifndef __NAV_CONFIG_H__
#define __NAV_CONFIG_H__


// 航向角PID控制器参数
#define NAV_YAW_PID_KP            3.0f      // yaw比例系数
#define NAV_YAW_PID_KI            1.5f      // 积分系数
#define NAV_YAW_PID_KD            0.08f      // 微分系数
#define NAV_YAW_PID_OUTPUT_LIMIT  180.0f    // 限制yaw输出

#define NAV_APPROACH_YAW_KP       0.4f      // 接近阶段偏航P增益
#define NAV_APPROACH_YAW_MAX      80.0f     // 接近阶段最大偏航速度

#define NAV_ALIGN_YAW_MAX         180.0f    // 对齐阶段最大偏航速度
#define NAV_ALIGN_YAW_MIN         4.0f     // 对齐阶段最小偏航速度

#define NAV_ALIGN_DIST            12.0f      // 进入对齐阶段距离
#define NAV_ALIGN_XY_KP           0.8f      // 位置P增益
#define NAV_ALIGN_XY_DEADBAND     0.3f      // 位置死区

#define NAV_MIN_SPEED             4.0f      // 最低速度

#define NAV_TIME_DECAY_FACTOR     5.0f      // 时间衰减因子
#define NAV_MAX_SENSOR            4         // 最大传感器数量



#endif /* __NAV_CONFIG_H__ */
