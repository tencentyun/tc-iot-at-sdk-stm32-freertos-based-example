/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "qcloud_iot_api_export.h"
#include "at_client.h"
#include "string.h"

static void dataTopic_cb(char *msg, void *context)
{
	Log_d("data topic call back:%s",msg);
}

static eAtResault net_prepare(void)
{
	eAtResault Ret;
	DeviceInfo sDevInfo;
	at_client_t pclient = at_client_get();	

	memset((char *)&sDevInfo, '\0', sizeof(DeviceInfo));
	Ret = (eAtResault)HAL_GetDevInfo(&sDevInfo);

	if(QCLOUD_RET_SUCCESS != Ret){
		Log_e("Get device info err");
		return QCLOUD_ERR_FAILURE;
	}
	
	if(QCLOUD_RET_SUCCESS != module_init(eMODULE_WIFI)) 
	{
		Log_e("module init failed");
		goto exit;
	}
	else
	{
		Log_d("module init success");	
	}

	/*at_parse thread should work first*/
	while(AT_STATUS_INITIALIZED != pclient->status)
	{	
		HAL_SleepMs(1000);
	}
	
	Log_d("Start shakehands with module...");
	Ret = module_handshake(CMD_TIMEOUT_MS);
	if(QCLOUD_RET_SUCCESS != Ret)
	{
		Log_e("module connect fail,Ret:%d", Ret);
		goto exit;
	}
	else
	{
		Log_d("module connect success");
	}
	
	Ret = iot_device_info_init(sDevInfo.product_id, sDevInfo.device_name, sDevInfo.devSerc);
	if(QCLOUD_RET_SUCCESS != Ret)
	{
		Log_e("dev info init fail,Ret:%d", Ret);
		goto exit;
	}

	Ret = module_info_set(iot_device_info_get(), ePSK_TLS);
	if(QCLOUD_RET_SUCCESS != Ret)
	{
		Log_e("module info set fail,Ret:%d", Ret);
	}

exit:

	return Ret;
}

void mqtt_demo_task(void *arg)
{
	eAtResault Ret;
	int count = 0;
	char payload[256];


	Log_d("mqtt_demo_task Entry...");
	
	do 
	{
		Ret = net_prepare();
		if(QCLOUD_RET_SUCCESS != Ret)
		{
			Log_e("net prepare fail,Ret:%d", Ret);
			break;
		}
		
		/*
		 *注意：module_register_network 联网需要根据所选模组适配修改实现
		*/
		Ret = module_register_network(eMODULE_ESP8266);
		if(QCLOUD_RET_SUCCESS != Ret)
		{			
			Log_e("network connect fail,Ret:%d", Ret);
			break;
		}
		
		MQTTInitParams init_params = DEFAULT_MQTTINIT_PARAMS;
		Ret = module_mqtt_conn(init_params);
		if(QCLOUD_RET_SUCCESS != Ret)
		{
			Log_e("module mqtt conn fail,Ret:%d", Ret);
			break;
		}
		else
		{
			Log_d("module mqtt conn success");
		}

		static char topic_name[MAX_SIZE_OF_CLOUD_TOPIC] = {0};		
		int size = HAL_Snprintf(topic_name, MAX_SIZE_OF_CLOUD_TOPIC, "%s/%s/data", iot_device_info_get()->product_id, iot_device_info_get()->device_name);
			
		if (size < 0 || size > sizeof(topic_name) - 1)
		{
			Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
			break;
		}
		Ret = module_mqtt_sub(topic_name, QOS0, dataTopic_cb, NULL);
		
		if(QCLOUD_RET_SUCCESS != Ret)
		{
			Log_e("module mqtt sub fail,Ret:%d", Ret);
			break;
		}
		else
		{
			Log_d("module mqtt sub success");
		}


		while(1)
		{
			HAL_SleepMs(3000);
			memset(payload, 0, 256);
#ifdef QUOTES_TRANSFER_NEED			
			HAL_Snprintf(payload, 256, "{\\\"action\\\": \\\"publish_test\\\""T_", \\\"count\\\": \\\"%d\\\"}",count++);
#else
			HAL_Snprintf(payload, 256, "{\"action\": \"publish_test\", \"count\": \"%d\"}",count++);	
#endif
			Log_d("pub_msg:%s", payload);
			Ret = module_mqtt_pub(topic_name, QOS0, payload);
			if(QCLOUD_RET_SUCCESS != Ret)
			{
				Log_e("module mqtt pub fail,Ret:%d", Ret);
				break;
			}
			else
			{
				Log_d("module mqtt pub success");
			}	
			
		}
	
		Log_e("Something goes err,Ret:%d", Ret);
		/*You should do something to recover*/
	}while (0);
	
	hal_thread_destroy(NULL);
}

void mqtt_sample(void)
{
	osThreadId demo_threadId;
	
#ifdef OS_USED
	hal_thread_create(&demo_threadId, 512, osPriorityNormal, mqtt_demo_task, NULL);
	hal_thread_destroy(NULL);
#else
	#error os should be used just now
#endif
}
