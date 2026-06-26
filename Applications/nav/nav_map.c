/**
 * @file nav_map.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 导航地图源文件
 */

#include "nav_map.h"
#include <string.h>
#include "FreeRTOS.h"

static TargetPoint_t *g_points = NULL;
static NavMapInfo_t *g_map_info = NULL;

/**
 * @brief 初始化地图
 * @param max_point_num 最大目标点数量
 * @return 初始化状态
 */
bool Map_Init(uint8_t max_point_num) {
    g_map_info = (NavMapInfo_t *)pvPortMalloc(sizeof(NavMapInfo_t));
    if (g_map_info == NULL) {
        return false;
    }
    memset(g_map_info, 0, sizeof(NavMapInfo_t));

    g_points = (TargetPoint_t *)pvPortMalloc(max_point_num * sizeof(TargetPoint_t));
    if (g_points == NULL) {
        vPortFree(g_map_info);
        return false;
    }
    memset(g_points, 0, max_point_num * sizeof(TargetPoint_t));
    g_map_info->max_points = max_point_num;

    if (!Map_LoadPoints(Map_GetDataPoints(), Map_GetDataPointCount())) {
        vPortFree(g_points);
        vPortFree(g_map_info);
        return false;
    }

    return true;
}

/**
 * @brief 获取地图信息
 * @return 地图信息指针
 */
NavMapInfo_t *Map_GetInfo(void) {
    return g_map_info;
}

/**
 * @brief 添加目标点（支持插入）
 * @param point 目标点指针
 * @return 添加状态
 */
bool Map_AddPoint(const TargetPoint_t *point) {
    if (g_map_info->point_count >= g_map_info->max_points) {
        return false;
    }

    // 找到插入位置
    uint8_t insert_index = g_map_info->point_count;
    for (uint8_t i = 0; i < g_map_info->point_count; i++) {
        if (g_points[i].id >= point->id) {
            insert_index = i;
            break;
        }
    }

    for (uint8_t i = g_map_info->point_count; i > insert_index; i--) {
        memcpy(&g_points[i], &g_points[i - 1], sizeof(TargetPoint_t));
        g_points[i].id++;
    }

    memcpy(&g_points[insert_index], point, sizeof(TargetPoint_t));
    g_map_info->point_count++;

    return true;
}

/**
 * @brief 删除目标点（并重新编号）
 * @param id 目标点ID
 * @return 删除状态
 */
bool Map_RemovePoint(uint8_t id) {
    for (uint8_t i = 0; i < g_map_info->point_count; i++) {
        if (g_points[i].id == id) {
            for (uint8_t j = i; j < g_map_info->point_count - 1; j++) {
                memcpy(&g_points[j], &g_points[j + 1], sizeof(TargetPoint_t));
                g_points[j].id = j;
            }
            g_map_info->point_count--;

            return true;
        }
    }
    return false;
}

/**
 * @brief 根据ID获取目标点
 * @param id 目标点ID
 * @return 目标点指针
 */
TargetPoint_t *Map_GetPoint(uint8_t id) {
    if (id >= g_map_info->point_count) {
        return NULL;
    }
    return &g_points[id];
}

/**
 * @brief 修改目标点
 * @param id 目标点ID
 * @param point 新的目标点数据
 * @return 修改状态
 */
bool Map_UpdatePoint(uint8_t id, TargetPoint_t *point) {
    if (id >= g_map_info->point_count) {
        return false;
    }

    point->id = id;
    memcpy(&g_points[id], point, sizeof(TargetPoint_t));

    return true;
}

/**
 * @brief 加载预定义目标点
 * @param points 目标点数组指针
 * @param count 目标点数量
 * @return 加载状态
 */
bool Map_LoadPoints(const TargetPoint_t *points, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        if (!Map_AddPoint(&points[i])) {
            return false;
        }
    }

    return true;
}
