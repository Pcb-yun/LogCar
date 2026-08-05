/**
 * @file turntable_pop.c
 * @brief 转盘出栈程序
 *
 * 出栈顺序由扫码模块读取的二维码数字(1~16)决定, 映射表见 pop_order:
 *   第 N 行的 A,B,C,D,E 依次为第 1~5 个要出栈的颜色(黑/白/红/绿/蓝)
 * 每调用一次 Turntable_Pop_Step() 出一个物料:
 *   1. 从分拣存储数据中找到该颜色所在的槽位
 *   2. 旋转转盘把该槽位转到出料口
 *   3. 通过 VL53L0X 检测物料是否已离开转盘
 *   4. 出栈成功后移除该槽位的分拣记录
 *
 * 与入栈任务的互斥:
 *   首次出栈自动调用 Turntable_Sort_Pause() 暂停入栈并等待其当前动作结束,
 *   出完一轮或调用 Turntable_Pop_End() 后恢复入栈, 避免两者同时驱动转盘。
 *
 * 用法(Shell):
 *   turntable_pop set <N>   设置出栈顺序(扫码值 1~16)
 *   turntable_pop step      出栈一步(出一个物料)
 *   turntable_pop end       提前结束出栈会话, 恢复入栈
 *   turntable_pop status    查看当前顺序与进度
 */

#include "turntable_pop.h"
#include "turntable_sort.h"
#include "turntable_ctrl.h"
#include "shell.h"
#include "log.h"
#include "cmsis_os2.h"
#include "vl53l0x_port.h"
#include <string.h>
#include <stdlib.h>

/* 颜色名称(与 SENSOR_InferColor 输出一致) */
static const char *const color_names[TURNTABLE_COLOR_NUM] = {
    "Black", "White", "Red", "Green", "Blue"
};

/* 二维码数字 -> 出栈颜色顺序(A,B,C,D,E 对应 1~5 位) */
static const uint8_t pop_order[16][TURNTABLE_ITEM_MAX] = {
    /* 1 */ {TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_BLUE},
    /* 2 */ {TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_BLUE},
    /* 3 */ {TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLUE},
    /* 4 */ {TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_GREEN},
    /* 5 */ {TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_GREEN},
    /* 6 */ {TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_GREEN},
    /* 7 */ {TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_RED},
    /* 8 */ {TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_RED},
    /* 9 */ {TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_RED},
    /*10 */ {TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_WHITE},
    /*11 */ {TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_WHITE},
    /*12 */ {TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLACK, TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_WHITE},
    /*13 */ {TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_BLACK},
    /*14 */ {TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_BLACK},
    /*15 */ {TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_BLACK},
    /*16 */ {TURNTABLE_COLOR_GREEN, TURNTABLE_COLOR_BLUE,  TURNTABLE_COLOR_RED,   TURNTABLE_COLOR_WHITE, TURNTABLE_COLOR_BLACK},
};

/* 当前出栈状态 */
static uint8_t s_seq[TURNTABLE_ITEM_MAX];
static uint8_t s_step = 0;
static bool    s_seq_set = false;
static bool    s_pop_active = false;   /* 出栈会话进行中(已占用转盘) */

static const char *pop_color_name(uint8_t color) {
    if (color >= TURNTABLE_COLOR_NUM) return "Unknown";
    return color_names[color];
}

/**
 * @brief 设置出栈顺序
 * @param number 扫码值 1~16
 * @return true 设置成功
 */
bool Turntable_Pop_SetSequence(uint8_t number) {
    if (number < 1 || number > 16) return false;

    if (s_pop_active) Turntable_Pop_End();   /* 重新设置前释放转盘 */

    memcpy(s_seq, pop_order[number - 1], sizeof(s_seq));
    s_step = 0;
    s_seq_set = true;
    logPrintln("Turntable pop: sequence set from code %u: %s,%s,%s,%s,%s",
               number,
               pop_color_name(s_seq[0]), pop_color_name(s_seq[1]),
               pop_color_name(s_seq[2]), pop_color_name(s_seq[3]),
               pop_color_name(s_seq[4]));
    return true;
}

/**
 * @brief 获取当前出栈进度
 */
uint8_t Turntable_Pop_GetStep(void) {
    return s_step;
}

/**
 * @brief 获取当前出栈顺序
 */
const uint8_t *Turntable_Pop_GetSequence(void) {
    return s_seq;
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
 * @brief 等待物料离开转盘
 * @return true 物料已离开; false 超时失败
 *
 * 必须先确认物料到达出料口(VL53L0X 距离变短), 再等待物料离开(距离恢复)。
 * 阶段1 超时直接判失败, 避免"没出料却误报成功"。
 */
static bool pop_wait_material_out(void) {
    uint32_t t0;

    /* 阶段1: 等待物料到达出料口(距离变短) */
    t0 = osKernelGetTickCount();
    while ((osKernelGetTickCount() - t0) < TURNTABLE_POP_TIMEOUT_MS) {
        osDelay(TURNTABLE_POP_POLL_MS);
        uint16_t d = Dist_Get();
        if (d > 0 && d < TURNTABLE_DIST_GOODS_THRESHOLD_MM) break;
    }
    if ((osKernelGetTickCount() - t0) >= TURNTABLE_POP_TIMEOUT_MS) {
        logWarning("Turntable pop: material never reached the port");
        return false;
    }

    /* 阶段2: 等待物料离开出料口(距离恢复) */
    t0 = osKernelGetTickCount();
    while ((osKernelGetTickCount() - t0) < TURNTABLE_POP_TIMEOUT_MS) {
        osDelay(TURNTABLE_POP_POLL_MS);
        uint16_t d = Dist_Get();
        if (d == 0 || d >= TURNTABLE_DIST_GOODS_THRESHOLD_MM) {
            logPrintln("Turntable pop: material left the turntable");
            return true;
        }
    }
    logWarning("Turntable pop: timeout waiting material to leave");
    return false;
}

/**
 * @brief 出栈一步: 按当前顺序出掉下一个颜色的物料
 * @return true 执行完成; false 顺序未设置或已出完
 */
bool Turntable_Pop_Step(void) {
    if (!s_seq_set) {
        logWarning("Turntable pop: sequence not set, use `turntable_pop set <N>` first");
        return false;
    }
    if (s_step >= TURNTABLE_ITEM_MAX) {
        logPrintln("Turntable pop: all %u materials popped", TURNTABLE_ITEM_MAX);
        return false;
    }

    /* 首次出栈时占用转盘并暂停入栈任务, 避免两者抢转盘/抢检测 */
    if (!s_pop_active) {
        Turntable_Sort_Pause();
        s_pop_active = true;
    }

    uint8_t color = s_seq[s_step];
    logPrintln("Turntable pop: step %u/%u target color=%s",
               s_step + 1, TURNTABLE_ITEM_MAX, pop_color_name(color));

    /* 1. 查找目标颜色所在的槽位 */
    uint8_t slot_id;
    if (!pop_find_slot(color, &slot_id)) {
        logWarning("Turntable pop: color %s not found in storage, skip",
                   pop_color_name(color));
        s_step++;
    } else {
        /* 2. 旋转转盘, 把目标槽位转到出料口 */
        turntable_move_to_id(slot_id);
        osDelay(TURNTABLE_POP_SETTLE_MS);

        /* 3. 检测物料是否已离开转盘 */
        if (!pop_wait_material_out()) {
            logWarning("Turntable pop: step %u failed, slot %u still occupied",
                       s_step + 1, slot_id);
        } else {
            /* 4. 出栈成功, 移除该槽位的分拣记录 */
            Turntable_Sort_Remove(slot_id);
            s_step++;
        }
    }

    /* 全部出完则释放转盘 */
    if (s_step >= TURNTABLE_ITEM_MAX) {
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
        logPrintln("Turntable pop: turntable released, sort resumed");
    }
}

/**
 * @brief 打印出栈状态
 */
static void turntable_pop_status(void) {
    logPrintln("Turntable pop status:");
    logPrintln("  sequence: %s", s_seq_set ? "set" : "NOT set");
    logPrintln("  step: %u/%u", s_step, TURNTABLE_ITEM_MAX);
    if (s_seq_set) {
        logPrintln("  order: %s,%s,%s,%s,%s",
                   pop_color_name(s_seq[0]), pop_color_name(s_seq[1]),
                   pop_color_name(s_seq[2]), pop_color_name(s_seq[3]),
                   pop_color_name(s_seq[4]));
    }
}

/**
 * @brief 出栈Shell命令
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
static void turntable_pop_Shell(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        logPrintln("Usage: turn_pop <set N|step|end|status>");
        return;
    }

    if (strcmp(argv[1], "set") == 0) {
        if (argc != 3) {
            logPrintln("Usage: turn_pop set <N>");
            return;
        }
        uint8_t n = (uint8_t)atoi(argv[2]);
        if (!Turntable_Pop_SetSequence(n)) {
            logPrintln("turn_pop: invalid code %u (1~16)", n);
        }
    } else if (strcmp(argv[1], "step") == 0) {
        Turntable_Pop_Step();
    } else if (strcmp(argv[1], "end") == 0) {
        Turntable_Pop_End();
    } else if (strcmp(argv[1], "status") == 0) {
        turntable_pop_status();
    } else {
        logPrintln("turn_pop: invalid command: %s", argv[1]);
    }
}

/**
 * @brief 导出出栈Shell命令
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    turn_pop, turntable_pop_Shell, turntable pop commands);
