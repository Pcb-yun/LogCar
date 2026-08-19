/**
 * @file turntable_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 转盘总控源文件
 */

#include "turntable_port.h"
#include "turntable_ctrl.h"
#include "vl53l0x_port.h"
#include "rpi_sensor_port.h"
#include "task.h"
#include "Events.h"
#include "mission.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "log.h"
#include <stdlib.h>

static TropLabel_t set_trop(void);

static TurntableSTO_t *g_sto[TURNTABLE_STO_NUM];
static TurntablePort_t g_port;
static bool is_init = false;

/* 二维码物料映射表 */
static const SENSOR_Color_t matl_order[16][5] = {
    /*             A            B            C            D            E     */
    /* 01 */ {COLOR_BLACK, COLOR_WHITE, COLOR_RED,   COLOR_GREEN, COLOR_BLUE},
    /* 02 */ {COLOR_WHITE, COLOR_BLACK, COLOR_RED,   COLOR_GREEN, COLOR_BLUE},
    /* 03 */ {COLOR_WHITE, COLOR_BLACK, COLOR_GREEN, COLOR_RED,   COLOR_BLUE},
    /* 04 */ {COLOR_BLUE,  COLOR_WHITE, COLOR_BLACK, COLOR_RED,   COLOR_GREEN},
    /* 05 */ {COLOR_WHITE, COLOR_RED,   COLOR_BLUE,  COLOR_BLACK, COLOR_GREEN},
    /* 06 */ {COLOR_BLACK, COLOR_RED,   COLOR_BLUE,  COLOR_WHITE, COLOR_GREEN},
    /* 07 */ {COLOR_BLUE,  COLOR_GREEN, COLOR_BLACK, COLOR_WHITE, COLOR_RED},
    /* 08 */ {COLOR_GREEN, COLOR_WHITE, COLOR_BLUE,  COLOR_BLACK, COLOR_RED},
    /* 09 */ {COLOR_WHITE, COLOR_GREEN, COLOR_BLACK, COLOR_BLUE,  COLOR_RED},
    /* 10 */ {COLOR_BLACK, COLOR_RED,   COLOR_BLUE,  COLOR_GREEN, COLOR_WHITE},
    /* 11 */ {COLOR_RED,   COLOR_BLUE,  COLOR_GREEN, COLOR_BLACK, COLOR_WHITE},
    /* 12 */ {COLOR_GREEN, COLOR_RED,   COLOR_BLACK, COLOR_WHITE, COLOR_BLUE},
    /* 13 */ {COLOR_WHITE, COLOR_RED,   COLOR_BLUE,  COLOR_GREEN, COLOR_BLACK},
    /* 14 */ {COLOR_RED,   COLOR_GREEN, COLOR_WHITE, COLOR_BLUE,  COLOR_BLACK},
    /* 15 */ {COLOR_BLUE,  COLOR_WHITE, COLOR_GREEN, COLOR_RED,   COLOR_BLACK},
    /* 16 */ {COLOR_GREEN, COLOR_BLUE,  COLOR_RED,   COLOR_WHITE, COLOR_BLACK}
};

/* 二维码奖杯映射表 */
static const TropLabel_t trop_order[6][3] = {
    /* 1 ABC */ {LABEL_A, LABEL_B, LABEL_C},
    /* 2 ACB */ {LABEL_A, LABEL_C, LABEL_B},
    /* 3 BAC */ {LABEL_B, LABEL_A, LABEL_C},
    /* 4 BCA */ {LABEL_B, LABEL_C, LABEL_A},
    /* 5 CAB */ {LABEL_C, LABEL_A, LABEL_B},
    /* 6 CBA */ {LABEL_C, LABEL_B, LABEL_A},
};

static const TropLabel_t trop_pop[4] = {LABEL_NONE, LABEL_B, LABEL_A, LABEL_C};

/**
 * @brief 初始化转盘入库信息
 * @param sto 转盘入库信息指针
 */
bool Turntable_Port_Init(void) {
    for (int i = 0; i < TURNTABLE_STO_NUM; i++) {
        g_sto[i] = pvPortMalloc(sizeof(TurntableSTO_t));
        if (!g_sto[i]) {
            for (uint8_t j = 0; j < i; j++) {
                vPortFree(g_sto[j]);
            }
            return false;
        }
    }
    for (int i = 0; i < TURNTABLE_STO_NUM; i++) {
        g_sto[i]->id = 0;
        g_sto[i]->color = COLOR_UNKNOWN;
        g_sto[i]->label = LABEL_NONE;
    }
    g_port.type = TURNTABLE_MATL;
    g_port.order = 0;
    is_init = true;
    return is_init;
}

/**
 * @brief 转盘自动入库任务
 * @param argument 任务参数
 */
void Turntable_Port_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsNoClear, osWaitForever);
    osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
    if (!is_init) vTaskDelete(NULL);
    uint8_t sto_idx = 0;

    for(;;) {
        osDelay(TURNTABLE_POLL_INTERVAL_MS);
        osEventFlagsWait(System_StatusHandle, TURNTABLE_RUN, osFlagsNoClear, osWaitForever);
        uint16_t dist = Dist_Get();
        if (dist == 0 || dist > TURNTABLE_DIST_THR) continue;

        g_sto[sto_idx]->id = turntable_get_id();
        turntable_move_to_next(MISSION_COLOR_SENSOR);

        osDelay(TURNTABLE_SETTLE_MS);

        if (g_port.type == TURNTABLE_MATL) {
#if MISSION_COLOR_SENSOR == 0 // 颜色传感器
            g_sto[sto_idx]->color = SENSOR_DetectColor();
#else // 树莓派通讯
            g_sto[sto_idx]->color = RPI_DetectColor();
#endif
            logInfo("sto_idx: %d, id: %d, color: %s", sto_idx, g_sto[sto_idx]->id, matl_str[g_sto[sto_idx]->color]);
            if (sto_idx == 4) goto cplt;
        } else if (g_port.type == TURNTABLE_TROP) {
            g_sto[sto_idx]->label = set_trop();
            logInfo("sto_idx: %d, id: %d, label: %s", sto_idx, g_sto[sto_idx]->id, trop_str[g_sto[sto_idx]->label]);
            if (sto_idx == 2) goto cplt;
        } else {
            logWarning("unknown type"); continue;
        }
        sto_idx++; continue;

    cplt:
        sto_idx = 0;
        osEventFlagsSet(System_StatusHandle, TURNTABLE_CPLT);
        osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
        turntable_move_to_close();
    }
}

/**
 * @brief 转盘出库
 * @param pop 出库类型
 * @return 出库状态
 */
bool Turntable_Pop(TurntablePop_t pop) {
	uint8_t pop_idx = pop;

	if (g_port.type == TURNTABLE_MATL) {
		SENSOR_Color_t target_color = matl_order[g_port.order][pop_idx];
		for (uint8_t i = 0; i < TURNTABLE_STO_NUM; i++) {
			if (g_sto[i]->color == target_color) {
				uint8_t out_id = g_sto[i]->id;
				logInfo("sto_idx=%d, id=%d, target_color=%s, order=%d",
				           i, out_id, matl_str[target_color], g_port.order + 1);
				turntable_move_to_id(out_id);
				g_sto[i]->color = COLOR_UNKNOWN;
				return true;
			}
		}
		return false;
	} else if (g_port.type == TURNTABLE_TROP) {
		TropLabel_t target_label = trop_pop[pop_idx];
		for (uint8_t i = 0; i < TURNTABLE_STO_NUM; i++) {
			if (g_sto[i]->label == target_label) {
				uint8_t out_id = g_sto[i]->id;
				logInfo("sto_idx=%d, id=%d, target_label=%s, order=%d",
				           i, out_id, trop_str[target_label], g_port.order + 1);
                turntable_move_to_id(out_id);
                // turntable_move_to_int(out_id, 500);
				g_sto[i]->label = LABEL_NONE;
				return true;
			}
		}
		return false;
	} else {
		logWarning("unknown type");
		return false;
	}
}

/**
 * @brief 设置映射表
 * @param order 映射表索引
 */
void Turntable_SetOrder(uint8_t order) {
    g_port.order = order - 1;
}

/**
 * @brief 设置转盘存放类型
 * @param type 存放类型
 */
void Turntable_Port_SetType(TurntableType_t type) {
    g_port.type = type;
}

/**
 * @brief 获取转盘存放类型
 * @return 存放类型
 */
TurntableType_t Turntable_Port_GetType(void) {
    return g_port.type;
}

/**
 * @brief 设置奖杯标签
 * @return 奖杯标签
 */
static TropLabel_t set_trop(void) {
	static int8_t count = 2;
	TropLabel_t label = trop_order[g_port.order][count];
	if (count <= 0) count = 2;
	else count--;
	return label;
}

/**
 * @brief 设置自动入库
 */
static void set_run(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: %s <run>", argv[0]);
        return;
    }

    if (strcmp(argv[1], "run") == 0){
        osEventFlagsSet(System_StatusHandle, TURNTABLE_RUN);
        logPrintln("Set run");
    } else {
        osEventFlagsClear(System_StatusHandle, TURNTABLE_RUN);
        logPrintln("Clear run");
    }
}

/**
 * @brief 转盘出库
 */
static void pop_test(int argc, char *argv[]) {
	if (argc != 2) {
		logPrintln("Usage: %s <pop_idx>", argv[0]);
		logPrintln("  MATL: 0=A, 1=B, 2=C, 3=D, 4=E  (set order first: 1-16)");
		logPrintln("  TROP: 2=A, 1=B, 3=C           (set order first: 1-6)");
		return;
	}

	uint8_t pop_idx = atoi(argv[1]);
	if (pop_idx > 4) {
		logPrintln("Invalid pop_idx: %d (range: 0-4)", pop_idx);
		return;
	}

	bool ret = Turntable_Pop((TurntablePop_t)pop_idx);
	if (!ret) {
		logPrintln("Pop failed: idx=%d, target not found (check order & sto content)", pop_idx);
	}
}

static void set_order(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: %s <order>", argv[0]);
        logPrintln("  MATL type: order=1~16, TROP type: order=1~6");
        return;
    }

    uint8_t order = atoi(argv[1]);
    if (g_port.type == TURNTABLE_MATL) {
        if (order < 1 || order > 16) {
            logPrintln("Invalid order: %d (MATL range: 1-16)", order);
            return;
        }
    } else if (g_port.type == TURNTABLE_TROP) {
        if (order < 1 || order > 6) {
            logPrintln("Invalid order: %d (TROP range: 1-6)", order);
            return;
        }
    } else {
        logPrintln("Please set type first (MATL/TROP)");
        return;
    }

    Turntable_SetOrder(order);
    logPrintln("Set order: %d", order);
}

static void set_type(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: %s <type>", argv[0]);
        logPrintln("  type: MATL/TROP");
        return;
    }

    if (strcmp(argv[1], "MATL") == 0) {
        Turntable_Port_SetType(TURNTABLE_MATL);
        logPrintln("Set type: MATL");
    } else if (strcmp(argv[1], "TROP") == 0) {
        Turntable_Port_SetType(TURNTABLE_TROP);
        logPrintln("Set type: TROP");
    } else {
        logPrintln("Invalid type: %s", argv[1]);
    }
}

/**
 * @brief 查看入库物料
 */
static void view_sto(void) {
    for (int i = 0; i < TURNTABLE_STO_NUM; i++) {
        logInfo("sto_idx: %d, id: %d, color: %s, label: %s", i, g_sto[i]->id, matl_str[g_sto[i]->color], trop_str[g_sto[i]->label]);
    }
}

ShellCommand TURNroup[] = {
        SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_FUNC|SHELL_CMD_DISABLE_RETURN, view, view_sto, view sto),
        SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, pop, pop_test, turntable pop),
        SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, run, set_run, change automatic inventory entry),
        SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, order, set_order, set order index),
        SHELL_CMD_GROUP_ITEM(SHELL_TYPE_CMD_MAIN|SHELL_CMD_DISABLE_RETURN, type, set_type, set type),
        SHELL_CMD_GROUP_END()
};
SHELL_EXPORT_CMD_GROUP(SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
turn, TURNroup, turntable Tool Group);
