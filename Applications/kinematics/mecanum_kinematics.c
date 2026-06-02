/**
 * @file mecanum_kinematics.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 麦克纳姆轮运动学算法实现
 * @details 实现四麦轮小车的正逆运动学解算
 */

#include "mecanum_kinematics.h"
#include "nav_math.h"
#include <string.h>

// 全局配置参数
static MecanumConfig_t g_config = {
    .wheel_radius = 5.0f,           // 轮子半径 5cm
    .wheel_base_width = 40.0f,      // 轮距宽度 40cm
    .wheel_base_length = 30.0f,     // 轮距长度 30cm
    .encoder_resolution = 1024.0f,  // 编码器分辨率 1024脉冲/转
    .max_wheel_speed = 10.0f,       // 轮子最大角速度 10rad/s
    .max_body_speed = 50.0f,        // 车体最大速度 50cm/s
    .max_angular_speed = 1.0f       // 最大角速度 1rad/s
};

// 上次时间戳（用于计算时间差）
static uint32_t g_last_timestamp = 0;

/**
 * @brief 初始化麦克纳姆轮运动学
 */
bool Mecanum_Kinematics_Init(MecanumConfig_t *config) {
    if (config == NULL) {
        return false;
    }

    // 验证配置参数有效性
    if (config->wheel_radius <= 0.0f ||
        config->wheel_base_width <= 0.0f ||
        config->wheel_base_length <= 0.0f ||
        config->encoder_resolution <= 0.0f) {
        return false;
    }

    memcpy(&g_config, config, sizeof(MecanumConfig_t));
    g_last_timestamp = 0;

    return true;
}

/**
 * @brief 设置配置参数
 */
void Mecanum_Kinematics_SetConfig(MecanumConfig_t *config) {
    if (config != NULL) {
        memcpy(&g_config, config, sizeof(MecanumConfig_t));
    }
}

/**
 * @brief 获取当前配置
 */
MecanumConfig_t *Mecanum_Kinematics_GetConfig(void) {
    return &g_config;
}

/**
 * @brief 计算轮子到车体中心的距离
 */
float Mecanum_GetWheelDistance(void) {
    return sqrtf(g_config.wheel_base_width * g_config.wheel_base_width +
                 g_config.wheel_base_length * g_config.wheel_base_length) / 2.0f;
}

/**
 * @brief 正运动学解算
 *
 * 麦克纳姆轮正运动学公式：
 * vx = (v1 + v2 + v3 + v4) / 4
 * vy = (-v1 + v2 + v3 - v4) / 4
 * ω = (-v1 + v2 - v3 + v4) / (4 * L)
 *
 * 其中：
 * v1: 左前轮线速度 = θ1 * r
 * v2: 右前轮线速度 = θ2 * r
 * v3: 左后轮线速度 = θ3 * r
 * v4: 右后轮线速度 = θ4 * r
 * L: 轮子到中心的距离 = sqrt(w² + l²)/2
 */
bool Mecanum_ForwardKinematics(WheelEncoderData_t *encoder_delta, PoseDelta_t *pose_delta) {
    if (encoder_delta == NULL || pose_delta == NULL) {
        return false;
    }

    // 计算轮子到中心的距离
    float L = Mecanum_GetWheelDistance();

    // 脉冲转弧度（每个脉冲对应的轮子转角）
    float pulse_to_rad = M_2PI / g_config.encoder_resolution;

    // 计算每个轮子的角度增量（rad）
    float delta_theta[4] = {
        (float)encoder_delta->front_left * pulse_to_rad,
        (float)encoder_delta->front_right * pulse_to_rad,
        (float)encoder_delta->rear_left * pulse_to_rad,
        (float)encoder_delta->rear_right * pulse_to_rad
    };

    // 计算每个轮子的线位移（cm）
    float delta_s[4];
    for (int i = 0; i < 4; i++) {
        delta_s[i] = delta_theta[i] * g_config.wheel_radius;
    }

    // 正运动学解算 - 计算位移增量
    pose_delta->dx = (delta_s[0] + delta_s[1] + delta_s[2] + delta_s[3]) / 4.0f;
    pose_delta->dy = (-delta_s[0] + delta_s[1] + delta_s[2] - delta_s[3]) / 4.0f;
    pose_delta->dyaw = (-delta_s[0] + delta_s[1] - delta_s[2] + delta_s[3]) / (4.0f * L);

    // 计算时间差（s）
    float dt = 0.01f;  // 默认10ms
    if (g_last_timestamp != 0 && encoder_delta->timestamp > g_last_timestamp) {
        dt = (float)(encoder_delta->timestamp - g_last_timestamp) / 1000.0f;
        if (dt < 0.001f) dt = 0.001f;  // 最小时间步长
    }
    g_last_timestamp = encoder_delta->timestamp;
    pose_delta->dt = dt;

    return true;
}

/**
 * @brief 逆运动学解算
 *
 * 麦克纳姆轮逆运动学公式：
 * v1 = vx - vy - L * ω
 * v2 = vx + vy + L * ω
 * v3 = vx + vy - L * ω
 * v4 = vx - vy + L * ω
 *
 * 其中：
 * v1~v4: 轮子线速度 (cm/s)
 * vx, vy: 车体X/Y方向速度 (cm/s)
 * ω: 车体角速度 (rad/s)
 * L: 轮子到中心的距离 (cm)
 */
bool Mecanum_InverseKinematics(MotionState_t *motion, WheelData_t *wheel_speed) {
    if (motion == NULL || wheel_speed == NULL) {
        return false;
    }

    // 计算轮子到中心的距离
    float L = Mecanum_GetWheelDistance();

    // 逆运动学解算 - 计算轮子线速度 (cm/s)
    float v1 = motion->vx - motion->vy - L * motion->omega;  // 左前轮
    float v2 = motion->vx + motion->vy + L * motion->omega;  // 右前轮
    float v3 = motion->vx + motion->vy - L * motion->omega;  // 左后轮
    float v4 = motion->vx - motion->vy + L * motion->omega;  // 右后轮

    // 转换为角速度 (rad/s)
    wheel_speed->front_left = v1 / g_config.wheel_radius;
    wheel_speed->front_right = v2 / g_config.wheel_radius;
    wheel_speed->rear_left = v3 / g_config.wheel_radius;
    wheel_speed->rear_right = v4 / g_config.wheel_radius;

    // 速度限制
    Mecanum_LimitWheelSpeed(wheel_speed);

    return true;
}

/**
 * @brief 轮子速度限制
 */
void Mecanum_LimitWheelSpeed(WheelData_t *wheel_speed) {
    if (wheel_speed == NULL) {
        return;
    }

    float max_speed = g_config.max_wheel_speed;

    // 限制每个轮子的速度
    wheel_speed->front_left = clamp(wheel_speed->front_left, -max_speed, max_speed);
    wheel_speed->front_right = clamp(wheel_speed->front_right, -max_speed, max_speed);
    wheel_speed->rear_left = clamp(wheel_speed->rear_left, -max_speed, max_speed);
    wheel_speed->rear_right = clamp(wheel_speed->rear_right, -max_speed, max_speed);
}

/**
 * @brief 车体速度限制
 */
void Mecanum_LimitMotionSpeed(MotionState_t *motion) {
    if (motion == NULL) {
        return;
    }

    // 限制线速度
    float speed = sqrtf(motion->vx * motion->vx + motion->vy * motion->vy);
    if (speed > g_config.max_body_speed) {
        float scale = g_config.max_body_speed / speed;
        motion->vx *= scale;
        motion->vy *= scale;
    }

    // 限制角速度
    motion->omega = clamp(motion->omega, -g_config.max_angular_speed, g_config.max_angular_speed);
}
