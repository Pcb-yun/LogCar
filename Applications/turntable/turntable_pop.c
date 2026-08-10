/**
 * @file turntable_pop.c
 * @brief 转盘出栈程序(物料搬运 / 奖杯搬运)
 *
 * 统一 pop 接口 Turntable_Pop_Execute():
 *   - 参数为字符串字母("a"~"e"/"A"~"E")或数字("1"~"5")
 *   - 根据参数类型返回不同的取放指导信息:
 *       物料模式: 字母 a~e 或数字 1~5 -> 目标点位 A~E, 返回该点位需放置的
 *                 颜色及其所在槽位, 并旋转转盘将目标槽位转到出料口
 *       奖杯模式: 字母 a~c 直接指定奖杯 A~C; 数字 1~3 指定数字点位,
 *                 返回该点位放置的奖杯字母及其槽位, 并旋转转盘出栈
 *
 * 出栈方式为"指定出栈": 扫码后由调用方指定目标点位, 每次出一个物品:
 *   - 第1次扫码(物料):
 *     颜色顺序由 pop_order_goods 表决定(扫码值 1~16)
 *   - 第2次扫码(奖杯):
 *     奖杯初始位置由 award_position 表决定(扫码值 1~6),
 *     物理入栈顺序取其逆序, 出栈时按字母推导槽位
 *
 * 与入栈任务的互斥:
 *   首次出栈自动调用 Turntable_Sort_Pause() 暂停入栈并等待其当前动作结束,
 *   出完一轮或调用 Turntable_Pop_End() 后恢复入栈, 避免两者同时驱动转盘。
 *
 * 用法(Shell):
 *   turn_pop set_goods <N>  设置物料出栈顺序(扫码值 1~16)
 *   turn_pop set_award <N>  设置奖杯模式(扫码值 1~6)
 *   turn_pop step <a~e|1~5> 统一接口: 指定出栈一个物品
 *   turn_pop end            提前结束出栈会话, 恢复入栈
 *   turn_pop status         查看当前顺序与进度
 */

#include "turntable_pop.h"
#include "turntable_sort.h"
#include "turntable_ctrl.h"
#include "shell.h"
#include "log.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* 颜色名称(与 SENSOR_InferColor 输出一致) */
static const char *const color_names[COLOR_NUM] = {
    "Black", "White", "Red", "Green", "Blue"
};

/* 颁奖台位置名称(奖杯 A->冠军, B->亚军, C->季军) */
static const char *const podium_names[LETTER_NUM] = {
    "Champion", "Runner-up", "Third"
};

/* 二维码数字(1~16) -> 出栈颜色顺序(A,B,C,D,E 对应 1~5 位) */
static const uint8_t pop_order_goods[16][TURNTABLE_ITEM_MAX] = {
    /* 1 */ {BLACK, WHITE, RED,   GREEN, BLUE},
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
    /*12 */ {GREEN, RED,   BLACK, WHITE, BLUE},
    /*13 */ {WHITE, RED,   BLUE,  GREEN, BLACK},
    /*14 */ {RED,   GREEN, WHITE, BLUE,  BLACK},
    /*15 */ {BLUE,  WHITE, GREEN, RED,   BLACK},
    /*16 */ {GREEN, BLUE,  RED,   WHITE, BLACK},
};

/* 奖杯初始位置映射表(第2次扫码码值 1~6):
 * 第 N 行第 i 列 = 数字点位 i+1 放置的奖杯字母
 *   1 ABC -> 点位1=A 点位2=B 点位3=C
 *   2 ACB -> 点位1=A 点位2=C 点位3=B
 *   3 BAC -> 点位1=B 点位2=A 点位3=C
 *   4 BCA -> 点位1=B 点位2=C 点位3=A
 *   5 CAB -> 点位1=C 点位2=A 点位3=B
 *   6 CBA -> 点位1=C 点位2=B 点位3=A
 * 奖杯物理入栈顺序取其逆序(第 N 个入栈 -> 槽位 N):
 *   码1(ABC) -> C 先入槽0, B 槽1, A 槽2 */
static const uint8_t award_position[6][LETTER_NUM] = {
    /* 1 ABC */ {TROPHY_A, TROPHY_B, TROPHY_C},
    /* 2 ACB */ {TROPHY_A, TROPHY_C, TROPHY_B},
    /* 3 BAC */ {TROPHY_B, TROPHY_A, TROPHY_C},
    /* 4 BCA */ {TROPHY_B, TROPHY_C, TROPHY_A},
    /* 5 CAB */ {TROPHY_C, TROPHY_A, TROPHY_B},
    /* 6 CBA */ {TROPHY_C, TROPHY_B, TROPHY_A},
};

static uint8_t s_goods_seq[TURNTABLE_ITEM_MAX];   /* 物料出栈顺序 */
static uint8_t s_award_seq[LETTER_NUM];           /* 奖杯出栈字母顺序(显示用) */
static uint8_t s_award_code = 0;                  /* 第2次扫码值 1~6 */
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
 * @brief 设置奖杯模式
 * @param number 第二次扫码值 1~6(决定奖杯初始位置映射)
 * @return true 设置成功
 */
bool Turntable_Pop_SetAwardSequence(uint8_t number) {
    if (number < 1 || number > 6) return false;

    if (s_pop_active) Turntable_Pop_End();   /* 重新设置前释放转盘 */

    s_award_code = number;
    /* 出栈字母顺序(显示用): 按点位 1~3 对应标签, 即扫码标签正序 */
    memcpy(s_award_seq, award_position[number - 1], sizeof(s_award_seq));
    s_step = 0;
    s_award_seq_set = true;
    s_award_mode = true;
    logPrintln("Turntable pop: award code %u, position=%c%c%c, push order=%c,%c,%c",
               number,
               (char)('A' + s_award_seq[0]), (char)('A' + s_award_seq[1]),
               (char)('A' + s_award_seq[2]),
               (char)('A' + award_position[number - 1][2]),
               (char)('A' + award_position[number - 1][1]),
               (char)('A' + award_position[number - 1][0]));
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
 * @brief 在分拣存储数据中查找奖杯槽位是否已入栈
 * @param target_id 目标槽位 id
 * @return true 找到
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
 *
 * 物理入栈顺序 = 标签逆序(见 award_position 表注释),
 * 因此字母在表行中的下标 = 槽位 id。
 */
static int8_t award_slot_of_letter(uint8_t letter) {
    if (s_award_code < 1 || s_award_code > 6) return -1;
    for (uint8_t j = 0; j < LETTER_NUM; j++) {
        if (award_position[s_award_code - 1][j] == letter) {
            return (int8_t)(LETTER_NUM - 1 - j);
        }
    }
    return -1;
}

/**
 * @brief 统一出栈接口: 根据调用参数类型(字符串或整数)返回不同信息
 * @param arg 参数:
 *             字符串字母: "a"~"e"/"A"~"E"
 *               物料模式: 目标点位 A~E
 *               奖杯模式: 目标奖杯字母 A~C
 *             数字: "1"~"5"
 *               物料模式: 目标点位 1~5(等价 A~E)
 *               奖杯模式: 目标数字点位 1~3
 * @return 取放指导信息
 *
 * 每次调用只出一个物品:
 *   1. 解析目标(物料: 颜色; 奖杯: 字母)
 *   2. 在分拣存储数据中找到目标槽位并旋转转盘到出料口
 *   3. 返回包含目标槽位、物品信息与取放动作描述的结果
 */
Turntable_PopResult_t Turntable_Pop_Execute(const char *arg) {
    Turntable_PopResult_t res;
    memset(&res, 0, sizeof(res));

    if (arg == NULL || arg[0] == '\0') return res;

    char c = arg[0];
    uint8_t index;

    /* 解析参数: 字母 a~e/A~E 或数字 1~5 */
    if (c >= 'a' && c <= 'e') {
        index = (uint8_t)(c - 'a');
    } else if (c >= 'A' && c <= 'E') {
        index = (uint8_t)(c - 'A');
    } else if (c >= '1' && c <= '5') {
        index = (uint8_t)(c - '1');
    } else {
        logWarning("Turntable pop: invalid arg '%s', use a~e (goods) or 1~5 (award)",
                   arg);
        return res;
    }

    if (s_award_mode) {
        /* ---- 奖杯模式 ---- */
        if (!s_award_seq_set) {
            logWarning("Turntable pop award: sequence not set, scan a code first");
            return res;
        }
        uint8_t total = LETTER_NUM;
        if (index >= total) {
            logWarning("Turntable pop: arg '%s' out of range, award mode has %u items",
                       arg, total);
            return res;
        }

        /* 首次出栈时占用转盘并暂停入栈任务 */
        if (!s_pop_active) {
            Turntable_Sort_Pause();
            s_pop_active = true;
        }

        /* 数字 1~3: 直接表示数字点位; 字母 a~c: 奖杯字母 A~C, 同一索引映射 */
        uint8_t trophy = (uint8_t)(TROPHY_A + index);   /* A, B, C */
        int8_t slot_id = award_slot_of_letter(trophy);
        if (slot_id < 0) {
            logWarning("Turntable pop award: trophy %c slot unknown, skip",
                       (char)('A' + trophy));
            return res;
        }
        res.mode   = 1;
        res.target = (char)('1' + index);           /* 数字点位 1~3 */
        res.trophy = trophy;
        res.slot_id = (uint8_t)slot_id;
        snprintf(res.action, sizeof(res.action),
                 "pick trophy %c at slot %d, place to %s",
                 (char)('A' + trophy), slot_id, podium_names[trophy]);
        logPrintln("Turntable pop award: target trophy=%c slot=%d -> %s",
                   (char)('A' + trophy), slot_id, podium_names[trophy]);

        if (!pop_find_award_slot((uint8_t)slot_id)) {
            logWarning("Turntable pop award: slot %d no trophy in storage, skip", slot_id);
            return res;
        }
        turntable_move_to_id((uint8_t)slot_id);
        osDelay(TURNTABLE_SETTLE_MS);

        Turntable_Sort_Remove((uint8_t)slot_id);
        s_step++;
        res.valid = true;

        if (s_step >= total) Turntable_Pop_End();
        return res;
    }

    /* ---- 物料模式 ---- */
    if (!s_goods_seq_set) {
        logWarning("Turntable pop goods: sequence not set, use `turn_pop set_goods <N>` first");
        return res;
    }
    if (index >= TURNTABLE_ITEM_MAX) return res;

    if (!s_pop_active) {
        Turntable_Sort_Pause();
        s_pop_active = true;
    }

    /* 目标点位 A~E 需放置的颜色 */
    uint8_t color = s_goods_seq[index];
    uint8_t slot_id;

    res.mode   = 0;
    res.target = (char)('A' + index);
    res.color  = color;
    strncpy(res.color_name, pop_color_name(color), sizeof(res.color_name) - 1);
    res.color_name[sizeof(res.color_name) - 1] = '\0';
    snprintf(res.action, sizeof(res.action),
             "pick %s at slot ?, place to point %c",
             pop_color_name(color), res.target);
    logPrintln("Turntable pop goods: target point %c color=%s",
               res.target, pop_color_name(color));

    /* 查找目标颜色所在的槽位 */
    if (!pop_find_slot(color, &slot_id)) {
        logWarning("Turntable pop goods: color %s not found in storage, skip",
                   pop_color_name(color));
        return res;
    }
    res.slot_id = slot_id;
    snprintf(res.action, sizeof(res.action),
             "pick %s at slot %u, place to point %c",
             pop_color_name(color), slot_id, res.target);

    /* 旋转转盘, 把目标槽位转到出料口 */
    turntable_move_to_id(slot_id);
    osDelay(TURNTABLE_SETTLE_MS);

    /* 出栈成功, 移除该槽位的分拣记录 */
    Turntable_Sort_Remove(slot_id);
    s_step++;
    res.valid = true;

    if (s_step >= TURNTABLE_ITEM_MAX) Turntable_Pop_End();
    return res;
}

/**
 * @brief 指定出栈: 按调用方指定的字母出一个物品(兼容旧接口)
 * @param letter 指定字母:
 *               物料模式 'a'~'e' -> pop_order_goods 颜色顺序第 1~5 位;
 *               奖杯模式 'a'~'c' -> 奖杯字符 A~C
 * @return true 本次出栈执行完成; false 顺序未设置、参数非法或目标不存在
 */
bool Turntable_Pop_StepChar(uint8_t letter) {
    char arg[2] = { (char)letter, '\0' };
    Turntable_PopResult_t res = Turntable_Pop_Execute(arg);
    return res.valid;
}

/**
 * @brief 获取当前已成功出栈的数量
 */
uint8_t Turntable_Pop_GetStep(void) {
    return s_step;
}

/**
 * @brief 获取物料出栈顺序(长度为 TURNTABLE_ITEM_MAX 的颜色序列)
 */
const uint8_t *Turntable_Pop_GetGoodsSequence(void) {
    return s_goods_seq;
}

/**
 * @brief 获取奖杯出栈顺序(长度为 LETTER_NUM 的字母序列)
 */
const uint8_t *Turntable_Pop_GetAwardSequence(void) {
    return s_award_seq;
}

/**
 * @brief 结束出栈会话, 释放转盘并恢复入栈任务
 * @note 全部出完时自动调用; 需要提前中断时可手动调用
 */
void Turntable_Pop_End(void) {
    if (s_pop_active) {
        s_pop_active = false;
        Turntable_Sort_Resume();
        logPrintln("Turntable pop: turntable released, sort resumed");
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
        logPrintln("  award: point1=%c point2=%c point3=%c",
                   (char)('A' + s_award_seq[0]), (char)('A' + s_award_seq[1]),
                   (char)('A' + s_award_seq[2]));
        logPrintln("  award slots: A=%d B=%d C=%d",
                   award_slot_of_letter(TROPHY_A), award_slot_of_letter(TROPHY_B),
                   award_slot_of_letter(TROPHY_C));
    } else if (s_goods_seq_set) {
        logPrintln("  goods seq: A=%s B=%s C=%s D=%s E=%s",
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
        logPrintln("Usage: turn_pop <set_goods N|set_award N|step <a~e|1~5>|end|status>");
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
            logPrintln("Usage: turn_pop step <a~e|1~5>");
            return;
        }
        Turntable_PopResult_t res = Turntable_Pop_Execute(argv[2]);
        if (res.valid) {
            logPrintln("Turntable pop: %s", res.action);
        }
    } else if (strcmp(argv[1], "end") == 0) {
        Turntable_Pop_End();
    } else if (strcmp(argv[1], "status") == 0) {
        turntable_pop_status();
    } else {
        logPrintln("Turntable pop: invalid command: %s", argv[1]);
    }
}

/**
 * @brief 导出出栈Shell命令
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    turn_pop, turntable_pop_Shell, turntable pop commands);
