/**
 * @file memory.h
 * @brief 内存分配宏定义
 *
 * 使用 FreeRTOS API 实现内存分配
 */

#ifndef MEMORY_H
#define MEMORY_H

// ============================================================================
// 内存分配宏定义
// 针对 FreeRTOS 嵌入式环境配置
// ============================================================================

#include "FreeRTOS.h"

// ----------------------------------------------------------------------------
// FreeRTOS 分配器
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// 静态内存池分配器（可选，取消注释以启用）
// ----------------------------------------------------------------------------
/*
#define MEMORY_POOL_SIZE 65536
extern char memory_pool[MEMORY_POOL_SIZE];
extern unsigned int memory_pool_ptr;

#define MALLOC(size) memory_pool_alloc(size)
#define FREE(ptr) ((void)0)  // 不支持释放

static inline void* memory_pool_alloc(unsigned int size) {
	if (memory_pool_ptr + size > MEMORY_POOL_SIZE) return NULL;
	void* ptr = memory_pool + memory_pool_ptr;
	memory_pool_ptr += size;
	return ptr;
}
*/

#endif // MEMORY_H
