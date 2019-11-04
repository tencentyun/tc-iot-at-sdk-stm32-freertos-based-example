/*********************************************************************************
 *
 * File Name:    board.h
 *
 * Description:    board drivers for stm32F103 dev kit board
 *
 * History:      <author>          <time>        <version>
 *              yougaliu          2018-7-19        1.0
 * Desc:          
 ********************************************************************************/

#ifndef __BOARD_H_
#define __BOARD_H_


#define OLED_I2C_Speed              400000  
#define AT_UART_BAUDRATE			(115200)
#define DEBUG_UART_BAUDRATE			(115200)


void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_TIM2_Init(void);
void BoardInit(void);
void AT_Uart_Init(void);



#endif

