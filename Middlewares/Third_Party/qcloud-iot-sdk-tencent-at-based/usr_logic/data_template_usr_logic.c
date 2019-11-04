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
#include "peripheral_drv.h"
#include "utils_timer.h"

#include "data_config.c"

#define CYCLE_TIME_MS		(5000)

static bool sg_control_msg_arrived = false;
static char sg_data_report_buffer[AT_CMD_MAX_LEN];
static size_t sg_data_report_buffersize = sizeof(sg_data_report_buffer) / sizeof(sg_data_report_buffer[0]);
static Timer sg_cycle_timer;

extern uint8_t g_PosRow;

#ifdef EVENT_POST_ENABLED
#include "events_config.c"
#ifdef	EVENT_TIMESTAMP_USED
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
#endif 

static void event_post_cb(char *msg, void *context)
{
	Log_d("eventReply:%s", msg);
	IOT_Event_clearFlag(context, FLAG_EVENT0|FLAG_EVENT1|FLAG_EVENT2);
}

#endif

static void set_propery_changed(void *pProperyData)
{	
	int i;

	for(i = 0; i < TOTAL_PROPERTY_COUNT; i++){
		if(sg_DataTemplate[i].data_property.data == pProperyData){
			sg_DataTemplate[i].state = eCHANGED;
			break;
		}
	}
}

#ifdef ACTION_ENABLED
#include "action_config.c"

// action : regist action and set the action handle callback, add your aciton logic here
static void OnActionCallback(void *pClient, const char *pClientToken, DeviceAction *pAction) 
{
	int i,inv;	
	sReplyPara replyPara;

	//do something base on input, just print as an sample
	DeviceProperty *pActionInput = pAction->pInput;

	//flow led control
	if(!strcmp(pAction->pActionId, "flow_led")){

		/*get input value*/
		for (i = 0; i < pAction->input_num; i++) {		
			if(!strcmp(pActionInput[i].key, "inv")){
				inv = *(int *)pActionInput[i].data;		
				Log_d("input inv:%d", inv); //ms
			}
		}	
		GrayDataSend(255, 0, 0);
		HAL_SleepMs(inv);
		GrayDataSend(0, 255, 0);
		HAL_SleepMs(inv);
		GrayDataSend(0, 0, 255);
		HAL_SleepMs(inv);
		GrayDataSend(0, 0, 0);				
	}	
		
	// construct output 
	memset((char *)&replyPara, 0, sizeof(sReplyPara));
	replyPara.code = eDEAL_SUCCESS;
	replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;						
	strcpy(replyPara.status_msg, "flow_led execute success!"); //add the message about the action resault 
			
	DeviceProperty *pActionOutnput = pAction->pOutput;	
	*(int*)(pActionOutnput[0].data) = 0; //set result 
		
	IOT_ACTION_REPLY(pClient, pClientToken, sg_data_report_buffer, sg_data_report_buffersize, pAction, &replyPara); 	
}

static int _register_data_template_action(void *pTemplate_client)
{
	int i,rc;
	
    for (i = 0; i < TOTAL_ACTION_COUNTS; i++) {
	    rc = IOT_Template_Register_Action(pTemplate_client, &g_actions[i], OnActionCallback);
	    if (rc != QCLOUD_RET_SUCCESS) {
	        rc = IOT_Template_Destroy(pTemplate_client);
	        Log_e("register device data template action failed, err: %d", rc);
	        return rc;
	    } else {
	        Log_i("data template action=%s registered.", g_actions[i].pActionId);
	    }
    }

	return QCLOUD_RET_SUCCESS;
}
#endif

static void OnControlMsgCallback(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty) 
{
    int i = 0;

    for (i = 0; i < TOTAL_PROPERTY_COUNT; i++) {		
        if (strcmp(sg_DataTemplate[i].data_property.key, pProperty->key) == 0) {
            sg_DataTemplate[i].state = eCHANGED;
            Log_i("Property=%s changed", pProperty->key);
            sg_control_msg_arrived = true;
            return;
        }
    }

    Log_e("Property=%s changed no match", pProperty->key);
}

/**
 * 注册数据模板属性
 */
static int _register_data_template_property(void *ptemplate_client)
{
	int i,rc;
	
    for (i = 0; i < TOTAL_PROPERTY_COUNT; i++) {
	    rc = IOT_Template_Register_Property(ptemplate_client, &sg_DataTemplate[i].data_property, OnControlMsgCallback);
	    if (rc != QCLOUD_RET_SUCCESS) {
	        rc = IOT_Template_Destroy(ptemplate_client);
	        Log_e("register device data template property failed, err: %d", rc);
	        return rc;
	    } else {
	        Log_i("data template property=%s registered.", sg_DataTemplate[i].data_property.key);
	    }
    }

	return QCLOUD_RET_SUCCESS;
}

static void OnReportReplyCallback(void *pClient, Method method, ReplyAck replyAck, const char *pJsonDocument, void *pUserdata) {
	Log_i("recv report_reply(ack=%d): %s", replyAck,pJsonDocument);
}


static void cycle_report_info(Timer *pTimer,ProductDataDefine *pWorkshop)
{
	char showstr[64];
	
	/*只读数据定时巡检上报*/
	if(expired(pTimer)){
		
		HumAndTempRead(TEMP_READ, &sg_ProductData.m_tempreture);
		set_propery_changed( &sg_ProductData.m_tempreture);

		HumAndTempRead(HUM_READ, &sg_ProductData.m_humidity);
		set_propery_changed( &sg_ProductData.m_humidity);

		GetLumen((uint32_t *)&sg_ProductData.m_brightness);
		set_propery_changed( &sg_ProductData.m_brightness);
			
		Log_d("Brightness:%d, Temp:%f, Hum:%f",sg_ProductData.m_brightness, sg_ProductData.m_tempreture, sg_ProductData.m_humidity);
		countdown_ms(pTimer, CYCLE_TIME_MS);

		memset(showstr, 0, 64);
		HAL_Snprintf(showstr, 64, "Brightness:%d", sg_ProductData.m_brightness);
		OledShowString(0, &g_PosRow, showstr, CHAR_WID_8, false); 
		memset(showstr, 0, 64);
		HAL_Snprintf(showstr, 64, "Temp:%f", sg_ProductData.m_tempreture);
		OledShowString(0, &g_PosRow, showstr, CHAR_WID_8, false); 
		memset(showstr, 0, 64);
		HAL_Snprintf(showstr, 64, "Hum:%f", sg_ProductData.m_humidity);
		OledShowString(0, &g_PosRow, showstr, CHAR_WID_8, false); 
	}
	
}


/*用户需要实现的下行数据的业务逻辑,待用户实现*/
static void deal_down_stream_user_logic(void *pClient, ProductDataDefine  *pWorkshop)
{
	char showstr[256];
	memset(showstr, 0, 256);
	HAL_Snprintf(showstr, 256, "color red");
	Log_d("color:%d, motor: %d", pWorkshop->m_led, pWorkshop->m_power_switch);
	
	/*灯光颜色*/
	switch(pWorkshop->m_led) {
	    case 0: //red
	        GrayDataSend(255, 0, 0);
			HAL_Snprintf(showstr, 256, "color Red");
	        break;
	    case 1: //green
	        GrayDataSend(0, 255, 0);
			HAL_Snprintf(showstr, 256, "color Green");
	        break;
	    case 2: //blue
	        GrayDataSend(0, 0, 255);
			HAL_Snprintf(showstr, 256, "color Blue");
	        break;
		case 3: //off
	        GrayDataSend(0, 0, 0);
			HAL_Snprintf(showstr, 256, "color Blue");
	        break;	
	    default:
	        GrayDataSend(0, 0, 0);
			HAL_Snprintf(showstr, 256, "led off");
	        break;
	}
	OledShowString(0, &g_PosRow, showstr, CHAR_WID_8, false); 
	
	memset(showstr, 0, 256);
	if(pWorkshop->m_power_switch  > 0){
        /* motor on */
		Log_d("motor on");
		HAL_Snprintf(showstr, 256, "motor on");
		MotorControl(FORWORD_ON);
	}else{
		/* motor off */
		Log_d("motor off");
		HAL_Snprintf(showstr, 256, "motor off");
		MotorControl(MOTOR_OFF);
	}
	OledShowString(0, &g_PosRow, showstr, CHAR_WID_8, false); 
	
#ifdef EVENT_POST_ENABLED
	if(eCHANGED == sg_DataTemplate[1].state){
		if(pWorkshop->m_power_switch > 0){	
			memset(sg_motor_state_status, 0, 64);
			strcpy(sg_motor_state_status,"motor on");

		}else{
			memset(sg_motor_state_status, 0, 64);
			strcpy(sg_motor_state_status,"motor off");			
		}
		IOT_Event_setFlag(pClient, FLAG_EVENT0);
	}
#endif	
}

static int deal_up_stream_user_logic(DeviceProperty *pReportDataList[], int *pCount)
{
	int i, j;

	/*cycle report local info*/
	cycle_report_info(&sg_cycle_timer, &sg_ProductData);

	/*收集发生变化变化的属性*/	
	for (i = 0, j = 0; i < TOTAL_PROPERTY_COUNT; i++) {       
        if(eCHANGED == sg_DataTemplate[i].state) {
            pReportDataList[j++] = &(sg_DataTemplate[i].data_property);
			sg_DataTemplate[i].state = eNOCHANGE;
        }
    }
	*pCount = j;

	return (*pCount > 0)?QCLOUD_RET_SUCCESS:QCLOUD_ERR_FAILURE;		
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
	
	if(QCLOUD_RET_SUCCESS != module_init(eMODULE_L206D)) 
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

static void eventPostCheck(void *client)
{
#ifdef EVENT_POST_ENABLED	
	int rc;
	int i;
	uint32_t eflag;
	sEvent *pEventList[EVENT_COUNTS];
	uint8_t event_count;
		
	//事件上报
	eflag = IOT_Event_getFlag(client);
	if((EVENT_COUNTS > 0 )&& (eflag > 0))
	{	
		event_count = 0;
		for(i = 0; i < EVENT_COUNTS; i++)
		{
		
			if((eflag&(1<<i))&ALL_EVENTS_MASK)
			{
				 pEventList[event_count++] = &(g_events[i]);				 
				 IOT_Event_clearFlag(client, 1<<i);
#ifdef	EVENT_TIMESTAMP_USED				 
				 update_events_timestamp(&g_events[i], 1);
#endif
			}			
		}	

		rc = IOT_Post_Event(client, sg_data_report_buffer, sg_data_report_buffersize, event_count, pEventList, event_post_cb);
		if(rc < 0)
		{
			Log_e("events post failed: %d", rc);
		}
	}
#endif

}

/*用户需要实现设备信息获取,此处示例实现*/
static int _get_sys_info(void *handle, char *pJsonDoc, size_t sizeOfBuffer)
{
	/*平台处理字段，必选字段至少有一个*/
    DeviceProperty plat_info[] = {
     	{.key = "module_hardinfo", .type = TYPE_TEMPLATE_STRING, .data = "ESP8266"},
     	{.key = "module_softinfo", .type = TYPE_TEMPLATE_STRING, .data = "V1.0"},
     	{.key = "fw_ver", 		   .type = TYPE_TEMPLATE_STRING, .data = QCLOUD_IOT_AT_SDK_VERSION},
     	{.key = "imei", 		   .type = TYPE_TEMPLATE_STRING, .data = "11-22-33-44"},
     	{.key = "lat", 			   .type = TYPE_TEMPLATE_STRING, .data = "22.546015"},
     	{.key = "lon", 			   .type = TYPE_TEMPLATE_STRING, .data = "113.941125"},
        {NULL, NULL, JINT32}  //结束
	};
		
	/*自定义附加信息*/
	DeviceProperty self_info[] = {
        {.key = "append_info", .type = TYPE_TEMPLATE_STRING, .data = "your self define ifno"},
        {NULL, NULL, JINT32}  //结束
	};

	return IOT_Template_JSON_ConstructSysInfo(handle, pJsonDoc, sizeOfBuffer, plat_info, self_info); 	
}


void data_template_demo_task(void *arg)
{
	eAtResault Ret;
	int rc;
	int ReportCont;
	void *client = NULL;
	at_client_t pclient = at_client_get();	
	DeviceProperty *pReportDataList[TOTAL_PROPERTY_COUNT];

	Log_d("data_template_demo_task Entry...");
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
		Ret = module_register_network(eMODULE_L206D);
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
		
		if(!IOT_MQTT_IsConnected())
		{
			Log_e("mqtt connect fail");
			break;
		}

		Ret = (eAtResault)IOT_Template_Construct(&client);
		if(QCLOUD_RET_SUCCESS != Ret)
		{
			Log_e("data template construct fail,Ret:%d", Ret);
			break;
		}
		else
		{
			Log_d("data template construct success");
		}

		//init data template
		_init_data_template();

				
		//register data template propertys here
		rc = _register_data_template_property(client);
		if (rc == QCLOUD_RET_SUCCESS) 
		{
			Log_i("Register data template propertys Success");
		} 
		else 
		{
			Log_e("Register data template propertys Failed: %d", rc);
			break;
		}

#ifdef ACTION_ENABLED
		//register data template actions
		rc = _register_data_template_action(client);
		if (rc == QCLOUD_RET_SUCCESS) {
			Log_i("Register data template actions Success");
		} else {
			Log_e("Register data template actions Failed: %d", rc);
			break;
		}
#endif
			
		//上报设备信息,平台根据这个信息提供产品层面的数据分析,譬如位置服务等
		rc = _get_sys_info(client, sg_data_report_buffer, sg_data_report_buffersize);
		if(QCLOUD_RET_SUCCESS == rc)
		{
			rc = IOT_Template_Report_SysInfo_Sync(client, sg_data_report_buffer, sg_data_report_buffersize, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);	
			if (rc != QCLOUD_RET_SUCCESS) 
			{
				Log_e("Report system info fail, err: %d", rc);
				break;
			}
		}
		else
		{
			Log_e("Get system info fail, err: %d", rc);
			break;
		}

		//获取离线期间数据
		rc = IOT_Template_GetStatus_sync(client, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
		if (rc != QCLOUD_RET_SUCCESS) 
		{
			Log_e("Get data status fail, err: %d", rc);
			break;
		}
		else
		{
			Log_d("Get data status success");
		}

	
		//只读属性的定时检测上报
		InitTimer(&sg_cycle_timer);
		countdown_ms(&sg_cycle_timer, CYCLE_TIME_MS); 

		while(1)
		{
			//HAL_SleepMs(1000);
			IOT_Template_Yield(client, 1000);
			
			/*服务端下行消息，业务处理逻辑1入口*/
			if (sg_control_msg_arrived) {	
				
				deal_down_stream_user_logic(client, &sg_ProductData);				
				//业务逻辑处理完后需要通知服务端control msg 已收到，请服务端删除control msg，否则服务端会保留control msg(通过Get status命令可以得到未删除的Control数据)
				sReplyPara replyPara;
				memset((char *)&replyPara, 0, sizeof(sReplyPara));
				replyPara.code = eDEAL_SUCCESS;
				replyPara.timeout_ms = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;						
				replyPara.status_msg[0] = '\0';			//可以通过 replyPara.status_msg 添加附加消息，一般在失败情况下才添加
				
				rc = IOT_Template_ControlReply(client, sg_data_report_buffer, sg_data_report_buffersize, &replyPara);
	            if (rc == QCLOUD_RET_SUCCESS) {
					Log_d("Contol msg reply success");
					sg_control_msg_arrived = false;   
	            } else {
	                Log_e("Contol msg reply failed, err: %d", rc);
					break;
	            }				
			}

			/*设备上行消息,业务逻辑2入口*/				
			/*上报属性的最新状态，应用侧可以通过report的状态确认control的命令是否执行成功*/
			if(QCLOUD_RET_SUCCESS == deal_up_stream_user_logic(pReportDataList, &ReportCont)){
				
				rc = IOT_Template_JSON_ConstructReportArray(client, sg_data_report_buffer, sg_data_report_buffersize, ReportCont, pReportDataList);
				if (rc == QCLOUD_RET_SUCCESS) {
					rc = IOT_Template_Report(client, sg_data_report_buffer, sg_data_report_buffersize, \
													OnReportReplyCallback, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
					if (rc == QCLOUD_RET_SUCCESS) {
						Log_d("data template reporte success");
					} else {
						Log_e("data template reporte failed, err: %d", rc);
						break;
					}
				} else {
					Log_e("construct reporte data failed, err: %d", rc);
					break;
				}
			}			
			eventPostCheck(client);
		}				
	}while (0);
		
	Log_e("Task teminated,Something goes wrong!!!");
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

