/*********************************************************************************
 *                   Copyright (c) 2016 - 2020,Tencent
 *                      All rights reserved.
 *
 * File Name:    retarget.c
 *
 * Description:   retarget stdin and stdout for printf
 *
 * History:      <author>          <time>        <version>
 *                   yougaliu          2018-6-6        1.0
 * Desc:           ORG.
 ********************************************************************************/
 
 
/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"


extern UART_HandleTypeDef huart1;
UART_HandleTypeDef *pDebugUart = &huart1;



#if !defined(__ICCARM__)
struct __FILE 
{
    int handle;
};
#endif

FILE __stdout;
FILE __stdin;



#if defined(__CC_ARM) ||  defined(__ICCARM__)
int fgetc(FILE * p_file)
{
    uint8_t input;

	if(HAL_UART_Receive(pDebugUart, &input, 1, 0xFFFF) == HAL_OK)
	{
		return input;
	}
    else
    {
		return -1;
	}
}


int fputc(int ch, FILE * p_file)
{

  HAL_UART_Transmit(pDebugUart, (uint8_t *)&ch, 1, 0xFFFF);
  
  return ch;
}

#elif defined(__GNUC__)


int _write(int file, const char * p_char, int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
       HAL_UART_Transmit(pDebugUart, (uint8_t *)p_char, 1, 0xFFFF);
    }

    return len;
}


int _read(int file, char * p_char, int len)
{

	HAL_UART_Transmit(pDebugUart, (uint8_t *)p_char, 1, 0xFFFF);

    return 1;
}
#endif
