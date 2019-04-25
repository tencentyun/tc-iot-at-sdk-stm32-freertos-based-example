/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "stm32f1xx_hal.h"		//This file is based STM32 just now
#include "hal_export.h"
#include "qcloud_iot_export_mqtt.h"

#ifdef  DEBUG_DEV_INFO_USED

static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1]	 = "03UKNYBUZG";
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1]  = "at_dev";

#ifdef AUTH_MODE_CERT
static char sg_device_cert_file_name[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1]      = "YOUR_DEVICE_NAME_cert.crt";
static char sg_device_privatekey_file_name[MAX_SIZE_OF_DEVICE_KEY_FILE_NAME + 1] = "YOUR_DEVICE_NAME_private.key";
#else
char sg_device_secret[MAX_SIZE_OF_DEVICE_SERC + 1] = "ttOARy0PjYgzd9OSs4Z3RA==";
#endif

#endif

void HAL_Printf(_IN_ const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
}

int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}


void HAL_DelayUs(_IN_ uint32_t us)
{
    HAL_Delay(us);
}

uint32_t HAL_GetTimeMs(void)
{	
    return HAL_GetTick();
}

uint32_t HAL_GetTimeSeconds(void)
{	
    return HAL_GetTimeMs()/1000;
}


void HAL_DelayMs(_IN_ uint32_t ms)
{
   (void)HAL_Delay(ms);
}

#ifdef OS_USED
void hal_thread_create(volatile void* threadId, uint16_t stackSize, int Priority, void (*fn)(void*), void* arg)
{
	osThreadDef(parseTask, (os_pthread)fn, (osPriority)Priority, 0, stackSize);
	threadId = osThreadCreate(osThread(parseTask), arg);
	(void)threadId; //Eliminate warning
}

void hal_thread_destroy(void* threadId)
{
	osThreadTerminate(threadId);
}

void HAL_SleepMs(_IN_ uint32_t ms)
{
   (void)osDelay(ms);
}

void *HAL_Malloc(_IN_ uint32_t size)
{
	return pvPortMalloc( size);
}

void HAL_Free(_IN_ void *ptr)
{
    vPortFree(ptr);
}

void *HAL_MutexCreate(void)
{
	return (void *)osMutexCreate (NULL);
}


void HAL_MutexDestroy(_IN_ void * mutex)
{
	osStatus ret;
	
    if(osOK != (ret = osMutexDelete((osMutexId)mutex)))
    {
		HAL_Printf("HAL_MutexDestroy err, err:%d\n\r",ret);
	}
}

void HAL_MutexLock(_IN_ void * mutex)
{
	osStatus ret;

	if(osOK != (ret = osMutexWait((osMutexId)mutex, osWaitForever)))
	{
		HAL_Printf("HAL_MutexLock err, err:%d\n\r",ret);
	}
}

void HAL_MutexUnlock(_IN_ void * mutex)
{
	osStatus ret;

	if(osOK != (ret = osMutexRelease((osMutexId)mutex)))
	{
		HAL_Printf("HAL_MutexUnlock err, err:%d\n\r",ret);
	}	
}
#else
void hal_thread_create(void** threadId, void (*fn)(void*), void* arg)
{

}

void HAL_SleepMs(_IN_ uint32_t ms)
{

}

void *HAL_Malloc(_IN_ uint32_t size)
{
	return NULL;
}

void HAL_Free(_IN_ void *ptr)
{
   
}

void *HAL_MutexCreate(void)
{
	return NULL;
}


void HAL_MutexDestroy(_IN_ void* mutex)
{
	
}

void HAL_MutexLock(_IN_ void* mutex)
{

}

void HAL_MutexUnlock(_IN_ void* mutex)
{

}

#endif

int HAL_GetProductID(char *pProductId, uint8_t maxlen)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(sg_product_id) > maxlen){
		return AT_ERR_FAILURE;
	}

	memset(pProductId, '\0', maxlen);
	strncpy(pProductId, sg_product_id, maxlen);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_GetProductID is not implement");
	return AT_ERR_FAILURE;
#endif
}


int HAL_GetDevName(char *pDevName, uint8_t maxlen)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(sg_device_name) > maxlen){
		return AT_ERR_FAILURE;
	}

	memset(pDevName, '\0', maxlen);
	strncpy(pDevName, sg_device_name, maxlen);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_GetDevName is not implement");
	return AT_ERR_FAILURE;
#endif
}


int HAL_SetProductID(const char *pProductId)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(pProductId) > MAX_SIZE_OF_PRODUCT_ID){
		return AT_ERR_FAILURE;
	}

	memset(sg_product_id, '\0', MAX_SIZE_OF_PRODUCT_ID);
	strncpy(sg_product_id, pProductId, MAX_SIZE_OF_PRODUCT_ID);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_SetProductID is not implement");
	return AT_ERR_FAILURE;
#endif
}

int HAL_SetDevName(const char *pDevName)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(pDevName) > MAX_SIZE_OF_DEVICE_NAME){
		return AT_ERR_FAILURE;
	}

	memset(sg_device_name, '\0', MAX_SIZE_OF_DEVICE_NAME);
	strncpy(sg_device_name, pDevName, MAX_SIZE_OF_DEVICE_NAME);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_SetDevName is not implement");
	return AT_ERR_FAILURE;
#endif
}

#ifdef AUTH_MODE_CERT	//证书 认证方式

int HAL_GetDevCertName(char *pDevCert, uint8_t maxlen)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(sg_device_cert_file_name) > maxlen){
		return AT_ERR_FAILURE;
	}

	memset(pDevCert, '\0', maxlen);
	strncpy(pDevCert, sg_device_cert_file_name, maxlen);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_GetDevCertName is not implement");
	return AT_ERR_FAILURE;
#endif
}

int HAL_GetDevPrivateKeyName(char *pDevPrivateKey, uint8_t maxlen)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(sg_device_privatekey_file_name) > maxlen){
		return AT_ERR_FAILURE;
	}

	memset(pDevPrivateKey, '\0', maxlen);
	strncpy(pDevPrivateKey, sg_device_privatekey_file_name, maxlen);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_GetDevPrivateKeyName is not implement");
	return AT_ERR_FAILURE;
#endif

}

int HAL_SetDevCertName(char *pDevCert)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(pDevCert) > MAX_SIZE_OF_DEVICE_CERT_FILE_NAME){
		return AT_ERR_FAILURE;
	}

	memset(sg_device_cert_file_name, '\0', MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);
	strncpy(sg_device_cert_file_name, pDevCert, MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_SetDevCertName is not implement");
	return AT_ERR_FAILURE;
#endif
}

int HAL_SetDevPrivateKeyName(char *pDevPrivateKey)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(pDevPrivateKey) > MAX_SIZE_OF_DEVICE_KEY_FILE_NAME){
		return AT_ERR_FAILURE;
	}

	memset(sg_device_privatekey_file_name, '\0', MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);
	strncpy(sg_device_privatekey_file_name, pDevPrivateKey, MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_SetDevPrivateKeyName is not implement");
	return AT_ERR_FAILURE;
#endif
}

#else	//PSK 认证方式
int HAL_GetDevSec(char *pDevSec, uint8_t maxlen)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(sg_device_secret) > maxlen){
		return AT_ERR_FAILURE;
	}

	memset(pDevSec, '\0', maxlen);
	strncpy(pDevSec, sg_device_secret, maxlen);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_GetDevSec is not implement");
	return AT_ERR_FAILURE;
#endif


}

int HAL_SetDevSec(const char *pDevSec)
{
#ifdef DEBUG_DEV_INFO_USED
	if(strlen(pDevSec) > MAX_SIZE_OF_DEVICE_SERC){
		return AT_ERR_FAILURE;
	}

	memset(sg_device_secret, '\0', MAX_SIZE_OF_DEVICE_SERC);
	strncpy(sg_device_secret, pDevSec, MAX_SIZE_OF_DEVICE_SERC);

	return AT_ERR_SUCCESS;
#else
	HAL_Printf("HAL_SetDevSec is not implement");
	return AT_ERR_FAILURE;
#endif
}
#endif
