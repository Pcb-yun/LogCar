#ifndef __TURNTABLE_POP_H__
#define __TURNTABLE_POP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "turntable_cfg.h"

/**
 * @brief 物料颜色(与 TCS230 识别结果名称一致)
 *        黑=Black, 白=White, 红=Red, 绿=Green, 蓝=Blue
 */
typedef enum {
    BLACK = 0,
    WHITE,
    RED,
    GREEN,
    BLUE,
    COLOR_NUM
} TurntableColor_t;

/**
 * @brief 奖杯字母(A/B/C)
 */
typedef enum {
    TROPHY_A = 0,
    TROPHY_B,
    TROPHY_C,
    LETTER_NUM
} TurntableLetter_t;

/**
 * @brief 统一的 pop 接口返回结果
 */
typedef struct {
    bool     valid;          /* 本次调用是否有效 */
    uint8_t  mode;           /* 0=物料模式, 1=奖杯模式 */
    char     target;         /* 目标点位: 物料 'A'~'E' 或 奖杯 '1'~'3' */
    uint8_t  color;          /* 物料颜色枚举(物料模式) */
    char     color_name[16]; /* 颜色名称(物料模式) */
    uint8_t  trophy;         /* 奖杯字母 A/B/C(奖杯模式) */
    uint8_t  slot_id;        /* 转盘目标槽位 */
    char     action[48];     /* 取放动作描述 */
} Turntable_PopResult_t;

/**
 * @brief 设置物料出栈顺序
 * @param number 第一次扫码的二维码数字(1~16)
 * @return true 设置成功; false 数字越界
 *
 * 数字对应的出栈颜色顺序见 turntable_pop.c 中的 pop_order_goods 表,
 * 调用后出栈进度重置为 0。
 */
bool Turntable_Pop_SetGoodsSequence(uint8_t number);

/**
 * @brief 设置奖杯模式
 * @param number 第二次扫码值 1~6(决定奖杯初始位置映射)
 * @return true 设置成功; false 数字越界
 *
 * 码值对应奖杯位置映射表 award_position:
 *   1 ABC -> 点位1=A 点位2=B 点位3=C
 *   2 ACB -> 点位1=A 点位2=C 点位3=B
 *   3 BAC -> 点位1=B 点位2=A 点位3=C
 *   4 BCA -> 点位1=B 点位2=C 点位3=A
 *   5 CAB -> 点位1=C 点位2=A 点位3=B
 *   6 CBA -> 点位1=C 点位2=B 点位3=A
 * 奖杯物理入栈顺序取其逆序, 见 turntable_pop.c。
 */
bool Turntable_Pop_SetAwardSequence(uint8_t number);

/**
 * @brief 统一的出栈接口
 * @param arg 参数: 字符串字母("a"~"e"/"A"~"E")或数字("1"~"5")
 * @return 取放指导信息(结构体)
 *
 * 参数解析:
 *   - 字母 a~e / A~E: 目标点位字母(物料模式点位 A~E, 奖杯模式奖杯 A~C)
 *   - 数字 1~5:       目标点位序号(物料模式 1=A ... 5=E, 奖杯模式 1~3 为数字点位)
 * 返回信息指导机器人的取放动作, 包括目标槽位、物品颜色/奖杯字母、动作描述。
 */
Turntable_PopResult_t Turntable_Pop_Execute(const char *arg);

/**
 * @brief 指定出栈: 按调用方指定的字母出一个物品(兼容旧接口)
 * @param letter 指定字母:
 *               物料模式 'a'~'e' -> pop_order_goods 颜色顺序第 1~5 位;
 *               奖杯模式 'a'~'c' -> 奖杯字符 A~C
 * @return true 本次出栈执行完成; false 顺序未设置、参数非法或目标不存在
 *
 * 每次调用只出一个物品:
 *   1. 从分拣存储数据中找到目标槽位(物料按颜色, 奖杯按入栈顺序)
 *   2. 旋转转盘把该槽位转到出料口
 * 旋转到位即认为出栈成功。
 */
bool Turntable_Pop_StepChar(uint8_t letter);

/**
 * @brief 获取当前已成功出栈的数量
 */
uint8_t Turntable_Pop_GetStep(void);

/**
 * @brief 获取物料出栈顺序(长度为 TURNTABLE_ITEM_MAX 的颜色序列)
 */
const uint8_t *Turntable_Pop_GetGoodsSequence(void);

/**
 * @brief 获取奖杯出栈顺序(长度为 LETTER_NUM 的字母序列)
 */
const uint8_t *Turntable_Pop_GetAwardSequence(void);

/**
 * @brief 结束出栈会话, 释放转盘并恢复入栈任务
 * @note 全部出完时自动调用; 需要提前中断时可手动调用
 */
void Turntable_Pop_End(void);

#ifdef __cplusplus
}
#endif

#endif /* __TURNTABLE_POP_H__ */
