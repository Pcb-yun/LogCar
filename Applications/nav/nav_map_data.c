/**
 * @file nav_map_data.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航地图目标点数据定义
 */

#include "nav_map.h"
#include "mission.h"

/**
 * @brief 定义所有目标点数据
 */
static const TargetPoint_t g_map_points[] = {
    {
        .id = 0,
        .name = "HOME",     // 出发点
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
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 1,
        .name = "OA1",     // 出站避让点
        .pose = {
            .x = 0.0f,
            .y = -25.96f,
            .yaw = 0.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_DISTANCE,
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 2,
        .name = "QrCode_1",     // 扫码点1
        .pose = {
            .x = 58.34f,
            .y = -31.79f,
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
            .yaw_threshold = 2.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 3,
        .name = "MATL_TRACK",   // 物料巡线开始点
        .pose = {
            .x = 29.96f,
            .y = -10.65f,
            .yaw = 0.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.0f,
            .yaw_threshold = 2.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 4,
        .name = "MATL_GRAP1",   // 物料抓取点1
        .pose = {
            .x = 97.59f,
            .y = -13.07f,
            .yaw = -8.5f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 5,
        .name = "MATL_GRAP2",   // 物料抓取点2
        .pose = {
            .x = 131.47f,
            .y = -12.04f,
            .yaw = -73.87f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 6,
        .name = "MATL_GRAP3",   // 物料抓取点3
        .pose = {
            .x = 153.41f,
            .y = -55.93f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 7,
        .name = "MATL_GRAP4",   // 物料抓取点4
        .pose = {
            .x = 149.96f,
            .y = -103.10f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 8,
        .name = "MATL_GRAP5",   // 物料抓取点5
        .pose = {
            .x = 119.41f,
            .y = -141.19f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
#if MISSION_USE_RPI_CAL // 使用树莓派校准
    {
        .id = 9,
        .name = "POP_A",   // A点
        .pose = {
            .x = 68.93f,
            .y = -71.25f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 10,
        .name = "POP_B",    // B点
        .pose = {
            .x = 46.59f,
            .y = -44.59f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 11,
        .name = "POP_C",    // C点
        .pose = {
            .x = -16.45f,
            .y = -75.70f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 12,
        .name = "POP_D",    // D点
        .pose = {
            .x = -29.14f,
            .y = -48.47f,
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
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 13,
        .name = "POP_E",    // E点
        .pose = {
            .x = -78.53f,
            .y = -57.87f,
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
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
#else // 定位死点
    {
        .id = 9,
        .name = "POP_A",   // A点
        .pose = {
            .x = 47.85f,
            .y = -91.22f,
            .yaw = -51.71f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 10,
        .name = "POP_B",    // B点
        .pose = {
            .x = 26.16f,
            .y = -65.51f,
            .yaw = -51.71f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 11,
        .name = "POP_C",    // C点
        .pose = {
            .x = -28.22f,
            .y = -92.98f,
            .yaw = -64.30f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 12,
        .name = "POP_D",    // D点
        .pose = {
            .x = -45.54f,
            .y = -69.24f,
            .yaw = -64.30f  // -57.83
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 13,
        .name = "POP_E",    // E点
        .pose = {
            .x = -94.69f,
            .y = -80.00f,
            .yaw = -59.33f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
#endif /* USE_RPI_CAL */
    {
        .id = 14,
        .name = "QrCode_2", // 扫码点2
        .pose = {
            .x = -67.0f,
            .y = -41.90f,
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
            .yaw_threshold = 2.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 15,
        .name = "TROP_TRACK", // 奖杯巡线开始点
        .pose = {
            .x = -90.03f,
            .y = -21.63f,
            .yaw = -150.0f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 1.5f,
            .yaw_threshold = 2.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 16,
        .name = "TROP_GRAP1", // 奖杯抓取点1
        .pose = {
            .x = -138.46f,
            .y = -15.00f,
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
            .distance_threshold = 2.0f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 17,
        .name = "TROP_GRAP2", // 奖杯抓取点2
        .pose = {
            .x = -150.02f,
            .y = -60.40f,
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
            .distance_threshold = 2.0f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 18,
        .name = "TROP_GRAP3", // 奖杯抓取点3
        .pose = {
            .x = -141.2f,
            .y = -104.37f,
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
            .distance_threshold = 2.0f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    #if MISSION_USE_RPI_CAL // 使用树莓派校准
    {
        .id = 19,
        .name = "OA2",     // 奖杯放置预备点
        .pose = {
            .x = -141.2f,
            .y = -159.81f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.5f,
            .yaw_threshold = 4.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 20,
        .name = "SECOND",     // 亚军
        .pose = {
            .x = -27.61f,
            .y = -159.81f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 21,
        .name = "FIRST",     // 冠军
        .pose = {
            .x = -0.22f,
            .y = -158.42f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 22,
        .name = "THRID",     // 季军
        .pose = {
            .x = 26.90f,
            .y = -157.32f,
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
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
#else // 定位死点
    {
        .id = 19,
        .name = "OA2",     // 奖杯放置预备点
        .pose = {
            .x = -133.12f,
            .y = -172.75f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 2.5f,
            .yaw_threshold = 4.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 20,
        .name = "SECOND",     // 亚军
        .pose = {
            .x = -8.28f,
            .y = -172.75f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 21,
        .name = "FIRST",     // 冠军
        .pose = {
            .x = 16.69f,
            .y = -169.12f,
            .yaw = -90.00f
        },
        .motion = {
            .target_speed = 90.0f,
            .target_angular_speed = 220.0f,
            .acceleration = 110.0f,
            .deceleration = 70.0f,
        },
        .arrive = {
            .check_mode = ARRIVE_CHECK_BOTH,
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 22,
        .name = "THRID",     // 季军
        .pose = {
            .x = 44.49f,
            .y = -194.71f,
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
            .distance_threshold = 0.5f,
            .yaw_threshold = 1.5f,
            .timeout_ms = 10000
        }
    },
#endif
    {
        .id = 23,
        .name = "OA3",     // 返回避让点
        .pose = {
            .x = 12.87f,
            .y = -89.49f,
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
            .distance_threshold = 1.8f,
            .yaw_threshold = 3.0f,
            .timeout_ms = 10000
        }
    },
    {
        .id = 24,
        .name = "SWEET_HOME",   // 回家吧孩子
        .pose = {
            .x = 4.0f,
            .y = 9.0f,
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
            .distance_threshold = 1.5f,
            .yaw_threshold = 2.0f,
            .timeout_ms = 10000
        }
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
