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
            .target_speed = 20.0f,
            .target_angular_speed = 0.3f,
            .acceleration = 15.0f,
            .deceleration = 20.0f,
            .angular_acceleration = 0.5f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 0.05f,
            .timeout_ms = 10000
        },
        .enable = true
    },

    // 取货点A
    {
        .id = 1,
        .name = "Pickup_A",
        .type = TARGET_POINT_PICKUP,
        .pose = {
            .x = 100.0f,
            .y = 0.0f,
            .yaw = 0.0f
        },
        .motion = {
            .target_speed = 30.0f,
            .target_angular_speed = 0.5f,
            .acceleration = 20.0f,
            .deceleration = 30.0f,
            .angular_acceleration = 1.0f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 3.0f,
            .yaw_threshold = 0.1f,
            .timeout_ms = 15000
        },
        .enable = true
    },

    // 取货点B
    {
        .id = 2,
        .name = "Pickup_B",
        .type = TARGET_POINT_PICKUP,
        .pose = {
            .x = 200.0f,
            .y = 0.0f,
            .yaw = 0.0f
        },
        .motion = {
            .target_speed = 30.0f,
            .target_angular_speed = 0.5f,
            .acceleration = 20.0f,
            .deceleration = 30.0f,
            .angular_acceleration = 1.0f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 3.0f,
            .yaw_threshold = 0.1f,
            .timeout_ms = 15000
        },
        .enable = true
    },

    // 送货点1
    {
        .id = 3,
        .name = "Delivery_1",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = 150.0f,
            .y = 100.0f,
            .yaw = 180.0f
        },
        .motion = {
            .target_speed = 25.0f,
            .target_angular_speed = 0.4f,
            .acceleration = 15.0f,
            .deceleration = 25.0f,
            .angular_acceleration = 0.8f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 0.08f,
            .timeout_ms = 12000
        },
        .enable = true
    },

    // 送货点2
    {
        .id = 4,
        .name = "Delivery_2",
        .type = TARGET_POINT_DELIVERY,
        .pose = {
            .x = 300.0f,
            .y = 150.0f,
            .yaw = 90.0f
        },
        .motion = {
            .target_speed = 25.0f,
            .target_angular_speed = 0.4f,
            .acceleration = 15.0f,
            .deceleration = 25.0f,
            .angular_acceleration = 0.8f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 0.08f,
            .timeout_ms = 12000
        },
        .enable = true
    },

    // 等待点1
    {
        .id = 5,
        .name = "Wait_1",
        .type = TARGET_POINT_WAIT,
        .pose = {
            .x = 100.0f,
            .y = 50.0f,
            .yaw = 0.0f
        },
        .motion = {
            .target_speed = 20.0f,
            .target_angular_speed = 0.3f,
            .acceleration = 15.0f,
            .deceleration = 20.0f,
            .angular_acceleration = 0.5f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_DISTANCE,
            .distance_threshold = 2.0f,
            .yaw_threshold = 0.1f,
            .timeout_ms = 8000
        },
        .enable = true
    },

    // 暂停点
    {
        .id = 6,
        .name = "Pause_1",
        .type = TARGET_POINT_PAUSE,
        .pose = {
            .x = 200.0f,
            .y = 50.0f,
            .yaw = 0.0f
        },
        .motion = {
            .target_speed = 20.0f,
            .target_angular_speed = 0.3f,
            .acceleration = 15.0f,
            .deceleration = 20.0f,
            .angular_acceleration = 0.5f
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_DISTANCE,
            .distance_threshold = 2.0f,
            .yaw_threshold = 0.1f,
            .timeout_ms = 5000
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
