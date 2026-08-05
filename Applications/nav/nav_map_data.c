/**
 * @file nav_map_data.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航地图目标点数据定义
 */

#include "nav_map.h"

/**
 * @brief 定义所有目标点数据
 */
static const TargetPoint_t g_map_points[] = {
    // 起点/原点
    {
        .id = 0,
        .name = "HOME",
        .type = TARGET_POINT_NORMAL,
        .pose = {
            .x = 0.0f,
            .y = 0.0f,
            .yaw = 0.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
    {
        .id = 1,
        .name = "QrCode_1",
        .type = TARGET_POINT_PICKUP,
        .pose = {
            .x = 64.00f,
            .y = -26.50f,
            .yaw = -90.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
    {
        .id = 2,
        .name = "a",
        .type = TARGET_POINT_PICKUP,
        .pose = {
            .x = 65.00f,
            .y = -94.00f,
            .yaw = -90.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
    {
        .id = 3,
        .name = "b",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = 45.00f,
            .y = -48.70f,
            .yaw = -90.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
    {
        .id = 4,
        .name = "c",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = -18.00f,
            .y = -94.00f,
            .yaw = -90.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
    {
        .id = 5,
        .name = "d",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = -30.50f,
            .y = -48.70f,
            .yaw = -90.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
    {
        .id = 5,
        .name = "e",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = -80.50f,
            .y = -48.70f,
            .yaw = -90.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
    {
        .id = 6,
        .name = "QrCode_2",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = -65.00f,
            .y = -26.50f,
            .yaw = -90.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 15000
        },
        .enable = true
    },
};

/**
 * @brief 获取预定义目标点数组
 */
const TargetPoint_t *Map_GetDataPoints(void) {
    return g_map_points;
}

/**
 * @brief 获取预定义目标点数量
 */
uint8_t Map_GetDataPointCount(void) {
    return sizeof(g_map_points) / sizeof(TargetPoint_t);
}
