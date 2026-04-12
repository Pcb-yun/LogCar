#include "user_uart.h"

uint8_t UsartSendBuf[USART_SEND_BUF_SIZE + 1];
uint8_t UsartRecvBuf[USART_RECV_BUF_SIZE + 1];
RingBufferTypeDef UsartSendRingBuf;//发送缓冲区
RingBufferTypeDef UsartRecvRingBuf;//接收缓冲区
Usart_DataTypeDef FSUS_Usart;
uint8_t rc1;//接收中断缓存

void User_Uart_Init(UART_HandleTypeDef *huartx)
{
	// 创建缓冲组
	RingBuffer_Init(&UsartSendRingBuf, USART_SEND_BUF_SIZE, UsartSendBuf);
	RingBuffer_Init(&UsartRecvRingBuf, USART_RECV_BUF_SIZE, UsartRecvBuf);
	// 初始化自定义用户串口结构体
	FSUS_Usart.recvBuf = &UsartRecvRingBuf;
	FSUS_Usart.sendBuf = &UsartSendRingBuf;
	FSUS_Usart.huartX = huartx;
	// 开启接收中断
	HAL_UART_Receive_IT(FSUS_Usart.huartX, (uint8_t *)&rc1, 1);
}
// 发送数据
void Usart_SendAll(Usart_DataTypeDef *usart)
{
	uint8_t value;
	while (RingBuffer_GetByteUsed(usart->sendBuf))
	{
		value = RingBuffer_Pop(usart->sendBuf);
		HAL_UART_Transmit(usart->huartX, &value, 1, 1);
	}
}
////中断接收
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//	uint8_t ucTemp;
//	if (huart->Instance == FSUS_Usart.huartX->Instance)
//	{
//		ucTemp = rc1;
//		RingBuffer_Push(FSUS_Usart.recvBuf, ucTemp);// 接收到数据放入缓冲区，不在中断具体处理数据
//		HAL_UART_Receive_IT(FSUS_Usart.huartX, (uint8_t *)&rc1, 1);
//	}
//}
