/**
 * @file message.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief 拓展消息显示模块源文件
 */

#include "message.h"
#if MESSAGE_ENABLE

/**
 * @brief 显示启动状态消息
 * @param status 消息状态
 * @param string 消息字符串
 */
void Show_dmesg(const dmesg_status status, const char *string) {
  switch(status) {
    case dmesg_wait:
      my_printf("[wait] %s", string); break;
    case dmesg_ok:
      my_printf("\r[ ok ]\r\n"); break;
    case dmesg_fail:
      my_printf("\r[fail]\r\n"); break;
  }
}





#endif
