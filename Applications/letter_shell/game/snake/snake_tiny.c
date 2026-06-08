#include "game_en.h"
#if GAME_ENABLE_SNAKE

#include "snake_tiny.h"
#include <string.h>

/* ============================= 内部函数声明 ============================= */

/**
  * @brief  生成下一个伪随机数
  * @note
  *   1. 使用线性同余法（LCG）
  *   2. 仅包含整数运算（乘法+加法），适合 MCU
  *   3. 利用 uint32_t 溢出自动完成 (mod 2^32)
  * @param  g：SnakeTiny 结构体指针
  * @retval 32 位伪随机数
  */
static uint32_t rng_next(SnakeTiny* g);

/**
  * @brief  判断两个方向是否互为反向
  * @note   用于禁止蛇直接反向移动（经典规则）
  * @param  a：方向 a
  * @param  b：方向 b
  * @retval 互为反向：true；否则：false
  */
static bool is_opposite(SnakeDir a, SnakeDir b);

/**
  * @brief  判断两个点是否相等
  * @param  a：点 a（网格坐标）
  * @param  b：点 b（网格坐标）
  * @retval 相等：true；不等：false
  */
static bool point_eq(SnakePoint a, SnakePoint b);

/**
  * @brief  判断某点是否被蛇身占用（只检查前 check_len 段）
  * @note
  *   1. 约定 body[0] 为蛇头
  *   2. check_len 用于“踩尾巴”规则：不吃时可忽略尾巴一段
  * @param  g：SnakeTiny 结构体指针
  * @param  p：待检测点
  * @param  check_len：检查长度（从 body[0] 开始）
  * @retval 被占用：true；未占用：false
  */
static bool snake_contains(const SnakeTiny* g, SnakePoint p, uint16_t check_len);

/**
  * @brief  刷新/生成食物位置（随机找空格）
  * @note   网格规模较小（如 32x16），重试法完全够用
  * @param  g：SnakeTiny 结构体指针
  * @retval 无
  */
static void spawn_food(SnakeTiny* g);


/* ============================= 内部函数实现 ============================= */

// 随机数计算
static uint32_t rng_next(SnakeTiny* g)
{
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

// 反向判断
static bool is_opposite(SnakeDir a, SnakeDir b)
{
    return ((((uint8_t)a + 2u) & 3u) == ((uint8_t)b & 3u));
}

// 坐标重叠判断
static bool point_eq(SnakePoint a, SnakePoint b)
{
    return (a.x == b.x) && (a.y == b.y);
}

// 碰撞检测
static bool snake_contains(const SnakeTiny* g, SnakePoint p, uint16_t check_len)
{
    uint16_t i;

    for (i = 0; i < check_len; i++)
    {
        if (point_eq(g->body[i], p))
        {
            return true;
        }
    }
    return false;
}


// 刷新食物
static void spawn_food(SnakeTiny* g)
{
    uint16_t tries;
    uint32_t r;
    SnakePoint f;

    tries = 600;

    while (tries--)
    {
        r = rng_next(g);

        f.x = (uint8_t)(r % g->grid_w);
        f.y = (uint8_t)((r >> 8) % g->grid_h);

        if (!snake_contains(g, f, g->len))
        {
            g->food = f;
            return;
        }
    }

    // 极端情况：蛇几乎占满（兜底）
    g->food.x = 0;
    g->food.y = 0;
}


/* ============================= 对外 API 实现 ============================= */

/**
  * @brief  贪吃蛇游戏核心初始化
  * @note
  *   1. 本函数内部使用 memset 对结构体清零
  *   2. 使用者无需提前对 SnakeTiny 进行 memset 或赋初值
  *   3. 本函数不包含任何延时或游戏速度设置
  * @param  g：SnakeTiny 结构体指针
  * @param  screen_w_px：屏幕宽度（像素）
  * @param  screen_h_px：屏幕高度（像素）
  * @param  cell_px：蛇块像素大小（例如 2/4/8）
  * @param  body_storage：蛇身缓存数组指针（由使用者提供）
  * @param  body_cap：蛇身缓存容量
  * @param  seed：随机数种子（0 表示使用默认种子）
  * @retval 成功：true
  *         失败：false
  */
bool snake_init(SnakeTiny* g,
                uint16_t screen_w_px,
                uint16_t screen_h_px,
                uint8_t cell_px,
                SnakePoint* body_storage,
                uint16_t body_cap,
                uint32_t seed)
{
    uint16_t gw;
    uint16_t gh;
    uint8_t  cx;
    uint8_t  cy;

    if (!g || !body_storage)
    {
        return false;
    }

    if (cell_px == 0)
    {
        return false;
    }

    // 计算网格大小（像素不能整除时会截断，边缘像素会浪费）
    gw = (uint16_t)(screen_w_px / cell_px);
    gh = (uint16_t)(screen_h_px / cell_px);

    // 网格太小无法游玩
    if (gw < 4 || gh < 4)
    {
        return false;
    }

    // 坐标使用 uint8_t 存储，网格尺寸不能超过 255
    if (gw > 255 || gh > 255)
    {
        return false;
    }

    // 蛇身缓存至少容纳初始长度（本实现初始=3，给 8 更保险）
    if (body_cap < 8)
    {
        return false;
    }

    // 整体清零
    memset(g, 0, sizeof(*g));

    // 写入配置
    g->screen_w_px = screen_w_px;
    g->screen_h_px = screen_h_px;
    g->cell_px     = cell_px;
    g->grid_w      = (uint8_t)gw;
    g->grid_h      = (uint8_t)gh;

    g->body     = body_storage;
    g->body_cap = body_cap;

    // 随机种子修正（seed=0 则置为 1）
    g->rng = (seed == 0u) ? 1u : seed;

    // 初始状态
    g->alive = true;
    g->score = 0;

    // 初始方向：向右
    g->dir = SNAKE_RIGHT;
    g->pending_dir = SNAKE_RIGHT;

    // 初始长度：3，居中放置
    g->len = 3;

    cx = (uint8_t)(g->grid_w / 2);
    cy = (uint8_t)(g->grid_h / 2);

    // 约定：body[0] 为蛇头
    g->body[0].x = cx;
    g->body[0].y = cy;

    g->body[1].x = (uint8_t)(cx - 1);
    g->body[1].y = cy;

    g->body[2].x = (uint8_t)(cx - 2);
    g->body[2].y = cy;

    // 生成食物
    spawn_food(g);

    return true;
}


/**
  * @brief  设置蛇的移动方向（按键层调用）
  * @note
  *   1. 禁止直接反向移动（经典贪吃蛇规则）
  *   2. 本函数不会立即推动蛇移动，只会记录 pending_dir
  * @param  g：SnakeTiny 结构体指针
  * @param  d：目标方向
  * @retval 无
  */
void snake_set_dir(SnakeTiny* g, SnakeDir d)
{
    if (!g || !g->alive)
    {
        return;
    }

    // 禁止反向
    if (!is_opposite(g->dir, d))
    {
        g->pending_dir = d;
    }
}


/**
  * @brief  推进贪吃蛇游戏逻辑一步
  * @note
  *   1. 每调用一次本函数，游戏状态推进一个逻辑步
  *   2. 本函数不包含任何延时或阻塞操作
  *   3. 游戏运行速度由本函数的调用频率决定
  * @param  g：SnakeTiny 结构体指针
  * @retval SnakeEvent事件枚举，用户可根据需要进行处理
  *
  */
SnakeEvent snake_tick(SnakeTiny* g)
{
    SnakePoint head;
    SnakePoint next;
    uint16_t check_len;
    uint16_t i;
    uint8_t eat;

    if (!g || !g->alive)
    {
        return SNAKE_EVT_NONE;
    }

    // 采纳输入方向（再次防反向）
    if (!is_opposite(g->dir, g->pending_dir))
    {
        g->dir = g->pending_dir;
    }

    // 计算下一格（撞墙死）
    head = g->body[0];
    next = head;

    switch (g->dir)
    {
        case SNAKE_UP:
            if (head.y == 0)
            {
                g->alive = false;
                return SNAKE_EVT_DIE_WALL;
            }
            next.y = (uint8_t)(head.y - 1);
            break;

        case SNAKE_RIGHT:
            if ((uint16_t)head.x + 1 >= g->grid_w)
            {
                g->alive = false;
                return SNAKE_EVT_DIE_WALL;
            }
            next.x = (uint8_t)(head.x + 1);
            break;

        case SNAKE_DOWN:
            if ((uint16_t)head.y + 1 >= g->grid_h)
            {
                g->alive = false;
                return SNAKE_EVT_DIE_WALL;
            }
            next.y = (uint8_t)(head.y + 1);
            break;

        case SNAKE_LEFT:
            if (head.x == 0)
            {
                g->alive = false;
                return SNAKE_EVT_DIE_WALL;
            }
            next.x = (uint8_t)(head.x - 1);
            break;

        default:
            break;
    }

    // 是否吃到食物
    eat = (uint8_t)point_eq(next, g->food);

    // 撞自己判定（踩尾巴规则）
    check_len = eat ? g->len : (uint16_t)(g->len - 1);
    if (snake_contains(g, next, check_len))
    {
        g->alive = false;
        return SNAKE_EVT_DIE_SELF;
    }

		// 吃到则增长
		if (eat)
		{
				if (g->len < g->body_cap)
				{
						g->len++;
						g->score++;

						// 达到最大长度 -> 胜利
						if (g->len >= g->body_cap)
						{
								g->alive = 0u;
								return SNAKE_EVT_WIN;
						}
				}
				else
				{
						// 理论兜底：已经满了还能吃到（一般不会发生）
						g->alive = 0u;
						return SNAKE_EVT_WIN;
				}
		}

    // 移动（数组整体右移；最简单）
    i = (uint16_t)(g->len - 1);
    while (i > 0)
    {
        g->body[i] = g->body[i - 1];
        i--;
    }
    g->body[0] = next;

    // 吃到后刷新食物
    if (eat)
    {
        spawn_food(g);
        return SNAKE_EVT_EAT;
    }

    return SNAKE_EVT_MOVE;
}

#endif /* GAME_ENABLE_SNAKE */
