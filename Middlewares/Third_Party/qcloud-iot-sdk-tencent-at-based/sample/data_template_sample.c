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
#include "lite-utils.h"
#include "at_client.h"
#include "string.h"
#include "data_config.c"


static bool sg_delta_arrived = false;
static bool sg_dev_report_new_data = false;


static char sg_shadow_update_buffer[1024];
static size_t sg_shadow_update_buffersize = sizeof(sg_shadow_update_buffer) / sizeof(sg_shadow_update_buffer[0]);

#ifdef EVENT_POST_ENABLED

#include "events_config.c"
static void update_events_timestamp(sEvent *pEvents, int count)
{
	int i;
	
	for(i = 0; i < count; i++){
        if (NULL == (&pEvents[i])) { 
	        Log_e("null event pointer"); 
	        return; 
        }
		pEvents[i].timestamp = HAL_GetTimeSeconds();
	}
}

static void event_post_cb(char *msg)
{
	Log_d("Reply:%s", msg);
	clearEventFlag(FLAG_EVENT0|FLAG_EVENT1|FLAG_EVENT2);
}

#endif



/*如果有自定义的字符串或者json，需要在这里解析*/
static int update_self_define_value(const char *pJsonDoc, DeviceProperty *pProperty) 
{
    int rc = AT_ERR_SUCCESS;
		
	if((NULL == pJsonDoc)||(NULL == pProperty)){
		return AT_ERR_NULL;
	}
	
	/*convert const char* to char * */
	char *pTemJsonDoc =HAL_Malloc(strlen(pJsonDoc));
	strcpy(pTemJsonDoc, pJsonDoc);

	char* property_data = LITE_json_value_of(pProperty->key, pTemJsonDoc);	
	
    if(property_data != NULL){
		if(pProperty->type == TYPE_TEMPLATE_STRING){
			/*如果多个字符串属性,根据pProperty->key值匹配，处理字符串*/			
			Log_d("string type wait to be deal,%s", property_data);
		}else if(pProperty->type == TYPE_TEMPLATE_JOBJECT){
			Log_d("Json type wait to be deal,%s",property_data);	
		}
		
		HAL_Free(property_data);
    }else{
		
		rc = AT_ERR_FAILURE;
		Log_d("Property:%s no matched",pProperty->key);	
	}
	
	HAL_Free(pTemJsonDoc);
		
    return rc;
}


static void OnDeltaTemplateCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty) 
{
    int i = 0;

    for (i = 0; i < TOTAL_PROPERTY_COUNT; i++) {
		/*其他数据类型已经在_handle_delta流程统一处理了，字符串和json串需要在这里处理，因为只有产品自己才知道string/json的自定义解析*/
        if (strcmp(sg_DataTemplate[i].data_property.key, pProperty->key) == 0) {
            sg_DataTemplate[i].state = eCHANGED;
			if((sg_DataTemplate[i].data_property.type == TYPE_TEMPLATE_STRING)
				||(sg_DataTemplate[i].data_property.type == TYPE_TEMPLATE_JOBJECT)){

				update_self_define_value(pJsonValueBuffer, &(sg_DataTemplate[i].data_property));
			}
		
            Log_i("Property=%s changed", pProperty->key);
            sg_delta_arrived = true;
            return;
        }
    }

    Log_e("Property=%s changed no match", pProperty->key);
}


/**
 * 注册数据模板属性
 */
static int _register_data_template_property(void *pshadow_client)
{
	int i,rc;
	
    for (i = 0; i < TOTAL_PROPERTY_COUNT; i++) {
	    rc = IOT_Shadow_Register_Property(pshadow_client, &sg_DataTemplate[i].data_property, OnDeltaTemplateCallback);
	    if (rc != AT_ERR_SUCCESS) {
	        rc = IOT_Shadow_Destroy(pshadow_client);
	        Log_e("register device data template property failed, err: %d", rc);
	        return rc;
	    } else {
	        Log_i("data template property=%s registered.", sg_DataTemplate[i].data_property.key);
	    }
    }

	return AT_ERR_SUCCESS;
}

/*用户需要实现的下行数据的业务逻辑,待用户实现*/
static void deal_down_stream_user_logic(ProductDataDefine   * pData)
{
	Log_d("someting about your own product logic wait to be done");
}

/*用户需要实现的上行数据的业务逻辑,此处仅供示例*/
static int deal_up_stream_user_logic(DeviceProperty *pReportDataList[], int *pCount)
{
	int i, j;
	
     for (i = 0, j = 0; i < TOTAL_PROPERTY_COUNT; i++) {       
        if(eCHANGED == sg_DataTemplate[i].state) {
            pReportDataList[j++] = &(sg_DataTemplate[i].data_property);
			sg_DataTemplate[i].state = eNOCHANGE;
        }
    }
	*pCount = j;

	return (*pCount > 0)?AT_ERR_SUCCESS:AT_ERR_FAILURE;
}


static void OnShadowUpdateCallback(void *pClient, Method method, RequestAck requestAck, const char *pJsonDocument, void *pUserdata) {
	Log_i("recv shadow update response, response ack: %d", requestAck);
}


static eAtResault net_prepare(void)
{
	eAtResault Ret;
	osThreadId threadId;
	DeviceInfo sDevInfo;
	at_client_t pclient = at_client_get();	

	memset((char *)&sDevInfo, 0, sizeof(DeviceInfo));
	Ret = (eAtResault)HAL_GetProductID(sDevInfo.product_id, MAX_SIZE_OF_PRODUCT_ID);
	Ret |= (eAtResault)HAL_GetDevName(sDevInfo.device_name, MAX_SIZE_OF_DEVICE_NAME);
	Ret |= (eAtResault)HAL_GetDevSec(sDevInfo.devSerc, MAX_SIZE_OF_DEVICE_SERC);
	
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

static void eventPostCheck(void)
{
#ifdef EVENT_POST_ENABLED	
	int rc;
	int i;
	uint32_t eflag;
	sEvent *pEventList[EVENT_COUNTS];
	uint8_t event_count;
		
	//事件上报
	setEventFlag(FLAG_EVENT0|FLAG_EVENT1|FLAG_EVENT2);
	eflag = getEventFlag();
	if((EVENT_COUNTS > 0 )&& (eflag > 0))
	{	
		event_count = 0;
		for(i = 0; i < EVENT_COUNTS; i++)
		{
			if((eflag&(1<<i))&ALL_EVENTS_MASK)
			{
				 pEventList[event_count++] = &(g_events[i]);
				 update_events_timestamp(&g_events[i], 1);
				 clearEventFlag(1<<i);
			}
		}	

		rc = qcloud_iot_post_event(get_shadow_client(), sg_shadow_update_buffer, sg_shadow_update_buffersize, \
											event_count, pEventList, event_post_cb);
		if(rc < 0)
		{
			Log_e("events post failed: %d", rc);
		}
	}
#endif

}

void data_template_demo_task(void *arg)
{
	eAtResault Ret;
	int rc;
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


		Ret = IOT_Shadow_Construct(eTEMPLATE);
		if(AT_ERR_SUCCESS != Ret)
		{
			Log_e("shadow construct fail,Ret:%d", Ret);
			break;
		}
		else
		{
			Log_d("shadow construct success");
		}

		//init data template
		_init_data_template();

	
#ifdef EVENT_POST_ENABLED
		rc = event_init(get_shadow_client());
		if (rc < 0) 
		{
			Log_e("event init failed: %d", rc);
			break;
		}
#endif
			
		//register data template propertys here
		rc = _register_data_template_property(get_shadow_client());
		if (rc == AT_ERR_SUCCESS) 
		{
			Log_i("Register data template propertys Success");
		} 
		else 
		{
			Log_e("Register data template propertys Failed: %d", rc);
			break;
		}


		//进行Shdaow Update操作的之前，最后进行一次同步操作，否则可能本机上shadow version和云上不一致导致Shadow Update操作失败
		rc = IOT_Shadow_Get_Sync(get_shadow_client(), QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		if (rc != AT_ERR_SUCCESS) 
		{
			Log_e("get device shadow failed, err: %d", rc);
			break;
		}	


		while(1)
		{
			HAL_SleepMs(1000);
			IOT_Shadow_Yield(2000);
			
			/*服务端下行消息，业务处理逻辑1入口*/
			if (sg_delta_arrived) 
			{						
		       	deal_down_stream_user_logic(&sg_ProductData);

				/*业务逻辑处理完后需要同步通知服务端:设备数据已更新，删除dseire数据*/	
	            int rc = IOT_Shadow_JSON_ConstructDesireAllNull(get_shadow_client(), sg_shadow_update_buffer, sg_shadow_update_buffersize);
	            if (rc == AT_ERR_SUCCESS) 
				{
	                rc = IOT_Shadow_Update_Sync(get_shadow_client(), sg_shadow_update_buffer, sg_shadow_update_buffersize, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	                sg_delta_arrived = false;
	                if (rc == AT_ERR_SUCCESS) 
					{
	                    Log_i("shadow update(desired) success");
	                } 
					else 
					{
	                    Log_e("shadow update(desired) failed, err: %d", rc);
	                }

	            } 
				else 
				{
	                Log_e("construct desire failed, err: %d", rc);
	            }

				sg_dev_report_new_data = true; //用户需要根据业务情况修改上报flag的赋值位置,此处仅为示例
			}	
			else
			{			
				Log_d("No delta msg received...");
			}

			/*设备上行消息,业务逻辑2入口*/
			if(sg_dev_report_new_data)
			{
				
				DeviceProperty *pReportDataList[TOTAL_PROPERTY_COUNT];
				int ReportCont;

				/*delta消息是属性的desire和属性的report的差异集，收到deseire消息处理后，要report属性的状态*/	
				if(AT_ERR_SUCCESS == deal_up_stream_user_logic(pReportDataList, &ReportCont))
				{
					
					rc = IOT_Shadow_JSON_ConstructReportArray(get_shadow_client(), sg_shadow_update_buffer, sg_shadow_update_buffersize, ReportCont, pReportDataList);
			        if (rc == AT_ERR_SUCCESS) 
					{
			            rc = IOT_Shadow_Update(get_shadow_client(), sg_shadow_update_buffer, sg_shadow_update_buffersize, 
			                    OnShadowUpdateCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
			            if (rc == AT_ERR_SUCCESS) 
						{
							sg_dev_report_new_data = false;
			                Log_i("shadow update(reported) success");
			            } 
						else 
						{
			                Log_e("shadow update(reported) failed, err: %d", rc);
			            }
			        }
					else 
					{
			            Log_e("construct reported failed, err: %d", rc);
			        }

				}
				else
				{
					 Log_d("no data need to be reported or someting goes wrong");
				}
			}else{
				Log_d("No device data need to be reported...");
			}
			
			eventPostCheck();
		}				
	}while (0);
	
	hal_thread_destroy(NULL);
}

void data_template_sample(void)
{
	osThreadId demo_threadId;
	
#ifdef OS_USED
	hal_thread_create(&demo_threadId, 512, osPriorityNormal, data_template_demo_task, NULL);
	hal_thread_destroy(NULL);
#else
	#error os should be used just now
#endif
}

