#ifndef __TURNTABLE_POP_H__
#define __TURNTABLE_POP_H__

#include <stdbool.h>
#include <stdint.h>
#include "turntable_conf.h"

/**
 * @brief 物料颜色(与 TCS230 识别结果名称一致)
 *        黑=Black, 白=White, 红=Red, 绿=Green, 蓝=Blue
 */
typedef enum {
    TURNTABLE_COLOR_BLACK = 0,
    TURNTABLE_COLOR_WHITE,
    TURNTABLE_COLOR_RED,
    TURNTABLE_COLOR_GREEN,
    TURNTABLE_COLOR_BLUE,
    TURNTABLE_COLOR_NUM
} TurntableColor_t;

/**
 * @brief 设置出栈顺序
 * @param number 扫码模块给的二维码数字(1~16)
 * @return true 设置成功; false 数字越界
 *
 * 数字对应的出栈颜色顺序见 turntable_pop.c 中的 pop_order 表,
 * 调用后出栈进度重置为 0。
 */
bool Turntable_Pop_SetSequence(uint8_t number);

/**
 * @brief 出栈一步: 按当前顺序出掉下一个颜色的物料
 * @return true 本次出栈执行完成; false 顺序未设置或已全部出完
 *
 * 每次调用只出一个物料:
 *   1. 从分拣存储数据中找到该颜色所在槽位
 *   2. 旋转转盘把该槽位转到出料口
 *   3. 通过 VL53L0X 检测物料是否已离开转盘
 * 阻塞直到物料离开(或超时)。
 */
bool Turntable_Pop_Step(void);

/**
 * @brief 获取当前出栈进度(已完成的步数 0~TURNTABLE_ITEM_MAX)
 */
uint8_t Turntable_Pop_GetStep(void);

/**
 * @brief 获取当前出栈顺序(长度为 TURNTABLE_ITEM_MAX 的颜色序列)
 */
const uint8_t *Turntable_Pop_GetSequence(void);

/**
 * @brief 结束出栈会话, 释放转盘并恢复入栈任务
 * @note 全部出完时自动调用; 需要提前中断时可手动调用
 */
void Turntable_Pop_End(void);

#endif /* __TURNTABLE_POP_H__ */
