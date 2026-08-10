/**
 * @file turntable_port.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 转盘总控源文件
 */

#include "turntable_port.h"
#include "turntable_ctrl.h"
#include "vl53l0x_port.h"
#include "task.h"
#include "Events.h"
#include "mission.h"
#include "shell.h"
#include "shell_cmd_group.h"
#include "log.h"

static TropLabel_t set_trop();

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
        osDelay(TURNTABLE_SETTLE_MS);

        g_sto[sto_idx]->id = turntable_get_id();
        turntable_move_to_next(MISSION_COLOR_SENSOR);

        while(dist < TURNTABLE_DIST_THR) {
            dist = Dist_Get();
            osDelay(1);
        }

        if (g_port.type == TURNTABLE_MATL) {
#if MISSION_COLOR_SENSOR == 0 // 颜色传感器
            g_sto[sto_idx]->color = SENSOR_DetectColor();
#else // 树莓派通讯
            g_sto[sto_idx]->color = RPI_DetectColor();
#endif
            logInfo("sto_idx: %d, id: %d, color: %s", sto_idx, g_sto[sto_idx]->id, matl_str[g_sto[sto_idx]->color]);
        } else if (g_port.type == TURNTABLE_TROP) {
            g_sto[sto_idx]->label = set_trop();
            logInfo("sto_idx: %d, id: %d, label: %s", sto_idx, g_sto[sto_idx]->id, trop_str[g_sto[sto_idx]->label]);
        } else {
            logWarning("unknown type");
            continue;
        }
        sto_idx++;
        if (sto_idx >= TURNTABLE_STO_NUM) sto_idx = 0;
    }
}

/**
 * @brief 转盘出库
 * @param pop 出库类型
 * @return 出库状态
 */
bool Turntable_Pop(TurntablePop_t pop) {
	uint8_t pop_idx = pop;

	if (pop <= MATL_E) {
		SENSOR_Color_t target_color = matl_order[g_port.order][pop_idx];
		for (uint8_t i = 0; i < TURNTABLE_STO_NUM; i++) {
			if (g_sto[i]->color == target_color) {
				turntable_move_to_id(g_sto[i]->id);
				osDelay(TURNTABLE_SETTLE_MS);
				g_sto[i]->color = COLOR_UNKNOWN;
				return true;
			}
		}
		return false;
	} else {
		TropLabel_t target_label = trop_order[g_port.order][pop_idx - 5];
		for (uint8_t i = 0; i < TURNTABLE_STO_NUM; i++) {
			if (g_sto[i]->label == target_label) {
				turntable_move_to_id(g_sto[i]->id);
				osDelay(TURNTABLE_SETTLE_MS);
				g_sto[i]->label = LABEL_NONE;
				return true;
			}
		}
		return false;
	}
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
static TropLabel_t set_trop() {
    static uint8_t count = 0;
    TropLabel_t label = trop_order[g_port.order][count];
    if (count >= 3) count = 0;
    else count++;
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
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    setrun, set_run, change automatic inventory entry);

/**
 * @brief 查看入库物料
 */
static void view_sto(int argc, char *argv[]) {
    if (argc != 1) {
        logPrintln("Usage: %s", argv[0]);
        return;
    }
    for (int i = 0; i < TURNTABLE_STO_NUM; i++) {
        logInfo("sto_idx: %d, id: %d, color: %s, label: %s", i, g_sto[i]->id, matl_str[g_sto[i]->color], trop_str[g_sto[i]->label]);
    }
}
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    stoview, view_sto, view sto);
