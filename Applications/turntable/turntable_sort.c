/**
 * @file turntable_sort.c
 * @brief 转盘自动分拣模块
 *
 * 功能流程:
 *   1. 周期性调用 vl53l0x_port 的 Dist_Get() 轮询进料口距离
 *   2. 扫码切换入栈模式:
 *      - 第1次扫码: 全部按物料处理, 入栈后识别颜色, 出栈顺序取 pop_order_goods
 *      - 第2次扫码: 清空存储并回初始位, 全部按奖杯处理, 不测颜色也不读字母;
 *                   奖杯物理入栈顺序由扫码值决定(1 ABC->CBA ... 6 CBA->ABC),
 *                   出栈统一 CBA
 *   3. 将种类/颜色与槽位 id 存入物料表, 通过 shell 命令查询
 */

#include "turntable_sort.h"
#include "turntable_ctrl.h"
#include "turntable_pop.h"
#include "shell.h"
#include "log.h"
#include "Events.h"
#include "cmsis_os2.h"
#include "sensor.h"
#include "rpi_sensor_port.h"
#include "vl53l0x_port.h"
#include "scan_driver.h"
#include <string.h>
#include <stdlib.h>

/* TCS230 白平衡参数与状态*/
extern bool isWB;
extern TCS230_RGBC_t rgbc_wb;

/* 物品存储表 */
static TurntableItem_t s_items[TURNTABLE_ITEM_MAX];
static uint8_t s_item_count = 0;
static uint8_t s_slot_idx = 0;              /* 当前与进料口对齐的槽位 id */
static volatile bool s_sort_running = true; /* 任务创建后默认自动运行 */
static volatile bool s_feed_clear = true;   /* 进料口已无物料(检测去抖) */
static volatile bool s_sort_busy = false;   /* 正在执行入栈动作(转动/测色) */
static volatile bool s_sort_paused = false; /* 出栈占用转盘, 暂停入栈 */

/* 扫码切换的入栈模式:
 *   第1次扫码 -> TURNTABLE_ITEM_GOODS(全部按物料)
 *   第2次扫码 -> TURNTABLE_ITEM_AWARD(全部按奖杯) */
static TurntableItemType_t s_sort_type = TURNTABLE_ITEM_GOODS;
static uint8_t s_scan_count = 0;            /* 已扫码次数 0/1/2 */

/**
 * @brief 读取颜色并推断
 * @return 颜色识别结果；未设置白平衡或读取无效时 color_name 为 NULL
 */
static SENSOR_ColorResult_t sort_read_color(void) {
    SENSOR_ColorResult_t fail = {NULL, 0};

    #if TURNTABLE_SENSOR == TCS230
    /* 非阻塞读取, 任务内驱动状态机直到完成(带超时, 防止传感器卡死) */
    SENSOR_StartReadAll(NULL);
    uint32_t t0 = osKernelGetTickCount();
    while (!SENSOR_ReadAll_IsComplete()) {
        SENSOR_Process();
        if ((osKernelGetTickCount() - t0) >= TURNTABLE_COLOR_READ_TIMEOUT_MS) {
            SENSOR_Cancel();
            logWarning("Turntable sort: color sensor read timeout, canceled");
            return fail;
        }
        osDelay(1);
    }

    const TCS230_RGBC_t *raw = SENSOR_GetResult();
    uint8_t r, g, b;
    float brightness;

    if (!sensor_rgbc_to_rgb(raw, &rgbc_wb, &r, &g, &b, &brightness)) {
        return fail;
    }
    return SENSOR_InferColor(r, g, b, brightness);
    #elif TURNTABLE_SENSOR == PRI
    /* 树莓派颜色识别 */
    return RPI_Read_Color();
    #else
    #error "Unsupported TURNTABLE_SENSOR value, check turntable_cfg.h"
    #endif
}

/**
 * @brief 处理一个物品: 入栈 -> 旋转 -> (识色) -> 存储
 * @param type 种类: TURNTABLE_ITEM_GOODS 识别颜色, TURNTABLE_ITEM_AWARD 跳过测色
 */
static void sort_process_one(TurntableItemType_t type) {
    if (s_sort_paused) return;      /* 出栈占用中, 放弃本次 */
    s_sort_busy = true;             /* 标记入栈动作进行中(出栈会等待其结束) */

    /* 1. 转到关闭位, 让进料口物品落入当前槽位(入栈) */
    turntable_move_to_close();
    osDelay(TURNTABLE_SETTLE_MS);

    /* 2. 旋转到下一槽位, 物品随转盘到达颜色检测位 */
    turntable_move_to_id((s_slot_idx + 1) % TURNTABLE_ITEM_MAX);
    osDelay(TURNTABLE_SETTLE_MS);

    TurntableItem_t *item = &s_items[s_item_count];
    item->id = s_slot_idx;
    item->type = type;
    item->valid = true;

    if (type == TURNTABLE_ITEM_AWARD) {
        /* 奖杯不测颜色, 也不检测字母(文字检测已取消) */
        strcpy(item->color, "-");
        item->confidence = 0;
        item->letter = 0;
        logPrintln("Turntable sort: stored id=%u type=Award", item->id);
    } else {
        /* 物料识别颜色 */
        SENSOR_ColorResult_t res = sort_read_color();
        if (res.color_name == NULL) {
            logWarning("Turntable sort: read color failed (need `sensor wb` first)");
            item->valid = false;
        } else {
            strncpy(item->color, res.color_name, sizeof(item->color) - 1);
            item->color[sizeof(item->color) - 1] = '\0';
            item->confidence = res.confidence;
            logPrintln("Turntable sort: stored id=%u type=Goods color=%s conf=%u%%",
                       item->id, item->color, item->confidence);
        }
    }

    /* 记录成功才计数, 失败时该槽位数据保持无效等待覆盖 */
    if (item->valid) {
        s_item_count++;
    }

    /* 3. 下一槽位与进料口对齐 */
    s_slot_idx = (s_slot_idx + 1) % TURNTABLE_ITEM_MAX;

    s_sort_busy = false;            /* 入栈动作结束 */
}

/**
 * @brief 停止自动分拣
 */
void Turntable_Sort_Stop(void) {
    s_sort_running = false;
    logPrintln("Turntable sort: stopped");
}

/**
 * @brief 清空物料存储数据并恢复自动分拣
 * @note 会先把转盘转回初始角度, 校准物理位置, 使槽位重新对齐进料口
 */
void Turntable_Sort_Clear(void) {
    memset(s_items, 0, sizeof(s_items));
    s_item_count = 0;
    s_slot_idx = 0;
    s_feed_clear = true;
    s_sort_running = true;
    turntable_move_to_init();   /* 校准物理角度, 槽位重新对齐进料口 */
    logPrintln("Turntable sort: data cleared, turntable back to init");
}

/**
 * @brief 暂停入栈(出栈占用转盘时调用)
 */
void Turntable_Sort_Pause(void) {
    s_sort_paused = true;
    /* 等待当前入栈动作结束, 避免与出栈同时驱动转盘 */
    uint32_t t0 = osKernelGetTickCount();
    while (s_sort_busy && (osKernelGetTickCount() - t0) < TURNTABLE_PAUSE_TIMEOUT_MS) {
        osDelay(10);
    }
    if (s_sort_busy) {
        /* 超时仍忙碌: 出栈强行接管转盘, 打印告警便于排查 */
        logWarning("Turntable sort: pause timeout, sort still busy, pop takes over");
    }
}

/**
 * @brief 恢复入栈(出栈结束释放转盘时调用)
 */
void Turntable_Sort_Resume(void) {
    s_sort_paused = false;
}

/**
 * @brief 移除某槽位的物料记录(出栈成功后调用)
 * @param slot_id 槽位 id
 * @return true 找到并移除
 */
bool Turntable_Sort_Remove(uint8_t slot_id) {
    for (uint8_t i = 0; i < s_item_count; i++) {
        if (s_items[i].valid && s_items[i].id == slot_id) {
            s_items[i].valid = false;
            return true;
        }
    }
    return false;
}

/**
 * @brief 获取已存储物料数量
 */
uint8_t Turntable_Sort_GetCount(void) {
    return s_item_count;
}

/**
 * @brief 获取物料存储表
 */
const TurntableItem_t *Turntable_Sort_GetItems(void) {
    return s_items;
}

/**
 * @brief 轮询扫码, 切换入栈模式
 *
 * 第1次扫码: 全部按物料, 出栈顺序取 pop_order_goods
 * 第2次扫码: 清空上一轮存储并回初始位, 全部按奖杯;
 *            奖杯物理入栈顺序由扫码值决定(1 ABC->CBA ... 6 CBA->ABC), 出栈统一 CBA
 */
static void sort_poll_scan(void) {
    char barcode[SCAN_BARCODE_MAX_LEN + 1];
    if (!Scan_GetNewBarcode(barcode, sizeof(barcode))) return;

    int code = atoi(barcode);
    if (s_scan_count == 0) {
        /* 第一次扫码: 物料模式 */
        s_scan_count = 1;
        s_sort_type = TURNTABLE_ITEM_GOODS;
        if (Turntable_Pop_SetGoodsSequence((uint8_t)code)) {
            logPrintln("Turntable sort: scan#1 code=%d, all treated as goods", code);
        } else {
            logWarning("Turntable sort: scan#1 code=%d out of range (1~16)", code);
        }
    } else if (s_scan_count == 1) {
        /* 第二次扫码: 奖杯模式, 清空上一轮数据 */
        s_scan_count = 2;
        s_sort_type = TURNTABLE_ITEM_AWARD;
        Turntable_Sort_Clear();
        if (Turntable_Pop_SetAwardSequence((uint8_t)code)) {
            logPrintln("Turntable sort: scan#2 code=%d, all treated as awards", code);
        } else {
            logWarning("Turntable sort: scan#2 code=%d out of range (1~6)", code);
        }
    } else {
        logPrintln("Turntable sort: extra scan ignored: %s", barcode);
    }
}

/**
 * @brief 自动分拣任务
 * @param argument 未使用
 *
 * 周期性轮询 VL53L0X 测距结果:
 *   - 距离低于 TURNTABLE_DIST_GOODS_THRESHOLD_MM 判定进料口有物品
 *   - 物品种类不再由距离区分, 由扫码决定的模式统一处理
 * 每次分拣后等待物品离开进料口, 再接受下一个。
 */
void Turntable_Sort_Task(void *argument) {
    (void)argument;

    osEventFlagsWait(System_StatusHandle, SYS_INIT_COMPLETE, osFlagsWaitAny, osWaitForever);

    for (;;) {
        osDelay(TURNTABLE_POLL_INTERVAL_MS);

        sort_poll_scan();           /* 扫码切换物料/奖杯模式 */

        if (!s_sort_running) continue;
        if (s_sort_paused) continue;    /* 出栈占用转盘, 暂停入栈 */
        if (SENSOR_IsBusy()) continue;

        uint16_t dist = Dist_Get();
        if (dist == 0 || dist >= TURNTABLE_DIST_GOODS_THRESHOLD_MM) {
            s_feed_clear = true;    /* 进料口无物品 */
            continue;
        }
        if (!s_feed_clear) continue;    /* 等待上一物品完全离开进料口 */

        if (s_item_count >= TURNTABLE_ITEM_MAX) {
            logPrintln("Turntable sort: storage full, stopped");
            Turntable_Sort_Stop();
            continue;
        }

        s_feed_clear = false;
        logPrintln("Turntable sort: item detected, dist=%u mm, type=%s",
                   dist, s_sort_type == TURNTABLE_ITEM_AWARD ? "Award" : "Goods");
        sort_process_one(s_sort_type);
    }
}

/**
 * @brief 打印已存储物料
 */
static void turntable_sort_list(void) {
    logPrintln("Turntable sort items (%u/%u):", s_item_count, TURNTABLE_ITEM_MAX);
    if (s_item_count == 0) {
        logPrintln("  (empty)");
        return;
    }
    for (uint8_t i = 0; i < s_item_count; i++) {
        const TurntableItem_t *item = &s_items[i];
        if (!item->valid) continue;     /* 已出栈的记录不再显示 */
        if (item->type == TURNTABLE_ITEM_AWARD) {
            logPrintln("  [%u] id=%u type=Award", i, item->id);
        } else {
            logPrintln("  [%u] id=%u type=Goods color=%-6s conf=%u%%",
                       i, item->id, item->color, item->confidence);
        }
    }
}

/**
 * @brief 自动分拣Shell命令
 * @param argc 命令参数个数
 * @param argv 命令参数数组
 */
static void turntable_sort_Shell(int argc, char *argv[]) {
    if (argc != 2) {
        logPrintln("Usage: turn_sort <stop|clear|list>");
        return;
    }
    if (strcmp(argv[1], "stop") == 0) {
        Turntable_Sort_Stop();
    } else if (strcmp(argv[1], "clear") == 0) {
        Turntable_Sort_Clear();
    } else if (strcmp(argv[1], "list") == 0) {
        turntable_sort_list();
    } else {
        logPrintln("turn_sort: invalid command: %s", argv[1]);
    }
}

/**
 * @brief 导出自动分拣Shell命令
 */
SHELL_EXPORT_CMD(
    SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
    turn_sort, turntable_sort_Shell, turntable auto sort commands);
