/**
 * @file nav_local.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航定位源文件
 */

#include "FreeRTOS.h"
#include "nav_local.h"
#include "cmsis_os2.h"
#include "task.h"
#include "Events.h"
#include <string.h>
#include "track.h"
#include "nav_math.h"
#include "motion_control.h"
#include "ops.h"
#include "log.h"
#include "nav_config.h"

/**
 * @brief 带权重的位姿数据
 */
typedef struct {
    Pose2D_t pose;          // 位姿数据
    uint32_t timestamp;     // 时间戳
    float weight;           // 置信度权重 (0.0-1.0)
} WeightedPose_t;

extern osMutexId_t Pose_MutexHandle;
static PoseTimestamp_t *g_pose = NULL;
static WeightedPose_t *sensor_sources[NAV_MAX_SENSOR];
static bool is_init = false;
static void Loc_Update(void);
static void Loc_Fusion_WeightedAverage(WeightedPose_t *sources[], uint8_t count, Pose2D_t *fused_pose);


/**
 * @brief 导航定位初始化
 * @return 初始化结果
 */
bool Loc_Init(void) {
    g_pose = pvPortMalloc(sizeof(PoseTimestamp_t));
    if (g_pose == NULL) {
        return false;
    }

    for (uint8_t i = 0; i < NAV_MAX_SENSOR; i++) {
        sensor_sources[i] = pvPortMalloc(sizeof(WeightedPose_t));
        if (sensor_sources[i] == NULL) {
            for (uint8_t j = 0; j < i; j++) {
                vPortFree(sensor_sources[j]);
            }
            vPortFree(g_pose);
            return false;
        }
        memset(sensor_sources[i], 0, sizeof(WeightedPose_t));
    }

    memset(g_pose, 0, sizeof(PoseTimestamp_t));

    is_init = true;
    return true;
}

/**
 * @brief 定位任务
 */
void Loc_Update_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    if (!is_init) vTaskDelete(NULL);

    for (;;) {
        Loc_Update();
        osDelay(1);
    }
}

/**
 * @brief 获取当前导航位姿
 * @param pose 输出参数，用于存储当前位姿
 * @return 获取结果
 */
bool Loc_Get(PoseTimestamp_t *pose) {
    if (!is_init) return false;

    if (osMutexAcquire(Pose_MutexHandle, osWaitForever) == osOK) {
        *pose = *g_pose;
        osMutexRelease(Pose_MutexHandle);
        return true;
    }

    return false;
}

/**
 * @brief 定位更新
 */
static void Loc_Update(void) {
    uint8_t source_count = 0;

    // 获取平面定位数据
    Pose2D_t ops_pose;
    OPSData_t ops_data;
    bool ops_rec = OPS_Get(&ops_data);
    if (ops_rec) {
        ops_pose.x = ops_data.x / 10.0f;
        ops_pose.y = ops_data.y / 10.0f;
        ops_pose.yaw = ops_data.yaw;
        sensor_sources[source_count]->pose = ops_pose;
        sensor_sources[source_count]->timestamp = ops_data.timestamp;
        sensor_sources[source_count]->weight = 0.9f;    // 平面定位权重
        source_count++;
    }

    // 数据融合
    Pose2D_t fused_pose;
    if (source_count > 0) {
        Loc_Fusion_WeightedAverage(sensor_sources, source_count, &fused_pose);
    } else {
        fused_pose = g_pose->pose;
    }

    if (osMutexAcquire(Pose_MutexHandle, osWaitForever) == osOK) {
        g_pose->pose = fused_pose;
        g_pose->timestamp = osKernelGetTickCount();
        osMutexRelease(Pose_MutexHandle);
    }
}

/**
 * @brief 带权重的位姿融合算法
 * @param sources 多源位姿数据数组
 * @param count 数据源数量
 * @param fused_pose 融合后的位姿
 */
static void Loc_Fusion_WeightedAverage(WeightedPose_t *sources[], uint8_t count, Pose2D_t *fused_pose) {
    // if (count == 1) {
    //     *fused_pose = sources[0]->pose; return;
    // }

    uint32_t current_tick = osKernelGetTickCount();
    float total_weight = 0.0f;
    float sum_x = 0.0f, sum_y = 0.0f;
    float sum_yaw = 0.0f;

    for (uint8_t i = 0; i < count; i++) {
        if (sources[i]->weight > 0.0f) {
            uint32_t time_diff = current_tick - sources[i]->timestamp;

            float time_weight = expf(-NAV_TIME_DECAY_FACTOR * time_diff / 1000.0f);
            float final_weight = sources[i]->weight * time_weight;

            sum_x += sources[i]->pose.x * final_weight;
            sum_y += sources[i]->pose.y * final_weight;
            sum_yaw += sources[i]->pose.yaw * final_weight;

            total_weight += final_weight;
        }
    }

    if (total_weight > 0.0f) {
        fused_pose->x = sum_x / total_weight;
        fused_pose->y = sum_y / total_weight;

        float raw_yaw = sum_yaw / total_weight;

        static float last_yaw = 0.0f;
        float alpha = 0.5f;
        fused_pose->yaw = alpha * raw_yaw + (1 - alpha) * last_yaw;
        last_yaw = fused_pose->yaw;
    }
}
