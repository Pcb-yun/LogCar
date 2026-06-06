/**
 ******************************************************************************
 * @file    snake_tiny.c
 * @author  Hanling
 * @brief   极简贪吃蛇逻辑内核（平台无关）- 源文件（C89/ARMCC兼容）
 *   1. 本模块仅包含游戏逻辑，不包含任何延时/定时/系统时钟相关代码
 *   2. 每调用一次 snake_tick()，游戏逻辑推进一步
 *   3. 游戏速度由 snake_tick() 调用频率决定
 *   4. 不使用 malloc：蛇身缓存由使用者提供
 *
 * @note		 本程序使用方式：
 *		1.	使用 snake_init 配置合适的参数进行初始化
 *		2.	使用 snake_set_dir 输入以任何方式获取的方向（包括但不限于：按键、触屏、摇杆）
 *		3.	使用 snake_tick 将游戏数据运行到下一步，可以使用任何延时方式和触发频率，然后可以处理其返回值
 *		4.  使用 SnakeTiny 结构体中储存的坐标，以任何方式刷新画面
 *
 ******************************************************************************
 * @attention
 *
 * 本代码为开源项目
 *
 * 作者：    江寒凌
 * CSDN：    https://blog.csdn.net/Hanling_
 * GitHub：	 https://github.com/K-Hanling
 *
 ******************************************************************************
 */

#ifndef SNAKE_TINY_H
#define SNAKE_TINY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief  移动方向
     */
    typedef enum
    {
        SNAKE_UP = 0,
        SNAKE_RIGHT = 1,
        SNAKE_DOWN = 2,
        SNAKE_LEFT = 3
    } SnakeDir;

    /**
     * @brief  snake_tick 执行结果（事件）
     * @note   上层可根据返回值决定是否蜂鸣、闪灯、加分显示等
     */
    typedef enum
    {
        SNAKE_EVT_NONE = 0, /* 未发生（一般不用） */
        SNAKE_EVT_MOVE,     /* 正常移动了一步 */
        SNAKE_EVT_EAT,      /* 吃到食物（可能伴随变长） */
        SNAKE_EVT_DIE_WALL, /* 撞墙死亡 */
        SNAKE_EVT_DIE_SELF, /* 撞到自己死亡 */
        SNAKE_EVT_WIN       /* 达到最大长度，胜利 */
    } SnakeEvent;

    /**
     * @brief  网格坐标点（不是像素坐标）
     */
    typedef struct
    {
        unsigned char x;
        unsigned char y;
    } SnakePoint;

    /**
     * @brief  贪吃蛇核心结构体
     * @note
     *   1. 由使用者分配（栈/全局/静态均可）
     *   2. snake_init() 内部会清零并完成初始化
     */
    typedef struct
    {
        // ---------- 配置（由 snake_init() 设置） ----------
        uint16_t screen_w_px; // 屏幕宽度（像素）
        uint16_t screen_h_px; // 屏幕高度（像素）
        uint8_t cell_px;      // 一个“格子”对应多少像素（蛇块大小）
        SnakePoint *body;     // 蛇身缓存（使用者提供）

        uint8_t grid_w;    // 网格宽度 = screen_w_px / cell_px
        uint8_t grid_h;    // 网格高度 = screen_h_px / cell_px
        uint16_t body_cap; // 蛇身缓存容量（最大长度）

        // ---------- 状态 ----------
        bool alive;           // 是否存活
        uint16_t len;         // 当前长度
        SnakeDir dir;         // 当前方向（tick使用）
        SnakeDir pending_dir; // 输入方向（由 snake_set_dir 写入）

        SnakePoint food; // 食物（一个点）
        uint16_t score;  // 分数/吃到次数

        // ---------- 随机数 ----------
        uint32_t rng; // 随机数状态（可复现）
    } SnakeTiny;

    /**
     * @brief  初始化贪吃蛇核心
     * @note
     *   1. 内部对 SnakeTiny 整体 memset 清零
     *   2. 初始蛇长为 3，默认向右
     *   3. 初始位置居中
     * 	4. 用户可以把 SnakeTiny 放在任何位置（栈/全局/静态/内存池）
     * 	5. 用户也可以把 body_storage 放在任何位置（静态数组推荐）
     *
     * @param  g：SnakeTiny 结构体指针
     * @param  grid_w：网格宽度（1~255，建议>=4）
     * @param  grid_h：网格高度（1~255，建议>=4）
     * @param  body_storage：蛇身缓存数组指针（使用者提供）
     * @param  body_cap：蛇身缓存容量（建议>=8）
     * @param  seed：随机种子（0 将自动修正为 1）
     *
     * @retval 成功：1
     *       	失败：0
     */
    bool snake_init(SnakeTiny *g,
                    uint16_t screen_w_px,
                    uint16_t screen_h_px,
                    uint8_t cell_px,
                    SnakePoint *body_storage,
                    uint16_t body_cap,
                    uint32_t seed);

    /**
     * @brief  设置蛇移动方向
     * @note   禁止直接反向移动（经典规则）
     * @param  g：SnakeTiny 结构体指针
     * @param  d：目标方向
     * @retval 无
     */
    void snake_set_dir(SnakeTiny *g, SnakeDir d);

    /**
     * @brief  推进游戏逻辑一步，并返回本步事件
     * @note   无延时；游戏速度由调用频率决定
     * @param  g：SnakeTiny 结构体指针
     * @retval 本步事件（移动/吃到/死亡原因）
     */
    SnakeEvent snake_tick(SnakeTiny *g);

    // 便捷读取
    static inline uint8_t snake_grid_w(const SnakeTiny *g) { return g->grid_w; }
    static inline uint8_t snake_grid_h(const SnakeTiny *g) { return g->grid_h; }

#ifdef __cplusplus
}
#endif

#endif // SNAKE_TINY_H
