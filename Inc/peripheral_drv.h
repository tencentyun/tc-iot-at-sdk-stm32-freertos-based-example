/*********************************************************************************
 *
 * File Name:    peripheral_drv.h
 *
 * Description:    peripheral device drivers for stm32F103 dev kit board
 *
 * History:      <author>          <time>        <version>
 *               yougaliu          2018-7-19        1.0
 * Desc:           ORG.
 ********************************************************************************/

#ifndef __PERIPHERAL_H_
#define __PERIPHERAL_H_

#include "stdbool.h"
#include "stm32f1xx_hal.h"


#define 	OLED_MAX_COL		(128)
#define 	OLED_MAX_ROW		(8)

#define 	CHAR_WID_16_ADD		(8)
#define 	CHAR_WID_8_ADD		(6)
#define		OLED_I2C_ADDRESS	(0x78)
#define		HTU_I2C_ADDRESS		(0x80)
#define     I2C_TIME_OUT		(0x1000)

#define     RGB_CLK_CYCLE		(20)
#define     RGB_FRAME_BIT		(32)
#define     RGB_FRAME_MSB		(1U<<(RGB_FRAME_BIT - 1))&0xFFFFFFFF


#define 	GPIO_DIN_LOW	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_RESET)
#define 	GPIO_DIN_HIGH	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_SET)

#define 	GPIO_CIN_LOW 	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET)
#define 	GPIO_CIN_HIGH 	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET)



typedef enum _char_width_type {
    CHAR_WID_16 = 16,
    CHAR_WID_8 = 8,   
} _char_width_type;

typedef enum _oled_Cmd_type {
    CMD_TYPE = 0,
    DATA_TYPE = 0x40,   
} oled_Cmd_type;


typedef enum _HT_Cmd_type {
    CMD_READ_TEMP = 0xE3,
    CMD_READ_HUM = 0xE5,   
} eHT_Cmd_type;

typedef enum _dev_state_type {
    STATE_ON = 0,
    STATE_OFF = 1,   
} dev_state_type;

typedef enum _motor_control_type {
	MOTOR_OFF = 0,
    FORWORD_ON = 1,
    BACKWORD_ON = 2,       
} motor_control_type;

typedef enum _eBuzzer_control_type {
	BUZZER_OFF = 0,
    BUZZER_ON = 1,       
} eBuzzer_control_type;

typedef enum _eHT_Select_type {
	HUM_READ = 0,
    TEMP_READ = 1,       
} eHT_Select_type;


HAL_StatusTypeDef OledInit(void);
HAL_StatusTypeDef OledShowString(uint8_t x, uint8_t *pRow, char *chr, uint8_t Char_Size, bool IsClear);
HAL_StatusTypeDef OledClear(uint8_t fill_Data);

void InitRGBState(void);
void GrayDataSend(uint8_t r, uint8_t g, uint8_t b);
void BuzzerControl(eBuzzer_control_type eCtlType);
void MotorControl(motor_control_type eCtlType);
void GetLumen(uint32_t *plumen);
HAL_StatusTypeDef HumAndTempRead(eHT_Select_type eSelect, float *value);





#endif

