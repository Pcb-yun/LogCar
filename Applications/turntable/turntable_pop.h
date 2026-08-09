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

typedef enum {
    A = 0,
    B,
    C,
    LETTER_NUM
} TurntableLetter_t;

/**
 * @brief 设置物料出栈顺序
 * @param number 扫码模块给的二维码数字(1~16)
 * @return true 设置成功; false 数字越界
 *
 * 数字对应的出栈颜色顺序见 turntable_pop.c 中的 pop_order_goods 表,
 * 调用后出栈进度重置为 0。
 */
bool Turntable_Pop_SetGoodsSequence(uint8_t number);

/**
 * @brief 设置奖杯模式
 * @param number 第二次扫码值 1~6(决定奖杯入栈顺序)
 * @return true 设置成功; false 数字越界
 *
 * 码值标签对应物理入栈顺序(逆序):
 *   1 ABC -> C,B,A    2 ACB -> B,C,A
 *   3 BAC -> C,A,B    4 BCA -> A,C,B
 *   5 CAB -> B,A,C    6 CBA -> A,B,C
 * 出栈顺序由 award_pop_order 表自定义(默认与码值标签一致), 见 turntable_pop.c。
 */
bool Turntable_Pop_SetAwardSequence(uint8_t number);

/**
 * @brief 指定出栈: 按调用方指定的字母出一个物品
 * @param letter 指定字母:
 *               物料模式 'a'~'e' -> pop_order_goods 颜色顺序第 1~5 位;
 *               奖杯模式 'a'~'c' -> award_pop_order 自定义出栈顺序第 1~3 位
 * @return true 本次出栈执行完成; false 顺序未设置、参数非法或目标不存在
 *
 * 每次调用只出一个物品:
 *   1. 从分拣存储数据中找到目标槽位(物料按颜色, 奖杯按入栈顺序)
 *   2. 旋转转盘把该槽位转到出料口
 * 取消距离检测, 旋转到位即认为出栈成功。
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
