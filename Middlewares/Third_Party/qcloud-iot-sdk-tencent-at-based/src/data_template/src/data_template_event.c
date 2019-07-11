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
#include "qcloud_iot_api_export.h"

#if defined(EVENT_POST_ENABLED)

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "data_template_client.h"
#include "data_template_event.h"
#include "lite-utils.h"
#include "utils_timer.h"
#include "utils_method.h"


void setEventFlag(void *client, uint32_t flag)
{
	Qcloud_IoT_Template *pTemplate = (Qcloud_IoT_Template *)client;
	
	pTemplate->inner_data.eventflags |= flag&0xffffffff;
}

void clearEventFlag(void *client, uint32_t flag)
{
	Qcloud_IoT_Template *pTemplate = (Qcloud_IoT_Template *)client;
	
	pTemplate->inner_data.eventflags &= (~flag)&0xffffffff;
}

uint32_t getEventFlag(void *client)
{
	Qcloud_IoT_Template *pTemplate = (Qcloud_IoT_Template *)client;
	
	return pTemplate->inner_data.eventflags;
}


/**
 * @brief 遍历事件列表
 */
static void _traverse_event_list(Qcloud_IoT_Template *pTemplate, List *list, const char *pClientToken, char *message, eEventDealType eDealType)
{
    IOT_FUNC_ENTRY;

    HAL_MutexLock(pTemplate->mutex);

    if (list->len) {
        ListIterator *iter;
        ListNode *node = NULL;

        if (NULL == (iter = list_iterator_new(list, LIST_TAIL))) {
            HAL_MutexUnlock(pTemplate->mutex);
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
			
			sReply *pReply =  (sReply *)node->val;						
			/*列表事件超时检查*/
			if(eDEAL_EXPIRED == eDealType){
				if(expired(&pReply->timer)){
					Log_e("eventToken[%s] timeout",pReply->client_token);
					list_remove(list, node);
					node = NULL;
				}					
			}

			/*列表事件回复匹配*/
			if((eDEAL_REPLY_CB == eDealType) && (0 == strcmp(pClientToken, pReply->client_token))){						
				if(NULL != pReply->callback){
					pReply->callback(message, pTemplate);
					Log_d("eventToken[%s] released",pReply->client_token);
					list_remove(list, node);
    				node = NULL;
				}						
			}
        }

        list_iterator_destroy(iter);
    }
    HAL_MutexUnlock(pTemplate->mutex);

    IOT_FUNC_EXIT;
}

void handle_template_expired_event(void *client) {
    IOT_FUNC_ENTRY;
	Qcloud_IoT_Template *pTemplate = (Qcloud_IoT_Template *)client;

    _traverse_event_list(pTemplate, pTemplate->inner_data.event_list, NULL, NULL, eDEAL_EXPIRED);

    IOT_FUNC_EXIT;
}

static void _on_event_reply_callback(char *msg, void *context) 
{
	POINTER_SANITY_CHECK_RTN(msg);
	Qcloud_IoT_Template *template_client = (Qcloud_IoT_Template *)context;
	//Qcloud_IoT_Template *template_client = get_template_client();

	int32_t code;
    char* client_token = NULL;
	char* status = NULL;

	Log_d("Receive event reply message: %s", msg);

	// 解析事件回复中的clientToken
    if (!parse_client_token(msg, &client_token)) {
        Log_e("fail to parse client token!");
        return;
    }

	// 解析事件回复中的处理结果
	if(!parse_code_return(msg, &code)){
        Log_e("fail to parse code");
        return;
    }

	if(!parse_status_return(msg, &status)){
       // Log_d("no status return");
    }
	
	//Log_d("eventToken:%s code:%d status:%s", client_token, code, status);
    if (template_client != NULL)
        _traverse_event_list(template_client, template_client->inner_data.event_list, client_token, msg, eDEAL_REPLY_CB);


    HAL_Free(client_token);
	HAL_Free(status);

	return;	
}


/**
 * @brief 创建事件回复结构体并添加到回复等待列表
 */
static sReply * _create_event_add_to_list(Qcloud_IoT_Template *pTemplate, OnEventReplyCallback replyCb, uint32_t reply_timeout_ms)
{
    IOT_FUNC_ENTRY;

    HAL_MutexLock(pTemplate->mutex);
    if (pTemplate->inner_data.event_list->len >= MAX_EVENT_WAIT_REPLY)
    {
        HAL_MutexUnlock(pTemplate->mutex);
		Log_e("Too many event wait for reply");
        IOT_FUNC_EXIT_RC(NULL);
    }

    sReply *pReply = (sReply *)HAL_Malloc(sizeof(Request));
    if (NULL == pReply) {
        HAL_MutexUnlock(pTemplate->mutex);
        Log_e("run memory malloc is error!");
        IOT_FUNC_EXIT_RC(NULL);
    }

    pReply->callback = replyCb;
	pReply->user_context = pTemplate;
  
    InitTimer(&(pReply->timer));
    countdown(&(pReply->timer), reply_timeout_ms);
	HAL_Snprintf(pReply->client_token, EVENT_TOKEN_MAX_LEN, "%s-%u", iot_device_info_get()->product_id, pTemplate->inner_data.token_num++);
 

    ListNode *node = list_node_new(pReply);
    if (NULL == node) {
        HAL_MutexUnlock(pTemplate->mutex);
        Log_e("run list_node_new is error!");
		HAL_Free(pReply);
        IOT_FUNC_EXIT_RC(NULL);
    }

    list_rpush(pTemplate->inner_data.event_list, node);

    HAL_MutexUnlock(pTemplate->mutex);

    IOT_FUNC_EXIT_RC(pReply);
} 

static int _IOT_Event_JSON_Init(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t event_count, OnEventReplyCallback replyCb, uint32_t reply_timeout_ms)
{
	POINTER_SANITY_CHECK(jsonBuffer, AT_ERR_INVAL);
		
	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template *)handle;
	int32_t rc_of_snprintf;
	sReply *pReply;
			
  	pReply = _create_event_add_to_list(ptemplate, replyCb, reply_timeout_ms);
	if(!pReply){
		Log_e("create event failed");
		return AT_ERR_FAILURE;
	}

	memset(jsonBuffer, 0, sizeOfBuffer);	
	if(event_count > SIGLE_EVENT){
#ifdef TRANSFER_LABEL_NEED			
		rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\\\"method\\\":\\\"%s\\\""T_", \\\"clientToken\\\":\\\"%s\\\""T_", ", \
																				POST_EVENTS, pReply->client_token);
#else
		rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\"method\":\"%s\", \"clientToken\":\"%s\", ", \
																		POST_EVENTS, pReply->client_token);
#endif
	}else{
#ifdef TRANSFER_LABEL_NEED	
		rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\\\"method\\\":\\\"%s\\\""T_", \\\"clientToken\\\":\\\"%s\\\""T_", ", \
																				POST_EVENT, pReply->client_token);
#else
		rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\"method\":\"%s\", \"clientToken\":\"%s\",  ", \
																				POST_EVENT, pReply->client_token);
#endif
	}

    return check_snprintf_return(rc_of_snprintf, sizeOfBuffer);
 }
												
static int _IOT_Construct_Event_JSON(void *handle, char *jsonBuffer, size_t sizeOfBuffer,
														uint8_t event_count, 
														sEvent *pEventArry[],
														OnEventReplyCallback replyCb, 
														uint32_t reply_timeout_ms) 
{	 
	 size_t remain_size = 0;
	 int32_t rc_of_snprintf = 0;
	 uint8_t i,j;
	 Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template *)handle;
	 
	 POINTER_SANITY_CHECK(ptemplate, AT_ERR_INVAL);
	 POINTER_SANITY_CHECK(jsonBuffer, AT_ERR_INVAL);
	 POINTER_SANITY_CHECK(pEventArry, AT_ERR_INVAL);

	 int rc = _IOT_Event_JSON_Init(ptemplate, jsonBuffer, sizeOfBuffer, event_count, replyCb, reply_timeout_ms);
 
	 if (rc != AT_ERR_SUCCESS) {
		 Log_e("event json init failed: %d", rc);
		 return rc;
	 }
	 //Log_d("event_count:%d, Doc_init:%s",event_count, jsonBuffer);
 
	 if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
		 return AT_ERR_JSON_BUFFER_TOO_SMALL;
	 }

	if(event_count > SIGLE_EVENT){//多个事件
#ifdef TRANSFER_LABEL_NEED	
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"events\\\":[");
#else
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"events\":[");
#endif
		rc = check_snprintf_return(rc_of_snprintf, remain_size);			
		if (rc != AT_ERR_SUCCESS) {
				return rc;
		}

		if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
			return AT_ERR_JSON_BUFFER_TOO_SMALL;
		}

		for(i = 0; i < event_count; i++){
			sEvent *pEvent = pEventArry[i];
			if(NULL == pEvent){
				Log_e("%dth/%d null event", i, event_count);
				return AT_ERR_NULL;
			}
			
#ifdef	EVENT_TIMESTAMP_USED
		#ifdef TRANSFER_LABEL_NEED
			rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "{\\\"eventId\\\":\\\"%s\\\""T_", \\\"type\\\":\\\"%s\\\""T_", \\\"timestamp\\\":%d"T_", \\\"params\\\":{",\
											pEvent->event_name, pEvent->type, pEvent->timestamp);
		#else
			rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "{\"eventId\":\"%s\", \"type\":\"%s\", \"timestamp\":%d, \"params\":{",\
											pEvent->event_name, pEvent->type, pEvent->timestamp);
		#endif
#else
		#ifdef TRANSFER_LABEL_NEED
			rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "{\\\"eventId\\\":\\\"%s\\\""T_", \\\"type\\\":\\\"%s\\\""T_", \\\"params\\\":{",\
											pEvent->event_name, pEvent->type);
		#else
			rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "{\"eventId\":\"%s\", \"type\":\"%s\", \"params\":{",\
											pEvent->event_name, pEvent->type);
		#endif
#endif

			rc = check_snprintf_return(rc_of_snprintf, remain_size);			
			if (rc != AT_ERR_SUCCESS) {
					return rc;
			}

			if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
				return AT_ERR_JSON_BUFFER_TOO_SMALL;
			}

			DeviceProperty *pJsonNode = pEvent->pEventData;
			for (j = 0; j < pEvent->eventDataNum; j++) {
				if (pJsonNode != NULL && pJsonNode->key != NULL) {
					rc = event_put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);

					if (rc != AT_ERR_SUCCESS) {
						return rc;
					}
				} else {
					Log_e("%dth/%d null event property data", i, pEvent->eventDataNum);
					return AT_ERR_INVAL;
				}
				pJsonNode++;
			}	
			if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
				return AT_ERR_JSON_BUFFER_TOO_SMALL;
			}
			
			rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer)-1, remain_size, "}}"T_"," );

			rc = check_snprintf_return(rc_of_snprintf, remain_size);
			if (rc != AT_ERR_SUCCESS) {
					return rc;
			}

			pEvent++;	
		}

		if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
			return AT_ERR_JSON_BUFFER_TOO_SMALL;
		}
		
#ifdef TRANSFER_LABEL_NEED	
		//转义符存在时为 \,
		jsonBuffer[strlen(jsonBuffer)-1] = '\0';
#endif		
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "]");
		rc = check_snprintf_return(rc_of_snprintf, remain_size);
		if (rc != AT_ERR_SUCCESS) {
				return rc;
		}

	}else{ //单个事件		
		sEvent *pEvent = pEventArry[0];
#ifdef	EVENT_TIMESTAMP_USED				
	#ifdef TRANSFER_LABEL_NEED	
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"eventId\\\":\\\"%s\\\""T_", \\\"type\\\":\\\"%s\\\""T_", \\\"timestamp\\\":%d"T_", \\\"params\\\":{",\
										pEvent->event_name, pEvent->type, pEvent->timestamp);
	#else
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"eventId\":\"%s\", \"type\":\"%s\", \"timestamp\":%d, \"params\":{",\
										pEvent->event_name, pEvent->type, pEvent->timestamp);
	#endif
#else
	#ifdef TRANSFER_LABEL_NEED	
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"eventId\\\":\\\"%s\\\""T_", \\\"type\\\":\\\"%s\\\""T_", \\\"params\\\":{",\
										pEvent->event_name, pEvent->type);
	#else
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"eventId\":\"%s\", \"type\":\"%s\", \"params\":{",\
										pEvent->event_name, pEvent->type);
	#endif
#endif
		rc = check_snprintf_return(rc_of_snprintf, remain_size);
		if (rc != AT_ERR_SUCCESS) {
				return rc;
		}

		if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
			return AT_ERR_JSON_BUFFER_TOO_SMALL;
		}

		DeviceProperty *pJsonNode = pEvent->pEventData;
		for (i = 0; i < pEvent->eventDataNum; i++) {			
			if (pJsonNode != NULL && pJsonNode->key != NULL) {
				rc = event_put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);

				if (rc != AT_ERR_SUCCESS) {
					return rc;
				}
			} else {
				Log_e("%dth/%d null event property data", i, pEvent->eventDataNum);
				return AT_ERR_INVAL;
			}
			pJsonNode++;
		}	
		if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
			return AT_ERR_JSON_BUFFER_TOO_SMALL;
		}
		
		rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "}" );

		rc = check_snprintf_return(rc_of_snprintf, remain_size);
		if (rc != AT_ERR_SUCCESS) {
				return rc;
		}
	}

	//finish json
	 if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) < 1) {
		 return AT_ERR_JSON_BUFFER_TOO_SMALL;
	 }
	 rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "}");
	 
	 return check_snprintf_return(rc_of_snprintf, remain_size);
}

												

static int _publish_event_to_cloud(void *c, char *pJsonDoc)
{
	IOT_FUNC_ENTRY;
	int rc = AT_ERR_SUCCESS;
	char topic[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
	int size = HAL_Snprintf(topic, MAX_SIZE_OF_CLOUD_TOPIC, "$thing/up/event/%s/%s", iot_device_info_get()->product_id, iot_device_info_get()->device_name);

	if (size < 0 || size > sizeof(topic) - 1)
	{
		Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic));
		return AT_ERR_FAILURE;
	}

	rc = module_mqtt_pub(topic, QOS0, pJsonDoc);
	if(AT_ERR_SUCCESS != rc)
	{
		Log_e("module mqtt pub fail,Ret:%d", rc);
	}

	IOT_FUNC_EXIT_RC(rc);
}


int event_init(void *c)
{
	static char topic_name[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
	eAtResault rc;


	int size = HAL_Snprintf(topic_name, MAX_SIZE_OF_CLOUD_TOPIC, "$thing/down/event/%s/%s", iot_device_info_get()->product_id, iot_device_info_get()->device_name);

	
	if (size < 0 || size > sizeof(topic_name) - 1)
	{
		Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
		return AT_ERR_FAILURE;
	}

	
	rc = module_mqtt_sub(topic_name, QOS0, _on_event_reply_callback, c);
	if (rc != AT_ERR_SUCCESS) 
	{
		Log_e("subscribe topic: %s failed: %d.", topic_name, rc);
	}

	return rc;
}



int qcloud_iot_post_event(void *pClient, char *pJsonDoc, size_t sizeOfBuffer, uint8_t event_count, sEvent *pEventArry[], OnEventReplyCallback replyCb) 									                                           
{
	int rc;
	
	rc = _IOT_Construct_Event_JSON(pClient, pJsonDoc, sizeOfBuffer, event_count, pEventArry, replyCb, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	if (rc != AT_ERR_SUCCESS) {
		Log_e("construct event json fail, %d",rc);
		return rc;
	}
	Log_d("post event:%s", pJsonDoc);		
	rc = _publish_event_to_cloud(pClient, pJsonDoc);	
	if (rc != AT_ERR_SUCCESS) {
		Log_e("publish event to cloud fail, %d",rc);
	}

	return rc;
}

int qcloud_iot_post_event_raw(void *pClient, char *pJsonDoc, size_t sizeOfBuffer, char *pEventMsg, OnEventReplyCallback replyCb)                                            
{
	int rc;
	size_t remain_size = 0;
	int32_t rc_of_snprintf;
	
	Qcloud_IoT_Template* ptemplate = (Qcloud_IoT_Template *)pClient;

	POINTER_SANITY_CHECK(ptemplate, AT_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, AT_ERR_INVAL);
	POINTER_SANITY_CHECK(pEventMsg, AT_ERR_INVAL);

	rc = _IOT_Event_JSON_Init(ptemplate, pJsonDoc, sizeOfBuffer, MUTLTI_EVENTS, replyCb, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT);
	if (rc != AT_ERR_SUCCESS) {
		Log_e("event json init failed: %d", rc);
		return rc;
	}

	if ((remain_size = sizeOfBuffer - strlen(pJsonDoc)) <= 1) {
		return AT_ERR_JSON_BUFFER_TOO_SMALL;
	}
#ifdef TRANSFER_LABEL_NEED	
	rc_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc), remain_size, "\\\"events\\\":[%s]}", pEventMsg);
#else
	rc_of_snprintf = HAL_Snprintf(pJsonDoc + strlen(pJsonDoc), remain_size, "\"events\":[%s]}", pEventMsg);
#endif
	rc = check_snprintf_return(rc_of_snprintf, remain_size);			
	if (rc != AT_ERR_SUCCESS) {
			return rc;
	}

	//Log_d("JsonDoc:%s", pJsonDoc);
	
	rc = _publish_event_to_cloud(pClient, pJsonDoc);	
	if (rc != AT_ERR_SUCCESS) {
		Log_e("publish event raw to cloud fail, %d",rc);
	}

	return rc;
}


#endif
#ifdef __cplusplus
}
#endif
