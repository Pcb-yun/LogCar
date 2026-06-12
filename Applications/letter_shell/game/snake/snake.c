/**
 * @file snake.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 贪吃蛇游戏移植
 */

#include "game_en.h"
#if GAME_ENABLE_SNAKE

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "shell.h"
#include "cmsis_os2.h"
#include "snake_tiny.h"

#define printf(...) shellPrint(snakeShell, ##__VA_ARGS__)
#undef getchar
#define getchar()   shellGetChar(snakeShell)

#define CONSOLE_W  40
#define CONSOLE_H  20
#define BODY_MAX   200

#define RENDER_LINES (1 + CONSOLE_H + 1)  // 标题行 + 游戏区域 + 提示行

static Shell *snakeShell = NULL;
static SnakeTiny g_game;
static SnakePoint g_body_buf[BODY_MAX];
static bool first_render = true;

static void sleep(int ms) {
    osDelay(ms);
}

static int shellGetChar(Shell *shell) {
    char data;
    while (shell->read(&data, 1) == 0) {
        sleep(1);
    }
    return data;
}

static void RenderConsole(const SnakeTiny* g) {
    int x, y, i;
    char buffer[CONSOLE_H][CONSOLE_W + 1];

    for (y = 0; y < CONSOLE_H; y++) {
        for (x = 0; x < CONSOLE_W; x++) {
            if (y == 0 || y == CONSOLE_H - 1 || x == 0 || x == CONSOLE_W - 1)
                buffer[y][x] = '#';
            else
                buffer[y][x] = ' ';
        }
        buffer[y][CONSOLE_W] = '\0';
    }

    buffer[g->food.y + 1][g->food.x + 1] = '$';

    for (i = 0; i < g->len; i++) {
        char marker = (i == 0) ? '@' : 'o';
        buffer[g->body[i].y + 1][g->body[i].x + 1] = marker;
    }

    if (first_render) {
        // 首次渲染：清屏后完整绘制
        printf("\033[2J\033[H");
        first_render = false;
    } else {
        // 后续渲染：光标上移覆盖
        printf("\033[%dA", RENDER_LINES);
    }

    printf("\rSnakeTiny | Score: %u\r\n", g->score);
    for (y = 0; y < CONSOLE_H; y++) {
        printf("%s\r\n", buffer[y]);
    }
    printf("WASD to Move. Q to Quit.");
}

int main_snake(int argc, char *argv[]) {
    SnakeEvent evt;
    (void)argc;
    (void)argv;

    snakeShell = shellGetCurrent();
    if (!snakeShell) {
        return -1;
    }

    first_render = true;
    if (!snake_init(&g_game, CONSOLE_W - 2, CONSOLE_H - 2, 1, g_body_buf, BODY_MAX, (uint32_t)SHELL_GET_TICK())) {
        printf("Init Failed!\r\n");
        return -1;
    }

    printf("Press Any Key to Start...\r\n");
    getchar();

    while (g_game.alive) {
        if (snakeShell->read((char*)&evt, 1) > 0) {
            char key = (char)evt;
            switch (key) {
                case 'w': case 'W': snake_set_dir(&g_game, SNAKE_UP);    break;
                case 's': case 'S': snake_set_dir(&g_game, SNAKE_DOWN);  break;
                case 'a': case 'A': snake_set_dir(&g_game, SNAKE_LEFT);  break;
                case 'd': case 'D': snake_set_dir(&g_game, SNAKE_RIGHT); break;
                case 'q': case 'Q': return 0;
            }
        }

        evt = snake_tick(&g_game);

        RenderConsole(&g_game);

        sleep(100);
    }

    printf("\r\nGAME OVER! Final Score: %u\r\n", g_game.score);

    return 0;
}

#endif /* GAME_ENABLE_SNAKE */
