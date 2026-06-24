/**
 * @file nav_math.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航数学库头文件
 */

#ifndef __NAV_MATH_H__
#define __NAV_MATH_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <math.h>


#ifndef M_PI
#define M_PI                    3.14159265358979323846f
#endif

#define M_PI_2                  1.57079632679489661923f
#define M_PI_4                  0.78539816339744830962f
#define M_2PI                   6.28318530717958647692f

#define RAD_TO_DEG              (180.0f / M_PI)
#define DEG_TO_RAD              (M_PI / 180.0f)


/**
 * @brief 弧度转角度
 */
static inline float rad2deg(float rad) {
    return rad * RAD_TO_DEG;
}

/**
 * @brief 角度转弧度
 */
static inline float deg2rad(float deg) {
    return deg * DEG_TO_RAD;
}

float normalize_angle(float angle);
float angle_diff(float target, float current);

typedef struct {
    float x;
    float y;
} Vector2D_t;

typedef struct {
    float x;
    float y;
    float z;
} Vector3D_t;

/**
 * @brief 2D向量加法
 */
static inline Vector2D_t vec2_add(Vector2D_t a, Vector2D_t b) {
    Vector2D_t res = {a.x + b.x, a.y + b.y};
    return res;
}

/**
 * @brief 2D向量减法
 */
static inline Vector2D_t vec2_sub(Vector2D_t a, Vector2D_t b) {
    Vector2D_t res = {a.x - b.x, a.y - b.y};
    return res;
}

/**
 * @brief 2D向量数乘
 */
static inline Vector2D_t vec2_mul(Vector2D_t v, float scalar) {
    Vector2D_t res = {v.x * scalar, v.y * scalar};
    return res;
}

/**
 * @brief 2D向量点积
 */
static inline float vec2_dot(Vector2D_t a, Vector2D_t b) {
    return a.x * b.x + a.y * b.y;
}

/**
 * @brief 2D向量叉积（返回标量）
 */
static inline float vec2_cross(Vector2D_t a, Vector2D_t b) {
    return a.x * b.y - a.y * b.x;
}

/**
 * @brief 2D向量模长
 */
static inline float vec2_norm(Vector2D_t v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}


Vector2D_t vec2_normalize(Vector2D_t v);
float vec2_distance(Vector2D_t a, Vector2D_t b);


typedef float Matrix3x3_t[9];

void mat3_mul(Matrix3x3_t A, Matrix3x3_t B, Matrix3x3_t C);
void mat3_vec_mul(Matrix3x3_t M, float *v, float *result);
void mat3_transpose(Matrix3x3_t M, Matrix3x3_t MT);
void mat3_identity(Matrix3x3_t M);
void mat3_add(Matrix3x3_t A, Matrix3x3_t B, Matrix3x3_t C);

typedef struct {
    float x;
    float y;
    float yaw;
} Pose2D_t;

Vector2D_t world_to_local(Vector2D_t world, Pose2D_t origin);
Vector2D_t local_to_world(Vector2D_t local, Pose2D_t origin);
void world_to_grid(float wx, float wy, float resolution,
                    uint16_t *gx, uint16_t *gy);
void grid_to_world(uint16_t gx, uint16_t gy, float resolution,
                    float *wx, float *wy);

/**
 * @brief 限制值在指定范围内
 */
static inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

/**
 * @brief 符号函数
 */
static inline int8_t sign(float x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

/**
 * @brief 线性插值
 */
static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float fast_invsqrt(float x);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAV_MATH_H__ */
