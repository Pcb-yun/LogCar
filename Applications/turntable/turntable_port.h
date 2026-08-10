/**
 * @file turntable_port.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 转盘总控头文件
 */

#ifndef __TURNTABLE_PORT_H__
#define __TURNTABLE_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include "turntable_cfg.h"
#include "sensor.h"

static const char *matl_str[6] = {"unknown", "black", "white", "red", "green", "blue"};
static const char *trop_str[3] = {"A", "B", "C"};

/**
 * @brief 转盘存放类型枚举
 */
typedef enum {
    TURNTABLE_NONE = 0,    // 未设置
    TURNTABLE_MATL,        // 物料
    TURNTABLE_TROP         // 奖杯
} TurntableType_t;

/**
 * @brief 奖杯标签枚举
 */
typedef enum {
    LABEL_NONE = 0,
    LABEL_A,
    LABEL_B,
    LABEL_C,
} TropLabel_t;

/**
 * @brief 转盘入库信息结构体
 */
typedef struct {
    uint8_t id;             // 转盘id号
    SENSOR_Color_t color;   // 物料颜色
    TropLabel_t label;      // 奖杯标签
} TurntableSTO_t;

/**
 * @brief 转盘信息结构体
 */
typedef struct {
    TurntableType_t type;   // 存放类型
    uint8_t order;          // 扫码数据
} TurntablePort_t;

/**
 * @brief 转盘出库类型枚举
 */
typedef enum {
    MATL_A = 0,     // 物料A
    MATL_B,         // 物料B
    MATL_C,         // 物料C
    MATL_D,         // 物料D
    MATL_E,         // 物料E
    TROP_A,         // 奖杯A
    TROP_B,         // 奖杯B
    TROP_C          // 奖杯C
} TurntablePop_t;


bool Turntable_Port_Init(void);
void Turntable_Port_SetType(TurntableType_t type);
bool Turntable_Pop(TurntablePop_t pop);
void Turntable_SetOrder(uint8_t order);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TURNTABLE_PORT_H__ */
