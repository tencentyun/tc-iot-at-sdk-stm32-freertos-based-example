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


static char sg_shadow_update_buffer[AT_CMD_MAX_LEN] = {0};
size_t sg_shadow_update_buffersize = sizeof(sg_shadow_update_buffer) / sizeof(sg_shadow_update_buffer[0]);

static DeviceProperty sg_shadow_property;
static int sg_current_update_count = 0;
static bool sg_delta_arrived = false;
static bool sg_lastupdate_acked = true;


void OnDeltaCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty) {
	int rc = IOT_Shadow_JSON_ConstructDesireAllNull(pClient, sg_shadow_update_buffer, sg_shadow_update_buffersize);

	if (rc == AT_ERR_SUCCESS) {
		sg_delta_arrived = true;
		Log_d("pJsonValueBuffer:%s", pJsonValueBuffer);
	}
	else {
		Log_e("construct desire failed, err: %d", rc);
	}
}

static void OnShadowUpdateCallback(void *pClient, Method method, RequestAck requestAck, const char *pJsonDocument, void *pUserdata) {
	Log_i("recv shadow update response, response ack: %d", requestAck);
	sg_lastupdate_acked = true;
}

static eAtResault net_prepare(void)
{
	eAtResault Ret;
	osThreadId threadId;
	DeviceInfo sDevInfo;
	at_client_t pclient = at_client_get();	

	memset((char *)&sDevInfo, '\0', sizeof(DeviceInfo));
	Ret = (eAtResault)HAL_GetDevInfo(&sDevInfo);

	if(AT_ERR_SUCCESS != Ret){
		Log_e("Get device info err");
		return AT_ERR_FAILURE;
	}
	
	if(AT_ERR_SUCCESS != module_init(eMODULE_WIFI)) 
	{
		Log_e("module init failed");
		goto exit;
	}
	else
	{
		Log_d("module init success");	
	}
	
	//	Parser Func should run in a separate thread
	if((NULL != pclient)&&(NULL != pclient->parser))
	{
		hal_thread_create(&threadId, PARSE_THREAD_STACK_SIZE, osPriorityNormal, pclient->parser, pclient);
	}


	while(AT_STATUS_INITIALIZED != pclient->status)
	{	
		HAL_SleepMs(1000);
	}
	
	Log_d("Start shakehands with module...");
	Ret = module_handshake(CMD_TIMEOUT_MS);
	if(AT_ERR_SUCCESS != Ret)
	{
		Log_e("module connect fail,Ret:%d", Ret);
		goto exit;
	}
	else
	{
		Log_d("module connect success");
	}
	
	Ret = iot_device_info_init(sDevInfo.product_id, sDevInfo.device_name, sDevInfo.devSerc);
	if(AT_ERR_SUCCESS != Ret)
	{
		Log_e("dev info init fail,Ret:%d", Ret);
		goto exit;
	}

	Ret = module_info_set(iot_device_info_get(), ePSK_TLS);
	if(AT_ERR_SUCCESS != Ret)
	{
		Log_e("module info set fail,Ret:%d", Ret);
	}

exit:

	return Ret;
}

void shadow_demo_task(void *arg)
{
	eAtResault Ret;
	int rc;
	void *pShadow_client = NULL;
	at_client_t pclient = at_client_get();	

	Log_d("shadow_demo_task Entry...");

	do  
	{
		Ret = net_prepare();
		if(AT_ERR_SUCCESS != Ret)
		{
			Log_e("net prepare fail,Ret:%d", Ret);
			break;
		}
		
		/*
		 *注意：module_register_network 联网需要根据所选模组适配修改实现
		*/
		Ret = module_register_network(eMODULE_ESP8266);
		if(AT_ERR_SUCCESS != Ret)
		{			
			Log_e("network connect fail,Ret:%d", Ret);
			break;
		}
		
		MQTTInitParams init_params = DEFAULT_MQTTINIT_PARAMS;
		Ret = module_mqtt_conn(init_params);
		if(AT_ERR_SUCCESS != Ret)
		{
			Log_e("module mqtt conn fail,Ret:%d", Ret);
			break;
		}
		else
		{
			Log_d("module mqtt conn success");
		}


		if(!IOT_MQTT_IsConnected())
		{
			Log_e("mqtt connect fail");
			break;
		}

		Ret = IOT_Shadow_Construct(&pShadow_client);
		if(AT_ERR_SUCCESS != Ret)
		{
			Log_e("shadow construct fail,Ret:%d", Ret);
			break;
		}
		else
		{
			Log_d("shadow construct success");
		}

		//注册delta属性
		sg_shadow_property.key = "updateCount";
		sg_shadow_property.data = &sg_current_update_count;
		sg_shadow_property.type = JINT32;
		rc = IOT_Shadow_Register_Property(pShadow_client, &sg_shadow_property, OnDeltaCallback);
		if (rc != AT_ERR_SUCCESS) 
		{
			Log_e("register device shadow property failed, err: %d", rc);
			rc = IOT_Shadow_Destroy(pShadow_client);			
			break;
		}


		//进行Shdaow Update操作的之前，最后进行一次同步操作，否则可能本机上shadow version和云上不一致导致Shadow Update操作失败
		rc = IOT_Shadow_Get_Sync(pShadow_client, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		if (rc != AT_ERR_SUCCESS) 
		{
			Log_e("get device shadow failed, err: %d", rc);
			break;
		}	


		while(1)
		{
			HAL_SleepMs(3000);
			IOT_Shadow_Yield(pShadow_client, 2000);
			if (sg_delta_arrived) 
			{
				rc = IOT_Shadow_Update_Sync(pShadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
				sg_delta_arrived = false;
				if (rc == AT_ERR_SUCCESS) 
				{
					Log_i("shadow update success");
				}			
			}

			if(sg_lastupdate_acked) //make sure vesion is sync
			{
				sg_lastupdate_acked = false;
				IOT_Shadow_JSON_ConstructReport(pShadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, 1, &sg_shadow_property);
				rc = IOT_Shadow_Update(pShadow_client, sg_shadow_update_buffer, sg_shadow_update_buffersize, OnShadowUpdateCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
				sg_current_update_count++;
			}

		}
		
	}while (0);
	
	hal_thread_destroy(NULL);
}

void shadow_sample(void)
{
	osThreadId demo_threadId;
	
#ifdef OS_USED
	hal_thread_create(&demo_threadId, 512, osPriorityNormal, shadow_demo_task, NULL);
	hal_thread_destroy(NULL);
#else
	#error os should be used just now
#endif
}
