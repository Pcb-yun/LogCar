#ifndef __TURNTABLE_SORT_H__
#define __TURNTABLE_SORT_H__

#include <stdbool.h>
#include <stdint.h>
#include "turntable_conf.h"

/**
 * @brief 分拣物品种类
 */
typedef enum {
    TURNTABLE_ITEM_GOODS = 0,   /**< 物料(需要测颜色) */
    TURNTABLE_ITEM_AWARD,       /**< 奖杯(不需要测颜色) */
} TurntableItemType_t;

/**
 * @brief 分拣存储的物料数据
 */
typedef struct {
    uint8_t  id;                /**< 物品所在槽位 ID */
    TurntableItemType_t type;   /**< 种类: 物料/奖杯 */
    char     color[16];         /**< 颜色名称(奖杯为 "-") */
    uint8_t  confidence;        /**< 可信度 0-100 */
    char     letter;            /**< 奖杯上印制的字母(A/B/C), 物料为 0 */
    bool     valid;             /**< 该槽位是否已存放物品 */
} TurntableItem_t;

void Turntable_Sort_Stop(void);
void Turntable_Sort_Clear(void);
uint8_t Turntable_Sort_GetCount(void);
const TurntableItem_t *Turntable_Sort_GetItems(void);
void Turntable_Sort_Task(void *argument);

/**
 * @brief 暂停入栈(出栈占用转盘时调用)
 * @note 会等待入栈当前动作结束, 避免与出栈同时驱动转盘
 */
void Turntable_Sort_Pause(void);

/**
 * @brief 恢复入栈(出栈结束释放转盘时调用)
 */
void Turntable_Sort_Resume(void);

/**
 * @brief 移除某槽位的物料记录(出栈成功后调用)
 * @param slot_id 槽位 id
 * @return true 找到并移除
 */
bool Turntable_Sort_Remove(uint8_t slot_id);

#endif /* __TURNTABLE_SORT_H__ */
