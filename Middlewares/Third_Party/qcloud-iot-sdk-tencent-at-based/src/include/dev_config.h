/**
 ******************************************************************************
 * @file			:  dev_config.h
 * @brief			:  dev config head file
 ******************************************************************************
 *
 * History:      <author>          <time>        <version>
 *               yougaliu          2019-3-20        1.0
 * Desc:          ORG.
 * Copyright (c) 2019 Tencent. 
 * All rights reserved.
 ******************************************************************************
 */
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_
#include "hal_export.h"
#include "qcloud_iot_export_mqtt.h"

typedef struct {
	char	product_id[MAX_SIZE_OF_PRODUCT_ID + 1];
	char 	device_name[MAX_SIZE_OF_DEVICE_NAME + 1];
	char	client_id[MAX_SIZE_OF_CLIENT_ID + 1];
	
#ifdef AUTH_MODE_CERT
	char  	devCertFileName[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1];
	char 	devPrivateKeyFileName[MAX_SIZE_OF_DEVICE_KEY_FILE_NAME + 1];
#else
	char	devSerc[MAX_SIZE_OF_DEVICE_SERC + 1];
#endif

#ifdef DEV_DYN_REG_ENABLED
	char	product_secret[MAX_SIZE_OF_PRODUCT_SECRET + 1];
#endif  
} DeviceInfo;

DeviceInfo* iot_device_info_get(void);
eAtResault iot_device_info_init(const char *product_id, const char *device_name, const char *device_serc);

#endif
