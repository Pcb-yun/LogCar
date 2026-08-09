/**
 * @file turntable_pop.c
 * @brief 转盘出栈程序
 *
 * 出栈方式为"指定出栈": 扫码后由调用方指定字母, 每次出一个物品:
 *   - 第1次扫码(物料): 
 *     字母对应 pop_order_goods 颜色顺序的第 1~5 位
 *   - 第2次扫码(奖杯): 
 *     直接指定奖杯字符 A~C, 槽位由扫码决定的物理入栈顺序推导
 *     (1 ABC->CBA、2 ACB->BCA、3 BAC->CBA、4 BCA->ABC、5 CAB->ABC、6 CBA->ABC)
 * 每调用一次 Turntable_Pop_StepChar(letter) 出一个物品:
 *   1. 从分拣存储数据中找到目标槽位(物料按颜色, 奖杯按入栈顺序推导的槽位)
 *   2. 旋转转盘把该槽位转到出料口
 *   3. 出栈成功后移除该槽位的分拣记录
 *
 * 与入栈任务的互斥:
 *   首次出栈自动调用 Turntable_Sort_Pause() 暂停入栈并等待其当前动作结束,
 *   出完一轮或调用 Turntable_Pop_End() 后恢复入栈, 避免两者同时驱动转盘。
 *
 * 用法(Shell):
 *   turntable_pop set_goods <N>  设置物料出栈顺序(扫码值 1~16)
 *   turntable_pop set_award <N>  设置奖杯模式(扫码值 1~6, 入栈顺序)
 *   turntable_pop step <a~e>     指定出栈一个物品(物料 a~e, 奖杯 a~c)
 *   turntable_pop end            提前结束出栈会话, 恢复入栈
 *   turntable_pop status         查看当前顺序与进度
 */

#include "turntable_pop.h"
#include "turntable_sort.h"
#include "turntable_ctrl.h"
#include "shell.h"
#include "log.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdlib.h>

/* 颜色名称(与 SENSOR_InferColor 输出一致) */
static const char *const color_names[COLOR_NUM] = {
    "Black", "White", "Red", "Green", "Blue"
};

/* 二维码数字 -> 出栈颜色顺序(A,B,C,D,E 对应 1~5 位) */
static const uint8_t pop_order_goods[16][TURNTABLE_ITEM_MAX] = {
    /* 1 */ {BLACK, RED,   GREEN, BLUE},
    /* 2 */ {WHITE, BLACK, RED,   GREEN, BLUE},
    /* 3 */ {WHITE, BLACK, GREEN, RED,   BLUE},
    /* 4 */ {BLUE,  WHITE, BLACK, RED,   GREEN},
    /* 5 */ {WHITE, RED,   BLUE,  BLACK, GREEN},
    /* 6 */ {BLACK, RED,   BLUE,  WHITE, GREEN},
    /* 7 */ {BLUE,  GREEN, BLACK, WHITE, RED},
    /* 8 */ {GREEN, WHITE, BLUE,  BLACK, RED},
    /* 9 */ {WHITE, GREEN, BLACK, BLUE,  RED},
    /*10 */ {BLACK, RED,   BLUE,  GREEN, WHITE},
    /*11 */ {RED,   BLUE,  GREEN, BLACK, WHITE},
    /*12 */ {GREEN, RED,   BLACK, WHITE,  BLUE},
    /*13 */ {WHITE, RED,   BLUE,  GREEN, BLACK},
    /*14 */ {RED,   GREEN, WHITE, BLUE,  BLACK},
    /*15 */ {BLUE,  WHITE, GREEN, RED,   BLACK},
    /*16 */ {GREEN, BLUE,  RED,   WHITE, BLACK},
};

/* 奖杯物理入栈顺序(第2次扫码码值 1~6): 第 N 个入栈 -> 槽位 N
 * 码值标签为 ABC/ACB/... , 物理入栈顺序取其逆序:
 *   码1(ABC) -> C 先入槽0, B 槽1, A 槽2 */
static const uint8_t award_push_order[6][LETTER_NUM] = {
    /* 1 ABC */ {C, B, A},
    /* 2 ACB */ {B, C, A},
    /* 3 BAC */ {C, A, B},
    /* 4 BCA */ {A, C, B},
    /* 5 CAB */ {B, A, C},
    /* 6 CBA */ {A, B, C},
};

/* 奖杯出栈顺序: 统一为 CBA, 与扫码值无关 */
static const uint8_t award_order_fixed[LETTER_NUM] = {C, B, A};

static uint8_t s_goods_seq[TURNTABLE_ITEM_MAX];   /* 物料出栈顺序 */
static uint8_t s_award_seq[LETTER_NUM];           /* 奖杯出栈字母顺序(固定 CBA, 仅作显示) */
static uint8_t s_award_slot_seq[LETTER_NUM];      /* 奖杯出栈槽位顺序(由入栈顺序推导) */
static uint8_t s_award_code = 0;                  /* 第2次扫码值 1~6, 用于字母->槽位映射 */
static uint8_t s_step = 0;                        /* 已成功出栈数量 */
static bool    s_goods_seq_set = false;           /* 物料出栈顺序已设置 */
static bool    s_award_seq_set = false;           /* 奖杯出栈顺序已设置 */
static bool    s_award_mode = false;              /* true=奖杯模式, false=物料模式 */
static bool    s_pop_active = false;              /* 出栈会话进行中(已占用转盘) */

/**
 * @brief 获取颜色名称
 */
static const char *pop_color_name(uint8_t color) {
    if (color >= COLOR_NUM) return "Unknown";
    return color_names[color];
}

/**
 * @brief 设置物料出栈顺序
 * @param number 扫码值 1~16
 * @return true 设置成功
 */
bool Turntable_Pop_SetGoodsSequence(uint8_t number) {
    if (number < 1 || number > 16) return false;

    if (s_pop_active) Turntable_Pop_End();   /* 重新设置前释放转盘 */

    memcpy(s_goods_seq, pop_order_goods[number - 1], sizeof(s_goods_seq));
    s_step = 0;
    s_goods_seq_set = true;
    s_award_mode = false;
    logPrintln("Turntable pop: goods sequence set from code %u: %s,%s,%s,%s,%s",
               number,
               pop_color_name(s_goods_seq[0]), pop_color_name(s_goods_seq[1]),
               pop_color_name(s_goods_seq[2]), pop_color_name(s_goods_seq[3]),
               pop_color_name(s_goods_seq[4]));
    return true;
}

/**
 * @brief 设置奖杯栈顺序
 * @param number 第二次扫码值 1~6(决定奖杯入栈顺序)
 * @return true 设置成功
 *
 * 码值标签对应物理入栈顺序(逆序):
 *   1 ABC -> C,B,A    2 ACB -> B,C,A
 *   3 BAC -> C,A,B    4 BCA -> A,C,B
 *   5 CAB -> B,A,C    6 CBA -> A,B,C
 */
bool Turntable_Pop_SetAwardSequence(uint8_t number) {
    if (number < 1 || number > 6) return false;

    if (s_pop_active) Turntable_Pop_End();   /* 重新设置前释放转盘 */

    s_award_code = number;
    memcpy(s_award_seq, award_order_fixed, sizeof(s_award_seq));
    /* 出栈统一 CBA: 依次找 C/B/A 在物理入栈顺序中的槽位(第 N 个入栈 -> 槽位 N) */
    for (uint8_t i = 0; i < LETTER_NUM; i++) {
        uint8_t letter = award_order_fixed[i];   /* C, B, A */
        for (uint8_t j = 0; j < LETTER_NUM; j++) {
            if (award_push_order[number - 1][j] == letter) {
                s_award_slot_seq[i] = j;
                break;
            }
        }
    }
    s_step = 0;
    s_award_seq_set = true;
    s_award_mode = true;
    logPrintln("Turntable pop: award code %u, push order=%d,%d,%d, pop=C,B,A (slots %u,%u,%u)",
               number,
               award_push_order[number - 1][0], award_push_order[number - 1][1],
               award_push_order[number - 1][2],
               s_award_slot_seq[0], s_award_slot_seq[1], s_award_slot_seq[2]);
    return true;
}

/**
 * @brief 在分拣存储数据中查找指定颜色所在的槽位
 * @param color 目标颜色
 * @param slot_id 输出槽位 id
 * @return true 找到
 */
static bool pop_find_slot(uint8_t color, uint8_t *slot_id) {
    uint8_t count = Turntable_Sort_GetCount();
    const TurntableItem_t *items = Turntable_Sort_GetItems();

    for (uint8_t i = 0; i < count; i++) {
        if (items[i].valid &&
            items[i].type == TURNTABLE_ITEM_GOODS &&
            strcmp(items[i].color, pop_color_name(color)) == 0) {
            *slot_id = items[i].id;
            return true;
        }
    }
    return false;
}

/**
 * @brief 查找奖杯槽位
 * @param target_id 奖杯物品 id
 * @return true 找到; false 未找到
 */
static bool pop_find_award_slot(uint8_t target_id) {
    uint8_t count = Turntable_Sort_GetCount();
    const TurntableItem_t *items = Turntable_Sort_GetItems();

    for (uint8_t i = 0; i < count; i++) {
        if (items[i].valid && items[i].type == TURNTABLE_ITEM_AWARD &&
            items[i].id == target_id) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 奖杯字母 -> 物理槽位(由第2次扫码决定的入栈顺序推导)
 * @param letter 字母枚举 A/B/C
 * @return 槽位 id; 扫码值非法或未找到时返回 -1
 */
static int8_t award_slot_of_letter(uint8_t letter) {
    if (s_award_code < 1 || s_award_code > 6) return -1;
    for (uint8_t j = 0; j < LETTER_NUM; j++) {
        if (award_push_order[s_award_code - 1][j] == letter) {
            return (int8_t)j;
        }
    }
    return -1;
}

/**
 * @brief 指定出栈: 按调用方指定的字母出一个物品
 * @param letter 指定字母:
 *               物料模式 'a'~'e' -> pop_order_goods 颜色顺序第 1~5 位;
 *               奖杯模式 'a'~'c' -> 奖杯字符 A~C
 * @return true 本次出栈执行完成; false 顺序未设置、参数非法或目标不存在
 *
 * 取消距离检测: 只旋转到目标槽位即认为出栈成功。
 */
bool Turntable_Pop_StepChar(uint8_t letter) {
    uint8_t index;
    if (letter >= 'a' && letter <= 'e') {
        index = (uint8_t)(letter - 'a');
    } else if (letter >= 'A' && letter <= 'E') {
        index = (uint8_t)(letter - 'A');
    } else {
        logWarning("Turntable pop: invalid letter '%c', use a~e (goods) or a~c (award)", letter);
        return false;
    }

    uint8_t total = s_award_mode ? LETTER_NUM : TURNTABLE_ITEM_MAX;
    if (index >= total) {
        logWarning("Turntable pop: letter '%c' out of range, this mode has %u items",
                   letter, total);
        return false;
    }

    if (s_award_mode) {
        if (!s_award_seq_set) {
            logWarning("Turntable pop award: sequence not set, scan a code first");
            return false;
        }
    } else {
        if (!s_goods_seq_set) {
            logWarning("Turntable pop goods: sequence not set, use `turntable_pop set_goods <N>` first");
            return false;
        }
    }

    /* 首次出栈时占用转盘并暂停入栈任务, 避免两者抢转盘 */
    if (!s_pop_active) {
        Turntable_Sort_Pause();
        s_pop_active = true;
    }

    uint8_t slot_id;
    if (s_award_mode) {
        /* fun(a)~fun(c) 直接指定奖杯字符 A~C, 由扫码入栈顺序推导其所在槽位 */
        uint8_t trophy = (uint8_t)(A + index);   /* A, B, C */
        slot_id = (uint8_t)award_slot_of_letter(trophy);
        logPrintln("Turntable pop award: fun(%c) target trophy=%c slot=%u",
                   letter, (char)('A' + trophy), slot_id);
        if (!pop_find_award_slot(slot_id)) {
            logWarning("Turntable pop award: slot %u no trophy found in storage, skip", slot_id);
            return false;
        }
        /* 旋转转盘, 把目标槽位转到出料口 */
        turntable_move_to_id(slot_id);
        osDelay(TURNTABLE_POP_SETTLE_MS);

        /* 出栈成功, 移除该槽位的分拣记录 */
        Turntable_Sort_Remove(slot_id);
        s_step++;
    } else {
        /* fun(a)~fun(e) 对应 pop_order_goods 颜色顺序的第 1~5 位 */
        uint8_t color = s_goods_seq[index];
        logPrintln("Turntable pop goods: fun(%c) target color=%s",
                   letter, pop_color_name(color));

        /* 1. 查找目标颜色所在的槽位 */
        if (!pop_find_slot(color, &slot_id)) {
            logWarning("Turntable pop goods: color %s not found in storage, skip",
                       pop_color_name(color));
            return false;
        }
        /* 2. 旋转转盘, 把目标槽位转到出料口 */
        turntable_move_to_id(slot_id);
        osDelay(TURNTABLE_POP_SETTLE_MS);

        /* 3. 出栈成功, 移除该槽位的分拣记录 */
        Turntable_Sort_Remove(slot_id);
        s_step++;
    }

    /* 全部出完则释放转盘 */
    if (s_step >= total) {
        Turntable_Pop_End();
    }
    return true;
}

/**
 * @brief 结束出栈会话, 释放转盘并恢复入栈任务
 * @note 全部出完时自动调用; 需要提前中断时可手动调用
 */
void Turntable_Pop_End(void) {
    if (s_pop_active) {
        s_pop_active = false;
        Turntable_Sort_Resume();
        logPrintln("Turntable pop goods: turntable released, sort resumed");
    }
}

/**
 * @brief 打印出栈状态
 */
static void turntable_pop_status(void) {
    logPrintln("Turntable pop status:");
    logPrintln("  mode: %s", s_award_mode ? "Award" : "Goods");
    logPrintln("  popped: %u/%u", s_step, s_award_mode ? LETTER_NUM : TURNTABLE_ITEM_MAX);
    if (s_award_mode) {
        logPrintln("  award: fun(a)=A fun(b)=B fun(c)=C");
        logPrintln("  award slots: A=%d B=%d C=%d",
                   award_slot_of_letter(A), award_slot_of_letter(B), award_slot_of_letter(C));
    } else if (s_goods_seq_set) {
        logPrintln("  goods seq: fun(a)=%s fun(b)=%s fun(c)=%s fun(d)=%s fun(e)=%s",
                   pop_color_name(s_goods_seq[0]), pop_color_name(s_goods_seq[1]),
                   pop_color_name(s_goods_seq[2]), pop_color_name(s_goods_seq[3]),
                   pop_color_name(s_goods_seq[4]));
    } else {
        logPrintln("  goods seq: NOT set");
    }
}

/**
 * @brief 出栈Shell命令
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
static void turntable_pop_Shell(int argc, char *argv[]) {
    if (argc < 2) {
        logPrintln("Usage: turn_pop <set_goods N|set_award N|step <a~e>|end|status>");
        return;
    }

    if (strcmp(argv[1], "set_goods") == 0) {
        if (argc != 3) {
            logPrintln("Usage: turn_pop set_goods <N>");
            return;
        }
        uint8_t n = (uint8_t)atoi(argv[2]);
        if (!Turntable_Pop_SetGoodsSequence(n)) {
            logPrintln("Turntable pop goods: invalid code %u (1~16)", n);
        }
    } else if (strcmp(argv[1], "set_award") == 0) {
        if (argc != 3) {
            logPrintln("Usage: turn_pop set_award <N>");
            return;
        }
        uint8_t n = (uint8_t)atoi(argv[2]);
        if (!Turntable_Pop_SetAwardSequence(n)) {
            logPrintln("Turntable pop award: invalid code %u (1~6)", n);
        }
    } else if (strcmp(argv[1], "step") == 0) {
        if (argc != 3) {
            logPrintln("Usage: turn_pop step <a~e>");
            return;
        }
        Turntable_Pop_StepChar((uint8_t)argv[2][0]);
    } else if (strcmp(argv[1], "end") == 0) {
        Turntable_Pop_End();
    } else if (strcmp(argv[1], "status") == 0) {
        turntable_pop_status();
    } else {
        logPrintln("Turntable pop goods: invalid command: %s", argv[1]);
    }
}

/**
 * @brief 导出出栈Shell命令
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    turn_pop, turntable_pop_Shell, turntable pop commands);
