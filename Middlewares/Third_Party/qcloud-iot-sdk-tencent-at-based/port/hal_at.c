/**
 ******************************************************************************
 * @file			: hat_at.c
 * @brief			: hardware driver for at cmd send and data recv, device init
 ******************************************************************************
 *
 * History:      <author>          <time>        <version>
 *               yougaliu          2019-3-20        1.0
 * Desc:           ORG.
 * Copyright (c) 2019 Tencent. 
 * All rights reserved.
 ******************************************************************************
 */
#include <string.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"
#include "ringbuff.h"
#include "at_log.h"
#include "hal_export.h"




#define AT_UART_IRQHandler           UART4_IRQHandler

extern UART_HandleTypeDef huart4;
static UART_HandleTypeDef *pAtUart = &huart4;

/*uart data recv buff which used by at_client*/
extern  sRingbuff 	  g_ring_buff;



/**
* @brief This function handles AT UART global interrupt,push recv char to ringbuff.
*/
void AT_UART_IRQHandler(void)
{ 
	uint8_t ch;
	if(__HAL_UART_GET_FLAG(pAtUart, UART_FLAG_RXNE) == SET)
	{	
		ch = (uint8_t) READ_REG(pAtUart->Instance->DR)&0xFF;	
		if(RINGBUFF_OK != ring_buff_push_data(&g_ring_buff, &ch, 1))
		{
		  //Log_e("ring buff push err"); // do not printf in isr at normal situation
		}		
		//__HAL_UART_CLEAR_FLAG(pAtUart, UART_IT_RXNE);				
	}
	__HAL_UART_CLEAR_PEFLAG(pAtUart);
}

/**
 *pdata: pointer of data for send
 *len:  len of data to be sent
 *return: the len of data send success
 * @brief hal api for at data send
 */
int at_send_data(uint8_t *pdata, uint16_t len)
{
	if(HAL_OK == HAL_UART_Transmit(pAtUart, pdata, len, 0xFFFF))
	{
		return len;
	}
	else
	{
		return 0;
	}	
}


extern void AT_Uart_Init(void);
/**
 * return: module power on resault
 * @brief power on module
 */
int module_power_on(void)
{
	Log_e("module power on is not implement");
	AT_Uart_Init();
	HAL_DelayMs(100);
	
#ifdef 	MODULE_TYPE_ESP8266
	/*work around first byte miss*/
	at_send_data("AT+RST\r\n", strlen("AT+RST\r\n"));
	HAL_DelayMs(100);
	at_send_data("AT+RST\r\n", strlen("AT+RST\r\n"));
#endif

#ifdef 	MODULE_TYPE_L206D
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
	HAL_DelayMs(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
	/* reset waiting delay */
	HAL_DelayMs(1000);
	
	/*reset module work around first byte miss*/
	at_send_data(" AT+CFUN=1,1\r\n", strlen(" AT+CFUN=1,1\r\n"));
	HAL_DelayMs(100);
	at_send_data(" AT+CFUN=1,1\r\n", strlen(" AT+CFUN=1,1\r\n"));
	HAL_DelayMs(1000);
#endif

	
    /* reset waiting delay */
    HAL_DelayMs(2000);
	
	ring_buff_flush(&g_ring_buff);
	
	return HAL_AT_OK;
}

/**
 *return: module power off resault
 * @brief power off module
 */
int module_power_off(void)
{
	Log_e("module power on is not implement");
	return HAL_AT_OK;
}

