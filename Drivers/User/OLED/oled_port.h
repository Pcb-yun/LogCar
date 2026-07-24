/**
 * @file oled_port.h
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief OLED模块用户层头文件
 */

#ifndef __OLED_PORT_H__
#define __OLED_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void OLED_Init(void);
void Oled_Refresh_Task(void *argument);











#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLED_PORT_H__ */
