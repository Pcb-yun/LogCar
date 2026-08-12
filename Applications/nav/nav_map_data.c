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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 1,
        .name = "OA1",     // 出站避让点
        .pose = {
            .x = 0.0f,
            .y = -32.0f,
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 2,
        .name = "QrCode_1",     // 扫码点1
        .pose = {
            .x = 65.0f,
            .y = -32.0f,
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 3,
        .name = "MATL_TRACK",   // 物料巡线开始点
        .pose = {
            .x = 83.00f,
            .y = -14.75f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 4,
        .name = "MATL_GRAP1",   // 物料抓取点1
        .pose = {
            .x = 83.00f,
            .y = -14.75f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 5,
        .name = "MATL_GRAP2",   // 物料抓取点2
        .pose = {
            .x = 128.f,
            .y = -30.78f,
            .yaw = -47.00f
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 6,
        .name = "MATL_GRAP3",   // 物料抓取点3
        .pose = {
            .x = 160.00f,
            .y = -67.65f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 7,
        .name = "MATL_GRAP4",   // 物料抓取点4
        .pose = {
            .x = 151.77f,
            .y = -112.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 8,
        .name = "MATL_GRAP5",   // 物料抓取点5
        .pose = {
            .x = 114.00f,
            .y = -151.28f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
#if USE_RPI_CAL // 使用树莓派校准
    {
        .id = 9,
        .name = "POP_A",   // A点
        .pose = {
            .x = 72.00f,
            .y = -69.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 10,
        .name = "POP_B",    // B点
        .pose = {
            .x = 49.00f,
            .y = -42.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 11,
        .name = "POP_C",    // C点
        .pose = {
            .x = -12.00f,
            .y = -75.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 12,
        .name = "POP_D",    // D点
        .pose = {
            .x = -28.00f,
            .y = -48.80f,
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 13,
        .name = "POP_E",    // E点
        .pose = {
            .x = -77.00f,
            .y = -59.00f,
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
            .timeout_ms = 5000
        }
    },
#else // 定位死点
    {
        .id = 9,
        .name = "POP_A",   // A点
        .pose = {
            .x = 49.00f,
            .y = -90.00f,
            .yaw = -52.00f
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 10,
        .name = "POP_B",    // B点
        .pose = {
            .x = 26.00f,
            .y = -63.00f,
            .yaw = -52.00f
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 11,
        .name = "POP_C",    // C点
        .pose = {
            .x = -30.00f,
            .y = -92.00f,
            .yaw = -60.00f
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 12,
        .name = "POP_D",    // D点
        .pose = {
            .x = -43.00f,
            .y = -65.00f,
            .yaw = -60.0f
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 13,
        .name = "POP_E",    // E点
        .pose = {
            .x = -75.00f,
            .y = -71.00f,
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
            .timeout_ms = 5000
        }
    },
#endif /* USE_RPI_CAL */
    {
        .id = 14,
        .name = "QrCode_2", // 扫码点2
        .pose = {
            .x = -61.00f,
            .y = -51.50f,
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
        }
    },
    {
        .id = 15,
        .name = "TROP_TRACK", // 奖杯巡线开始点
        .pose = {
            .x = -115.00f,
            .y = -55.00f,
            .yaw = -140.0f
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
        }
    },
    {
        .id = 16,
        .name = "TROP_GRAP2", // 奖杯抓取点2
        .pose = {
            .x = -147.00f,
            .y = -90.00f,
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
        }
    },
    {
        .id = 17,
        .name = "TROP_GRAP3", // 奖杯抓取点3
        .pose = {
            .x = -136.00f,
            .y = -132.00f,
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
        }
    },
    {
        .id = 18,
        .name = "OA2",     // 奖杯放置预备点
        .pose = {
            .x = -100.00f,
            .y = -156.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
#if USE_RPI_CAL // 使用树莓派校准
    {
        .id = 19,
        .name = "SECOND",     // 亚军
        .pose = {
            .x = -11.00f,
            .y = -156.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 20,
        .name = "FIRST",     // 冠军
        .pose = {
            .x = 14.00f,
            .y = -156.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 21,
        .name = "THRID",     // 季军
        .pose = {
            .x = 41.0f,
            .y = -156.00f,
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
            .timeout_ms = 5000
        }
    },
#else // 定位死点
    {
        .id = 19,
        .name = "SECOND",     // 亚军
        .pose = {
            .x = -13.00f,
            .y = -156.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 20,
        .name = "FIRST",     // 冠军
        .pose = {
            .x = 13.00f,
            .y = -156.00f,
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
            .distance_threshold = 1.0f,
            .yaw_threshold = 1.8f,
            .timeout_ms = 5000
        }
    },
    {
        .id = 21,
        .name = "THRID",     // 季军
        .pose = {
            .x = 40.0f,
            .y = -156.00f,
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
            .timeout_ms = 5000
        }
    },
#endif
    {
        .id = 22,
        .name = "OA3",     // 返回避让点
        .pose = {
            .x = 18.0f,
            .y = -100.0f,
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
            .timeout_ms = 5000
        }
    },
    {
        .id = 23,
        .name = "SWEET_HOME",   // 回家吧孩子
        .pose = {
            .x = 0.0f,
            .y = 0.0f,
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
            .timeout_ms = 5000
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
