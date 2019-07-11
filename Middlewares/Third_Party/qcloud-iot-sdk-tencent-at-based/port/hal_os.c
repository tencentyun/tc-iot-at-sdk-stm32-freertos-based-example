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
#include "stm32f1xx_hal.h"		
#include "qcloud_iot_api_export.h"


#ifdef  DEBUG_DEV_INFO_USED

/* 产品名称, 与云端同步设备状态时需要  */
static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1]	 = "7OCRBY34ND";
/* 设备名称, 与云端同步设备状态时需要 */
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1]  = "dev1";

#ifdef DEV_DYN_REG_ENABLED
/* 产品密钥, 若使能动态注册功能，控制台生成，必填。若不使能，则不用赋值  */
static char sg_product_secret[MAX_SIZE_OF_PRODUCT_SECRET + 1]  = "YOUR_PRODUCT_SECRET";
#endif

#ifdef AUTH_MODE_CERT
/* 客户端证书文件名  非对称加密使用, TLS 证书认证方式*/
static char sg_device_cert_file_name[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1]      = "YOUR_DEVICE_NAME_cert.crt";
/* 客户端私钥文件名 非对称加密使用, TLS 证书认证方式*/
static char sg_device_privatekey_file_name[MAX_SIZE_OF_DEVICE_KEY_FILE_NAME + 1] = "YOUR_DEVICE_NAME_private.key";
#else
/* 设备密钥, TLS PSK认证方式*/
static char sg_device_secret[MAX_SIZE_OF_DEVICE_SERC + 1] = "/Eb0To0fjOVj9TP8o0M4Rg==";
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

int HAL_SetDevInfo(void *pdevInfo)
{
	int ret = AT_ERR_SUCCESS;;
	DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;
	

	if(NULL == pdevInfo){
		return AT_ERR_FAILURE;
	}
	
#ifdef DEBUG_DEV_INFO_USED
	memset(sg_product_id, '\0', MAX_SIZE_OF_PRODUCT_ID);	
	memset(sg_device_name, '\0', MAX_SIZE_OF_DEVICE_NAME);
	
	strncpy(sg_product_id, devInfo->product_id, MAX_SIZE_OF_PRODUCT_ID);
	strncpy(sg_device_name, devInfo->device_name, MAX_SIZE_OF_DEVICE_NAME);
	
#ifdef DEV_DYN_REG_ENABLED
	memset(sg_product_secret, '\0', MAX_SIZE_OF_PRODUCT_SECRET);
	strncpy(sg_product_secret, devInfo->product_secret, MAX_SIZE_OF_PRODUCT_KEY);
#endif
	
#ifdef 	AUTH_MODE_CERT
	memset(sg_device_cert_file_name, '\0', MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);
	memset(sg_device_privatekey_file_name, '\0', MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);
	
	strncpy(sg_device_cert_file_name, devInfo->devCertFileName, MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);
	strncpy(sg_device_privatekey_file_name, devInfo->devPrivateKeyFileName, MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);
#else
	memset(sg_device_secret, '\0', MAX_SIZE_OF_DEVICE_SERC);
	strncpy(sg_device_secret, devInfo->devSerc, MAX_SIZE_OF_DEVICE_SERC);
#endif
		
#else
	 Log_e("HAL_SetDevInfo is not implement");
	 (void)devInfo; //eliminate compile warning

	 return AT_ERR_FAILURE;

#endif

	return ret;
}

int HAL_GetDevInfo(void *pdevInfo)
{
	int ret = AT_ERR_SUCCESS;
	DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;

	if(NULL == pdevInfo){
		return AT_ERR_FAILURE;
	}
	
	memset((char *)devInfo, '\0', sizeof(DeviceInfo));	
	
#ifdef DEBUG_DEV_INFO_USED	

	strncpy(devInfo->product_id, sg_product_id, MAX_SIZE_OF_PRODUCT_ID);
	strncpy(devInfo->device_name, sg_device_name, MAX_SIZE_OF_DEVICE_NAME);
	
#ifdef DEV_DYN_REG_ENABLED
	memset(devInfo->product_secret, '\0', MAX_SIZE_OF_PRODUCT_SECRET);
	strncpy(devInfo->product_secret, sg_product_secret, MAX_SIZE_OF_PRODUCT_SECRET);
#endif	
	
#ifdef 	AUTH_MODE_CERT
	strncpy(devInfo->devCertFileName, sg_device_cert_file_name, MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);
	strncpy(devInfo->devPrivateKeyFileName, sg_device_privatekey_file_name, MAX_SIZE_OF_DEVICE_KEY_FILE_NAME);
#else
	strncpy(devInfo->devSerc, sg_device_secret, MAX_SIZE_OF_DEVICE_SERC);
#endif 

#else
   Log_e("HAL_GetDevInfo is not implement");

   return AT_ERR_FAILURE;
#endif

	return ret;
}


