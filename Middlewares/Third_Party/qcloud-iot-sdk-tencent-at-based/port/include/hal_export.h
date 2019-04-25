/******************************************************************************
 * @file           : hal_export.h
 * @brief          : hal api export head file
 ******************************************************************************
 *
 * Copyright (c) 2019 Tencent. 
 * All rights reserved.
 ******************************************************************************
 */
  
#ifndef __HAL_EXPORT_H__
#define __HAL_EXPORT_H__
#include "stdint.h"
#include "stdbool.h"
#include <stdarg.h>

#define 	PARSE_THREAD_STACK_SIZE		1024
#define 	OS_USED
#define 	_IN_            /* 表明这是一个输入参数. */
#define		_OU_            /* 表明这是一个输出参数. */
#define 	HAL_AT_ERROR	   -1  
#define 	HAL_AT_OK		   0

#define  	AUTH_MODE_KEY
#define 	DEBUG_DEV_INFO_USED

#ifndef 	AUTH_MODE_KEY	//非PSK认证则为证书认证
#define		AUTH_MODE_CERT
#endif

//#define 	AT_PRINT_RAW_CMD


#ifdef OS_USED
#include "cmsis_os.h"
#endif

typedef void * osThreadId;

typedef struct _Timer_ {
    uint32_t end_time;
}Timer;


int at_send_data(uint8_t *pdata, uint16_t len);
int module_power_on(void);
int module_power_off(void);

void HAL_Printf(_IN_ const char *fmt, ...);
int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...);
int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap);

void HAL_DelayMs(_IN_ uint32_t ms);
void HAL_DelayUs(_IN_ uint32_t us);
uint32_t HAL_GetTimeMs(void);
uint32_t HAL_GetTimeSeconds(void);


bool HAL_Timer_expired(Timer *timer);
void HAL_Timer_countdown_ms(Timer *timer, unsigned int timeout_ms);
void HAL_Timer_countdown(Timer *timer, unsigned int timeout); 
int HAL_Timer_remain(Timer *timer); 
void HAL_Timer_init(Timer *timer); 

int HAL_GetProductID(char *pProductId, uint8_t maxlen);
int HAL_GetDevName(char *pDevName, uint8_t maxlen);
int HAL_SetProductID(const char *pProductId);
int HAL_SetDevName(const char *pDevName);
int HAL_GetDevCertName(char *pDevCert, uint8_t maxlen);
int HAL_GetDevPrivateKeyName(char *pDevPrivateKey, uint8_t maxlen);
int HAL_SetDevCertName(char *pDevCert);
int HAL_SetDevPrivateKeyName(char *pDevPrivateKey);
int HAL_GetDevSec(char *pDevSec, uint8_t maxlen);
int HAL_SetDevSec(const char *pDevSec);

#ifdef OS_USED
void hal_thread_create(volatile void* threadId, uint16_t stackSize, int Priority, void (*fn)(void*), void* arg);
void hal_thread_destroy(void* threadId);
void HAL_SleepMs(_IN_ uint32_t ms);
void *HAL_MutexCreate(void);
void HAL_MutexDestroy(_IN_ void* mutex);
void HAL_MutexLock(_IN_ void* mutex);
void HAL_MutexUnlock(_IN_ void* mutex);
void *HAL_Malloc(_IN_ uint32_t size);
void HAL_Free(_IN_ void *ptr);
#endif

#endif
