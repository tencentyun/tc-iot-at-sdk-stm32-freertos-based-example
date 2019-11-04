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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "data_template_client_common.h"
#include "data_template_client_json.h"
#include "data_template_client.h"
#include "data_template_action.h"
#include "qcloud_iot_api_export.h"



static Qcloud_IoT_Template sg_data_template_client = {0};

void *get_template_client(void)
{
	return (void *)&sg_data_template_client;
}

bool IOT_Template_IsConnected(void *handle)
{
	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);

	IOT_FUNC_EXIT_RC(IOT_MQTT_IsConnected())
}

static void _handle_message(void) 
{
	Qcloud_IoT_Template *template_client = get_template_client();
	//HAL_MutexLock(template_client->mutex);
	List *msg_list = template_client->inner_data.pMsgList;

    if (msg_list->len) {
			  ListIterator *iter;
        ListNode *node = NULL;

        if (NULL == (iter = list_iterator_new(msg_list, LIST_TAIL))) {
            //HAL_MutexUnlock(template_client->mutex);
            IOT_FUNC_EXIT;
        }

        for (;;) {
            node = list_iterator_next(iter);
            if (NULL == node) {
                break;
            }

            if (NULL == node->val) {
                Log_e("node's value is invalid!");
                continue;
            }
			DownStreamMsg *pMsg =  (DownStreamMsg *)node->val;		
			deliver_message(pMsg->data, pMsg->size);
			HAL_Free(pMsg->data);
			list_remove(msg_list, node);
		}
		list_iterator_destroy(iter);
    }
		
   //HAL_MutexUnlock(template_client->mutex);
}

void IOT_Template_Message_Arrived_CallBack(const char *data, int size) 
{	
	Qcloud_IoT_Template *template_client = get_template_client();
	
	DownStreamMsg *node = HAL_Malloc(sizeof(DownStreamMsg));
	POINTER_SANITY_CHECK_RTN(node);
	
	node->data = HAL_Malloc(size + 1);
	POINTER_SANITY_CHECK_RTN(node->data);
	
	node->size = size;
	strncpy(node->data, data, size);
	node->data[size] = '\0';
	
	list_lpush(template_client->inner_data.pMsgList, list_node_new(node)); //放入请求
}

int IOT_Template_Register_Property(void *handle, DeviceProperty *pProperty, OnPropRegCallback callback) {

    IOT_FUNC_ENTRY;
	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template*)handle;
	int rc;

    if (IOT_MQTT_IsConnected() == false) {
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
    }

	if (template_common_check_property_existence(ptemplate, pProperty)) 
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_PROPERTY_EXIST);

    rc = template_common_register_property_on_delta(ptemplate, pProperty, callback);

    IOT_FUNC_EXIT_RC(rc);
}

int IOT_Template_UnRegister_Property(void *handle, DeviceProperty *pProperty) {
	IOT_FUNC_ENTRY;
	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template*)handle;

	if (IOT_MQTT_IsConnected() == false) {
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
    }

	if (!template_common_check_property_existence(ptemplate, pProperty)) {
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_PROPERTY_NOT_EXIST);
    }
	int rc =  template_common_remove_property(ptemplate, pProperty);
	IOT_FUNC_EXIT_RC(rc);
}

#ifdef ACTION_ENABLED
int IOT_Template_Register_Action(void *handle, DeviceAction *pAction, OnActionHandleCallback callback) 
{
	IOT_FUNC_ENTRY;
	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template*)handle;
	int rc;

	if (IOT_MQTT_IsConnected() == false) {
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
    }

    rc = IOT_Action_Register(ptemplate, pAction, callback);
    IOT_FUNC_EXIT_RC(rc);
}

int IOT_Template_UnRegister_Action(void *handle, DeviceAction *pAction) 
{
	IOT_FUNC_ENTRY;
	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template*)handle;
	int rc; 

	if (IOT_MQTT_IsConnected() == false) {
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
    }


	rc = IOT_Action_Remove(ptemplate, pAction);
	IOT_FUNC_EXIT_RC(rc);
}
#endif

static void _reply_ack_cb(void *pClient, Method method, ReplyAck replyAck, const char *pReceivedJsonDocument, void *pUserdata) 
{
	Request *request = (Request *)pUserdata;
	Log_d("replyAck=%d", replyAck);

    if (NULL != pReceivedJsonDocument) {
        Log_d("Received Json Document=%s", pReceivedJsonDocument);
    } else {
        Log_d("Received Json Document is NULL");
    }

    *((ReplyAck *)(request->user_context))= replyAck;
}

/*control data may be for get status replay*/
static void _get_status_reply_ack_cb(void *pClient, Method method, ReplyAck replyAck, const char *pReceivedJsonDocument, void *pUserdata) 
{
	Request *request = (Request *)pUserdata;
	
	Log_d("replyAck=%d", replyAck);
	if (NULL != pReceivedJsonDocument) {
        Log_d("Received Json Document=%s", pReceivedJsonDocument);
    } else {
        Log_d("Received Json Document is NULL");
    }

	if (*((ReplyAck *)request->user_context) == ACK_ACCEPTED){	
		IOT_Template_ClearControl(pClient, request->client_token, NULL, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);	
	}else{
		*((ReplyAck *)request->user_context)= replyAck;
	}			
}

int IOT_Template_ClearControl(void *handle, char *pClientToken, OnRequestCallback callback, uint32_t timeout_ms) 
{

    IOT_FUNC_ENTRY;
	int rc;
	char JsonDoc[MAX_CLEAE_DOC_LEN];
	
	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pClientToken, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* template_client = (Qcloud_IoT_Template*)handle;

	if (IOT_MQTT_IsConnected() == false) {
		Log_e("template is disconnected");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}

	// 如果没有之前没有订阅$thing/down/property成功，再一次订阅
	if (template_client->inner_data.sync_status < 0) {
		rc = subscribe_template_downstream_topic(template_client);
		if (rc < 0)
		{
			Log_e("Subcribe $thing/down/property fail!");
		}
	}
	
	memset(JsonDoc, 0, MAX_CLEAE_DOC_LEN);
#ifdef QUOTES_TRANSFER_NEED
	int rc_of_snprintf = HAL_Snprintf(JsonDoc, MAX_CLEAE_DOC_LEN, "{\\\"clientToken\\\":\\\"%s\\\"}", pClientToken);
#else
	int rc_of_snprintf = HAL_Snprintf(JsonDoc, MAX_CLEAE_DOC_LEN, "{\"clientToken\":\"%s\"}", pClientToken);
#endif
	rc = check_snprintf_return(rc_of_snprintf, MAX_CLEAE_DOC_LEN);	
    if (rc != QCLOUD_RET_SUCCESS) {
        return rc;
    }

	RequestParams request_params = DEFAULT_REQUEST_PARAMS;	
	init_request_params(&request_params, CLEAR, callback, NULL, timeout_ms/1000);
	
	rc = send_template_request(template_client, &request_params, JsonDoc, MAX_CLEAE_DOC_LEN);
	
	IOT_FUNC_EXIT_RC(rc);
}


int IOT_Template_JSON_ConstructReportArray(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, DeviceProperty *pDeviceProperties[]) 
{		
	POINTER_SANITY_CHECK(jsonBuffer, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pDeviceProperties, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template*)handle;
	POINTER_SANITY_CHECK(ptemplate, QCLOUD_ERR_INVAL);
	
    size_t remain_size = 0;
    int32_t rc_of_snprintf = 0;
	int rc;
    int8_t i;


	build_empty_json(&(ptemplate->inner_data.token_num), jsonBuffer);
	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }
#ifdef QUOTES_TRANSFER_NEED
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, ""T_", \\\"params\\\":{");
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, ", \"params\":{");
#endif
    rc = check_snprintf_return(rc_of_snprintf, remain_size);

    if (rc != QCLOUD_RET_SUCCESS) {
        return rc;
    }

    for (i = 0; i < count; i++) {
		DeviceProperty *pJsonNode = pDeviceProperties[i];
        if (pJsonNode != NULL && pJsonNode->key != NULL) {
            rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);

            if (rc != QCLOUD_RET_SUCCESS) {
                return rc;
            }
        } else {
            return QCLOUD_ERR_INVAL;
        }
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }

#ifdef COMMA_TRANSFER_NEED		
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 2, remain_size, "}}");
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "}}");
#endif

    rc = check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != QCLOUD_RET_SUCCESS) {
		Log_e("construct datatemplate report array failed: %d", rc);
		return rc;
	}

	return rc;
}


int IOT_Template_Report(void *handle, char *pJsonDoc, size_t sizeOfBuffer, OnReplyCallback callback, void *userContext, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, QCLOUD_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* template_client = (Qcloud_IoT_Template*)handle;

	if (IOT_MQTT_IsConnected() == false) {
		Log_e("template is disconnected");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}

	// 如果没有之前没有订阅$thing/down/property成功，再一次订阅
	if (template_client->inner_data.sync_status < 0) {
		rc = subscribe_template_downstream_topic(template_client);
		if (rc < 0)
		{
			Log_e("Subcribe $thing/down/property fail!");
		}
	}

	//Log_d("Report Document: %s", pJsonDoc);

	RequestParams request_params = DEFAULT_REQUEST_PARAMS;
	init_request_params(&request_params, REPORT, callback, userContext, timeout_ms/1000);

	rc = send_template_request(template_client, &request_params, pJsonDoc, sizeOfBuffer);
	IOT_FUNC_EXIT_RC(rc);
}

int IOT_Template_Report_Sync(void *handle, char *pJsonDoc, size_t sizeOfBuffer, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc = QCLOUD_RET_SUCCESS;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, QCLOUD_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, QCLOUD_ERR_INVAL);


	if (IOT_MQTT_IsConnected() == false) {
		Log_e("template is disconnected");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}

	ReplyAck ack_report = ACK_NONE;
	rc = IOT_Template_Report(handle, pJsonDoc, sizeOfBuffer, _reply_ack_cb, &ack_report, timeout_ms);
	if (rc != QCLOUD_RET_SUCCESS) IOT_FUNC_EXIT_RC(rc);

	while (ACK_NONE == ack_report) {
        IOT_Template_Yield(handle, TEMPLATE_SYNC_TIMEOUT);
    }

	if (ACK_ACCEPTED == ack_report) {
        rc = QCLOUD_RET_SUCCESS;
    } else if (ACK_TIMEOUT == ack_report) {
        rc = QCLOUD_ERR_REPORT_TIMEOUT;
    } else if (ACK_REJECTED == ack_report) {
        rc = QCLOUD_ERR_REPORT_REJECTED;
    }
	
	IOT_FUNC_EXIT_RC(rc);
}


int IOT_Template_JSON_ConstructSysInfo(void *handle, char *jsonBuffer, size_t sizeOfBuffer, DeviceProperty *pPlatInfo, DeviceProperty *pSelfInfo) 
{		
	POINTER_SANITY_CHECK(jsonBuffer, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pPlatInfo, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template*)handle;
	POINTER_SANITY_CHECK(ptemplate, QCLOUD_ERR_INVAL);
	
    size_t remain_size = 0;
    int32_t rc_of_snprintf = 0;
	int rc;
	uint8_t pos;
	
#ifdef COMMA_TRANSFER_NEED	
	pos = 2;
#else
	pos = 1;
#endif


	build_empty_json(&(ptemplate->inner_data.token_num), jsonBuffer);
	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }
#ifdef QUOTES_TRANSFER_NEED
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, ""T_", \\\"params\\\":{");
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, ",\"params\":{");
#endif
    rc = check_snprintf_return(rc_of_snprintf, remain_size);

    if (rc != QCLOUD_RET_SUCCESS) {
        return rc;
    }

	DeviceProperty *pJsonNode = pPlatInfo;
    while((NULL != pJsonNode)&&(NULL != pJsonNode->key)) {	
        rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);
        if (rc != QCLOUD_RET_SUCCESS) {
            return rc;
        }
   		pJsonNode++;
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }

    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - pos, remain_size, "}"T_",");
    rc = check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != QCLOUD_RET_SUCCESS) {
		return rc;
	}
	
    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }

	pJsonNode = pSelfInfo;
	if((NULL == pJsonNode)||(NULL == pJsonNode->key)){
		Log_d("No self define info");
		goto end;
	}
#ifdef QUOTES_TRANSFER_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - pos - 1, remain_size, ""T_",\\\"device_label\\\":{");
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - pos - 1, remain_size, ",\"device_label\":{");
#endif
    rc = check_snprintf_return(rc_of_snprintf, remain_size);

    if (rc != QCLOUD_RET_SUCCESS) {
        return rc;
    }

    while((NULL != pJsonNode)&&(NULL != pJsonNode->key)) {	
        rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);
        if (rc != QCLOUD_RET_SUCCESS) {
            return rc;
        }
   		pJsonNode++;
    }

    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - pos, remain_size, "}}"T_",");
    rc = check_snprintf_return(rc_of_snprintf, remain_size);
	if (rc != QCLOUD_RET_SUCCESS) {
		return rc;
	}

end:
	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - pos, remain_size, "}");
    rc = check_snprintf_return(rc_of_snprintf, remain_size);

	return rc;
}

int IOT_Template_Report_SysInfo(void *handle, char *pJsonDoc, size_t sizeOfBuffer, OnReplyCallback callback, void *userContext, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, QCLOUD_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* template_client = (Qcloud_IoT_Template*)handle;

	if (IOT_MQTT_IsConnected() == false) {
		Log_e("template is disconnected");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}

	// 如果没有之前没有订阅$thing/down/property成功，再一次订阅
	if (template_client->inner_data.sync_status < 0) {
		rc = subscribe_template_downstream_topic(template_client);
		if (rc < 0)
		{
			Log_e("Subcribe $thing/down/property fail!");
		}
	}

	//Log_d("INFO Document: %s", pJsonDoc);

	RequestParams request_params = DEFAULT_REQUEST_PARAMS;
	init_request_params(&request_params, RINFO, callback, userContext, timeout_ms/1000);

	rc = send_template_request(template_client, &request_params, pJsonDoc, sizeOfBuffer);
	IOT_FUNC_EXIT_RC(rc);
}

int IOT_Template_Report_SysInfo_Sync(void *handle, char *pJsonDoc, size_t sizeOfBuffer, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc = QCLOUD_RET_SUCCESS;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, QCLOUD_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, QCLOUD_ERR_INVAL);


	if (IOT_MQTT_IsConnected() == false) {
		Log_e("template is disconnected");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}

	ReplyAck ack_report = ACK_NONE;
	rc = IOT_Template_Report_SysInfo(handle, pJsonDoc, sizeOfBuffer, _reply_ack_cb, &ack_report, timeout_ms);
	if (rc != QCLOUD_RET_SUCCESS) IOT_FUNC_EXIT_RC(rc);

	while (ACK_NONE == ack_report) {
        IOT_Template_Yield(handle, TEMPLATE_SYNC_TIMEOUT);
    }

	if (ACK_ACCEPTED == ack_report) {
        rc = QCLOUD_RET_SUCCESS;
    } else if (ACK_TIMEOUT == ack_report) {
        rc = QCLOUD_ERR_REPORT_TIMEOUT;
    } else if (ACK_REJECTED == ack_report) {
        rc = QCLOUD_ERR_REPORT_REJECTED;
    }
	
	IOT_FUNC_EXIT_RC(rc);
}


int IOT_Template_GetStatus(void *handle, OnReplyCallback callback, void *userContext, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(callback, QCLOUD_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* template_client = (Qcloud_IoT_Template*)handle;

	if (IOT_MQTT_IsConnected() == false) {
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}


	// 如果没有之前没有订阅$thing/down/property成功，再一次订阅
	if (template_client->inner_data.sync_status < 0) {
		rc = subscribe_template_downstream_topic(template_client);
		if (rc < 0)
		{
			Log_e("Subcribe $thing/down/property fail!");
		}
	}

	char getRequestJsonDoc[MAX_SIZE_OF_JSON_WITH_CLIENT_TOKEN];
	build_empty_json(&(template_client->inner_data.token_num), getRequestJsonDoc);
	
	//Log_d("GET Status Document: %s", getRequestJsonDoc);

	RequestParams request_params = DEFAULT_REQUEST_PARAMS;
	init_request_params(&request_params, GET, callback, userContext, timeout_ms/1000);

	rc = send_template_request(template_client, &request_params, getRequestJsonDoc, MAX_SIZE_OF_JSON_WITH_CLIENT_TOKEN);
	IOT_FUNC_EXIT_RC(rc);
} 

int IOT_Template_GetStatus_sync(void *handle, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc = QCLOUD_RET_SUCCESS;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, QCLOUD_ERR_INVAL);


	if (IOT_MQTT_IsConnected() == false) {
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}

	ReplyAck ack_request = ACK_NONE;
	rc = IOT_Template_GetStatus(handle, _get_status_reply_ack_cb, &ack_request, timeout_ms);
	if (rc != QCLOUD_RET_SUCCESS) IOT_FUNC_EXIT_RC(rc);
	
	while (ACK_NONE == ack_request) {
        IOT_Template_Yield(handle, TEMPLATE_SYNC_TIMEOUT);
    }

	if (ACK_ACCEPTED == ack_request) {
        rc = QCLOUD_RET_SUCCESS;
    } else if (ACK_TIMEOUT == ack_request) {
        rc = QCLOUD_ERR_GET_TIMEOUT;
    } else if (ACK_REJECTED == ack_request) {
        rc = QCLOUD_ERR_GET_REJECTED;
    }

	IOT_FUNC_EXIT_RC(rc);
}

static int _template_ConstructControlReply(char *jsonBuffer, size_t sizeOfBuffer, sReplyPara *replyPara) 
{
	
	POINTER_SANITY_CHECK(jsonBuffer, QCLOUD_ERR_INVAL);

	int rc;
	size_t remain_size = 0;
	int32_t rc_of_snprintf;

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }
#ifdef QUOTES_TRANSFER_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer , remain_size, "{\\\"code\\\":%d"T_", \\\"clientToken\\\":\\\"%s\\\""T_",", replyPara->code, get_control_clientToken());
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer , remain_size, "{\"code\":%d, \"clientToken\":\"%s\",", replyPara->code, get_control_clientToken());
#endif
	rc = check_snprintf_return(rc_of_snprintf, remain_size);
	
    if (rc != QCLOUD_RET_SUCCESS) {
        return rc;
    }

	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return QCLOUD_ERR_JSON_BUFFER_TOO_SMALL;
    }

	if(strlen(replyPara->status_msg) > 0){
#ifdef QUOTES_TRANSFER_NEED		
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"status\\\":\\\"%s\\\"}", replyPara->status_msg);
#else
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"status\":\"%s\"}", replyPara->status_msg);
#endif
	}else{		
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "}");		
	}

	rc = check_snprintf_return(rc_of_snprintf, remain_size);
	
	return rc;
}

int IOT_Template_ControlReply(void *handle, char *pJsonDoc, size_t sizeOfBuffer, sReplyPara *replyPara) {

    IOT_FUNC_ENTRY;
	int rc;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* template_client = (Qcloud_IoT_Template*)handle;

	if (IOT_MQTT_IsConnected() == false) {
		Log_e("template is disconnected");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
	}

	// 如果没有之前没有订阅$thing/down/property成功，再一次订阅
	if (template_client->inner_data.sync_status < 0) {
		rc = subscribe_template_downstream_topic(template_client);
		if (rc < 0)
		{
			Log_e("Subcribe $thing/down/property fail!");
		}
	}
	
	rc = _template_ConstructControlReply(pJsonDoc, sizeOfBuffer, replyPara);
	if (rc != QCLOUD_RET_SUCCESS) {
		Log_e("Construct ControlReply fail,rc=%d",rc);
		return rc;
	}
	Log_d("Report Document: %s", pJsonDoc);

	RequestParams request_params = DEFAULT_REQUEST_PARAMS;	
	init_request_params(&request_params, REPLY, NULL, NULL, replyPara->timeout_ms/1000);

	rc = send_template_request(template_client, &request_params, pJsonDoc, sizeOfBuffer);
	IOT_FUNC_EXIT_RC(rc);
}

void IOT_Template_Yield(void *handle, uint32_t timeout_ms)
{		
	POINTER_SANITY_CHECK_RTN(handle);
	NUMBERIC_SANITY_CHECK_RTN(timeout_ms);
	
	_handle_message();
	handle_template_expired_reply((Qcloud_IoT_Template *)handle);
	
#ifdef EVENT_POST_ENABLED	
	handle_template_expired_event((Qcloud_IoT_Template *)handle);
#endif

	HAL_SleepMs(1);
}


int IOT_Template_Construct(void **client)
{
	POINTER_SANITY_CHECK(client, NULL);

	int rc;
	Qcloud_IoT_Template *template_client = get_template_client();

	template_client->status = eCLIENT_UNINITIALIZED;
	template_client->inner_data.eventflags = 0;
	template_client->inner_data.upstream_topic = NULL;
	template_client->inner_data.downstream_topic = NULL;
	template_client->inner_data.token_num = 0;
	
	rc = qcloud_iot_template_init(template_client);	
	if (rc != QCLOUD_RET_SUCCESS) 
	{
		Log_e("template init err,rc:%d", rc);
	}
	
	rc = subscribe_template_downstream_topic(template_client);
	if(rc != QCLOUD_RET_SUCCESS)
	{
		Log_e("Subcribe $thing/down/property fail!");
	}
	else 
	{
		template_client->inner_data.sync_status = rc;		
	}


#ifdef EVENT_POST_ENABLED
	rc = IOT_Event_Init(template_client);
	if (rc != QCLOUD_RET_SUCCESS) 
	{
		Log_e("event init failed: %d", rc);		
	}
#endif

#ifdef ACTION_ENABLED
	rc = IOT_Action_Init(template_client);
	if (rc != QCLOUD_RET_SUCCESS) 
	{
		Log_e("action init failed: %d", rc);
	}
#endif


	template_client->status = eCLIENT_INITIALIZED;
	*client = template_client;

	return rc;
}

int IOT_Template_Destroy(void *handle)
{
	IOT_FUNC_ENTRY;

	POINTER_SANITY_CHECK(handle, QCLOUD_ERR_INVAL);

	Qcloud_IoT_Template* template_client = (Qcloud_IoT_Template*)handle;
	qcloud_iot_template_reset(handle);

	module_mqtt_discon();

	if (NULL != template_client->mutex) 
	{
		HAL_MutexDestroy(template_client->mutex);
	}

	if (NULL != template_client->inner_data.downstream_topic) 
	{
		HAL_Free(template_client->inner_data.downstream_topic);
		template_client->inner_data.downstream_topic = NULL;
	}

	HAL_Free(handle);

	IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS)
}

#ifdef __cplusplus
}
#endif

