/**
 * @file memory.h
 * @brief 内存分配宏定义
 *
 * 使用 FreeRTOS API 实现内存分配
 */

#include "game_en.h"
#if GAME_ENABLE_AP


#ifndef MEMORY_H
#define MEMORY_H

#include "FreeRTOS.h"

/**
 * @brief 分配内存
 * @param size 分配的字节数
 * @return 分配到的内存指针，失败返回 NULL
 */
#ifndef MALLOC
#define MALLOC(size) pvPortMalloc(size)
#endif

/**
 * @brief 释放内存
 * @param ptr 要释放的内存指针
 */
#ifndef FREE
#define FREE(ptr) vPortFree(ptr)
#endif

#endif // MEMORY_H

#endif /* GAME_ENABLE_AP */
