/**
 * @file nav_map_data.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航地图目标点数据定义
 */

#include "nav_map.h"

/**
 * @brief 定义所有目标点数据
 */
static TargetPoint_t g_map_points[] = {
    // 起点/原点
    {
        .id = 0,
        .name = "Origin",
        .type = TARGET_POINT_NORMAL,
        .pose = {
            .x = 0.0f,
            .y = 0.0f,
            .yaw = 0.0f
        },
        .motion = {
            .target_speed = 80.0f,
            .target_angular_speed = 50.0f,
            .acceleration = 30.0f,
            .deceleration = 30.0f,
            .angular_acceleration = 20.0f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        },
        .enable = true
    },
    {
        .id = 1,
        .name = "Test_1",
        .type = TARGET_POINT_PICKUP,
        .pose = {
            .x = 50.0f,
            .y = 0.0f,
            .yaw = 90.0f
        },
        .motion = {
            .target_speed = 80.0f,
            .target_angular_speed = 50.0f,
            .acceleration = 30.0f,
            .deceleration = 30.0f,
            .angular_acceleration = 20.0f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        },
        .enable = true
    },
    {
        .id = 2,
        .name = "Test_2",
        .type = TARGET_POINT_PICKUP,
        .pose = {
            .x = 50.0f,
            .y = 50.0f,
            .yaw = 180.0f
        },
        .motion = {
            .target_speed = 80.0f,
            .target_angular_speed = 50.0f,
            .acceleration = 30.0f,
            .deceleration = 30.0f,
            .angular_acceleration = 20.0f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        },
        .enable = true
    },
    {
        .id = 3,
        .name = "Test_3",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = 0.0f,
            .y = 50.0f,
            .yaw = 270.0f
        },
        .motion = {
            .target_speed = 80.0f,
            .target_angular_speed = 50.0f,
            .acceleration = 30.0f,
            .deceleration = 30.0f,
            .angular_acceleration = 20.0f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        },
        .enable = true
    }
};

/**
 * @brief 获取预定义目标点数组
 */
TargetPoint_t *Map_GetDataPoints(void) {
    return g_map_points;
}

/**
 * @brief 获取预定义目标点数量
 */
uint8_t Map_GetDataPointCount(void) {
    return sizeof(g_map_points) / sizeof(TargetPoint_t);
}
