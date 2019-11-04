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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <string.h>
#include "qcloud_iot_api_export.h"




static DeviceInfo   sg_device_info;

eAtResault iot_device_info_init(const char *product_id, const char *device_name, const char *device_serc)
{	

	memset(&sg_device_info, 0x0, sizeof(DeviceInfo));
	if ((MAX_SIZE_OF_PRODUCT_ID) < strlen(product_id))
	{
		Log_e("product name(%s) length:(%lu) exceeding limitation", product_id, strlen(product_id));
		return QCLOUD_ERR_FAILURE;
	}
	if ((MAX_SIZE_OF_DEVICE_NAME) < strlen(device_name))
	{
		Log_e("device name(%s) length:(%lu) exceeding limitation", device_name, strlen(device_name));
		return QCLOUD_ERR_FAILURE;
	}

	if ((MAX_SIZE_OF_DEVICE_SERC) < strlen(device_serc))
	{
		Log_e("device serc exceeding limitation");
		return QCLOUD_ERR_FAILURE;
	}

	strncpy(sg_device_info.product_id, product_id, MAX_SIZE_OF_PRODUCT_ID);
	strncpy(sg_device_info.device_name, device_name, MAX_SIZE_OF_DEVICE_NAME);
	strncpy(sg_device_info.devSerc, device_serc, MAX_SIZE_OF_DEVICE_SERC);

	/* construct device-id(@product_id+@device_name) */
	memset(sg_device_info.client_id, 0x0, MAX_SIZE_OF_CLIENT_ID);
    int ret = HAL_Snprintf(sg_device_info.client_id, MAX_SIZE_OF_CLIENT_ID, "%s%s", product_id, device_name);
    if ((ret < 0) || (ret >= MAX_SIZE_OF_CLIENT_ID)) {
        Log_e("set device info failed");
        return QCLOUD_ERR_FAILURE;
    }

    Log_i("SDK_Ver: %s, Product_ID: %s, Device_Name: %s", QCLOUD_IOT_AT_SDK_VERSION, product_id, device_name);

	return QCLOUD_RET_SUCCESS;
}

DeviceInfo* iot_device_info_get(void)
{
    return &sg_device_info;
}

#ifdef __cplusplus
}
#endif
