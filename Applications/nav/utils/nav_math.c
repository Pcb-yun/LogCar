/**
 * @file nav_math.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航数学库源文件
 */

#include "nav_math.h"


/**
 * @brief 角度归一化
 * @param angle 角度值(度)
 * @return 归一化后的角度值(度)，范围[-180, 180]
 */
float normalize_angle(float angle) {
    while (angle > 180.0f) {
        angle -= 360.0f;
    }
    while (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

/**
 * @brief 计算角度差
 * @param target 目标角度
 * @param current 当前角度
 * @return 角度差值
 */
float angle_diff(float target, float current) {
    return normalize_angle(target - current);
}

/**
 * @brief 2D向量归一化
 * @param v 2D向量
 * @return 归一化后的2D向量
 */
Vector2D_t vec2_normalize(Vector2D_t v) {
    float norm = vec2_norm(v);
    if (norm < 1e-6f) {
        Vector2D_t zero = {0, 0};
        return zero;
    }
    return vec2_mul(v, 1.0f / norm);
}

/**
 * @brief 计算两点之间的距离
 * @param a 第一个点
 * @param b 第二个点
 * @return 两点之间的距离
 */
float vec2_distance(Vector2D_t a, Vector2D_t b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

/**
 * @brief 3x3矩阵乘法
 * @param A 第一个矩阵
 * @param B 第二个矩阵
 * @param C 结果矩阵
 */
void mat3_mul(Matrix3x3_t A, Matrix3x3_t B, Matrix3x3_t C) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            C[i * 3 + j] = 0;
            for (int k = 0; k < 3; k++) {
                C[i * 3 + j] += A[i * 3 + k] * B[k * 3 + j];
            }
        }
    }
}

/**
 * @brief 3x3矩阵与向量乘法
 * @param M 矩阵
 * @param v 向量
 * @param result 结果向量
 */
void mat3_vec_mul(Matrix3x3_t M, float *v, float *result) {
    for (int i = 0; i < 3; i++) {
        result[i] = 0;
        for (int j = 0; j < 3; j++) {
            result[i] += M[i * 3 + j] * v[j];
        }
    }
}

/**
 * @brief 3x3矩阵转置
 * @param M 矩阵
 * @param MT 转置后的矩阵
 */
void mat3_transpose(Matrix3x3_t M, Matrix3x3_t MT) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            MT[i * 3 + j] = M[j * 3 + i];
        }
    }
}

/**
 * @brief 3x3矩阵赋值为单位矩阵
 * @param M 矩阵
 */
void mat3_identity(Matrix3x3_t M) {
    for (int i = 0; i < 9; i++) {
        M[i] = (i % 4 == 0) ? 1.0f : 0.0f;
    }
}

/**
 * @brief 3x3矩阵加法
 * @param A 第一个矩阵
 * @param B 第二个矩阵
 * @param C 结果矩阵
 */
void mat3_add(Matrix3x3_t A, Matrix3x3_t B, Matrix3x3_t C) {
    for (int i = 0; i < 9; i++) {
        C[i] = A[i] + B[i];
    }
}

/**
 * @brief 世界坐标转换为本地坐标
 * @param world 世界坐标
 * @param origin 原始点
 * @return 本地坐标
 */
Vector2D_t world_to_local(Vector2D_t world, Pose2D_t origin) {
    float dx = world.x - origin.x;
    float dy = world.y - origin.y;

    Vector2D_t local;
    local.x = dx * cosf(origin.yaw) + dy * sinf(origin.yaw);
    local.y = -dx * sinf(origin.yaw) + dy * cosf(origin.yaw);

    return local;
}

/**
 * @brief 本地坐标转换为世界坐标
 * @param local 本地坐标
 * @param origin 原始点
 * @return 世界坐标
 */
Vector2D_t local_to_world(Vector2D_t local, Pose2D_t origin) {
    Vector2D_t world;
    world.x = origin.x + local.x * cosf(origin.yaw) - local.y * sinf(origin.yaw);
    world.y = origin.y + local.x * sinf(origin.yaw) + local.y * cosf(origin.yaw);

    return world;
}

/**
 * @brief 世界坐标转换为网格坐标
 * @param wx 世界坐标x
 * @param wy 世界坐标y
 * @param resolution 网格分辨率
 * @param gx 网格坐标x
 * @param gy 网格坐标y
 */
void world_to_grid(float wx, float wy, float resolution,
                    uint16_t *gx, uint16_t *gy) {
    *gx = (uint16_t)(wx / resolution + 0.5f);
    *gy = (uint16_t)(wy / resolution + 0.5f);
}

/**
 * @brief 网格坐标转换为世界坐标
 * @param gx 网格坐标x
 * @param gy 网格坐标y
 * @param resolution 网格分辨率
 * @param wx 世界坐标x
 * @param wy 世界坐标y
 */
void grid_to_world(uint16_t gx, uint16_t gy, float resolution,
                    float *wx, float *wy) {
    *wx = (float)gx * resolution;
    *wy = (float)gy * resolution;
}

/**
 * @brief 快速倒平方根
 * @param x 输入值
 * @return 快速近似倒平方根
 */
float fast_invsqrt(float x) {
    // 快速近似算法（Quake III风格）
    float xhalf = 0.5f * x;
    int32_t i = *(int32_t*)&x;
    i = 0x5f3759df - (i >> 1);
    x = *(float*)&i;
    x = x * (1.5f - xhalf * x * x);
    return x;
}
