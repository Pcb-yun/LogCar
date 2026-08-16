/**
 * @file nav_config.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航配置头文件
 */

#ifndef __NAV_CONFIG_H__
#define __NAV_CONFIG_H__

#define NAV_MAP_TOOL                 0       // 是否启用地图shell工具

#define NAV_UPDATE_TIME              5       // 导航状态更新时间间隔（毫秒）
#define NAV_TIME_DECAY_FACTOR        5.0f    // 时间衰减因子
#define NAV_MAX_SENSOR               2       // 最大传感器数量

#define NAV_STARTUP_SPEED            1.5f    // 车辆启动速度（cm/s），克服静摩擦的最小速度，当规划速度低于此值时强制使用此值

#define NAV_MIN_SPEED                (NAV_STARTUP_SPEED * 0.6f)   // 最低速度（cm/s），用于到达检测

#define NAV_YAW_PID_KP               1.8f    // yaw比例系数
#define NAV_YAW_PID_KI               0.05f   // yaw积分系数
#define NAV_YAW_PID_KD               0.3f    // yaw微分系数
#define NAV_YAW_PID_OUTPUT_LIMIT     150.0f  // yaw PID输出限制

#define NAV_ALIGN_YAW_MAX            100.0f  // 对齐阶段最大偏航速度
#define NAV_ALIGN_YAW_MIN            2.0f    // 对齐阶段最小偏航速度（deg/s），旋转启动值
#define NAV_YAW_ACCEL_LIMIT          350.0f  // yaw加速度限制
#define NAV_YAW_ZERO_CROSS_LOCK_MS   2       // yaw零交叉锁存持续时间(ms)

#define NAV_ALIGN_DIST               5.0f   // 进入对齐阶段距离（cm）
#define NAV_ALIGN_XY_KP              1.2f    // 位置P增益（提高加快对齐速度）
#define NAV_ALIGN_XY_KI              0.02f   // 位置I增益（降低减少积分饱和）
#define NAV_ALIGN_XY_DEADBAND        0.2f    // 位置死区（cm）

#define NAV_NEAR_TARGET_FACTOR       12.0f   // 到达锁定阈值倍数（相对于distance_threshold）
#define NAV_NEAR_TARGET_MIN_SPEED    NAV_STARTUP_SPEED  // 接近目标时的最小微调速度（cm/s），基于启动速度
#define NAV_ARRIVE_CHECK_COUNT       2       // 连续到达检查次数（防止惯性/打滑误判）
#define NAV_ALIGN_XY_HYSTERESIS      0.2f    // 位置微调滞回（cm，防止小误差来回震荡）

#define NAV_TRAJ_SPEED_CAP           60.0f   // 梯形规划近目标速度上限（cm/s），提高以减少速度突降
#define NAV_NEAR_TARGET_GAIN         3.1f    // 近目标位置反馈增益系数，提高以增强小误差响应
#define NAV_NEAR_TARGET_SPEED_LIMIT  4.5f    // 近目标速度限幅倍数，提高允许更大调整速度

#define NAV_YAW_DEADBAND             1.0f    // yaw死区（deg）
#define NAV_YAW_FEEDFORWARD_THRESH   5.0f    // yaw前馈触发阈值（deg）
#define NAV_YAW_FEEDFORWARD_WEIGHT   0.7f    // yaw前馈权重
#define NAV_YAW_FEEDBACK_WEIGHT      0.3f    // yaw反馈权重

#define NAV_NEAR_TARGET_DEADBAND_RATIO  0.25f  // 近目标死区比例（相对于distance_threshold）
#define NAV_NEAR_TARGET_STOP_RATIO      0.3f  // 近目标停止阈值比例

#define NAV_NEAR_TARGET_DIST_FACTOR_MIN 0.15f // 近目标距离因子下限
#define NAV_POSITION_CORRECTION_SPEED_THRESH 0.5f // 位置反馈速度阈值（cm/s）
#define NAV_LOW_SPEED_COMPENSATION      (NAV_STARTUP_SPEED * 0.2f)  // 低速起步补偿（cm/s），基于启动速度的20%

#define NAV_NEAR_TRANSITION_FRAMES      3       // 远→近目标模式过渡帧数
#define NAV_NEAR_ACCEL_MULTIPLIER       1.2f    // 近目标加减速倍率




#endif /* __NAV_CONFIG_H__ */
