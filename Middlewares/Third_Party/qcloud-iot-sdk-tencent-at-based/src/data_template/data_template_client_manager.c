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

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "utils_timer.h"
#include "at_utils.h"
#include "data_template_client_json.h"
#include "data_template_client.h"
#include "qcloud_iot_api_export.h"


typedef void (*TraverseTemplateHandle)(Qcloud_IoT_Template *pTemplate, ListNode **node, List *list, const char *pClientToken, const char *pType);

static char sg_template_cloud_rcv_buf[CLOUD_IOT_JSON_RX_BUF_LEN];
static char sg_template_clientToken[MAX_SIZE_OF_CLIENT_TOKEN];



/**
 * @brief 取消订阅topic: $thing/down/property/{ProductId}/{DeviceName}
 */
static int _unsubscribe_template_downstream_topic(void* pClient)
{
    IOT_FUNC_ENTRY;
    int rc = QCLOUD_RET_SUCCESS;

    char downstream_topic[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
    int size = HAL_Snprintf(downstream_topic, MAX_SIZE_OF_CLOUD_TOPIC, "$thing/down/property/%s/%s", iot_device_info_get()->product_id, iot_device_info_get()->device_name);

    if (size < 0 || size > MAX_SIZE_OF_CLOUD_TOPIC - 1)
    {
        Log_e("buf size < topic length!");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

	rc = module_mqtt_unsub(downstream_topic);
    if (rc != QCLOUD_RET_SUCCESS) {
        Log_e("unsubscribe topic: %s failed: %d.", downstream_topic, rc);
    }

    IOT_FUNC_EXIT_RC(rc);
}

void qcloud_iot_template_reset(void *pClient) {
    POINTER_SANITY_CHECK_RTN(pClient);

    Qcloud_IoT_Template *template_client = (Qcloud_IoT_Template *)pClient;
    if (template_client->inner_data.property_handle_list) {
        list_destroy(template_client->inner_data.property_handle_list);
    }

    _unsubscribe_template_downstream_topic(template_client);

    if (template_client->inner_data.reply_list)
    {
        list_destroy(template_client->inner_data.reply_list);
    }

	if (template_client->inner_data.event_list)
    {
        list_destroy(template_client->inner_data.event_list);
    }
}

int qcloud_iot_template_init(Qcloud_IoT_Template *pTemplate) {
	IOT_FUNC_ENTRY;

    POINTER_SANITY_CHECK(pTemplate, QCLOUD_ERR_INVAL);

    pTemplate->mutex = HAL_MutexCreate();
    if (pTemplate->mutex == NULL)
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
		
		
		pTemplate->inner_data.pMsgList = list_new();
		if(pTemplate->inner_data.pMsgList)
		{
			pTemplate->inner_data.pMsgList->free = HAL_Free;
		} else {
		  Log_e("no memory to allocate pMsgList");
    	IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
		}

    pTemplate->inner_data.property_handle_list = list_new();
    if (pTemplate->inner_data.property_handle_list)
    {
        pTemplate->inner_data.property_handle_list->free = HAL_Free;
    }
    else {
    	Log_e("no memory to allocate property_handle_list");
    	IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

	pTemplate->inner_data.reply_list = list_new();
	if (pTemplate->inner_data.reply_list)
	{
		pTemplate->inner_data.reply_list->free = HAL_Free;
	} else {
		Log_e("no memory to allocate reply_list");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
	}
		
	pTemplate->inner_data.event_list = list_new();
	if (pTemplate->inner_data.event_list)
	{
		pTemplate->inner_data.event_list->free = HAL_Free;
	} else {
		Log_e("no memory to allocate event_list");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
	}

	pTemplate->inner_data.action_handle_list = list_new();
	if (pTemplate->inner_data.action_handle_list)
	{
		pTemplate->inner_data.action_handle_list->free = HAL_Free;
	} else {
		Log_e("no memory to allocate action_handle_list");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
	}

	IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS);
}

static void _set_control_clientToken(const char *pClientToken){
	memset(sg_template_clientToken, '\0', MAX_SIZE_OF_CLIENT_TOKEN);
	strncpy(sg_template_clientToken, pClientToken, MAX_SIZE_OF_CLIENT_TOKEN);
}

char * get_control_clientToken(void){
	return sg_template_clientToken;
}


/**
 * @brief 发布文档请求到物联云
 *
 * @param pClient                   Qcloud_IoT_Client对象
 * @param method                    文档操作方式
 * @param pJsonDoc                  等待发送的文档
 * @return 返回QCLOUD_ERR_SUCCESS, 表示发布文档请求成功
 */
static int _publish_to_template_upstream_topic(Qcloud_IoT_Template *pTemplate, Method method, char *pJsonDoc)
{
    IOT_FUNC_ENTRY;
    int rc = QCLOUD_RET_SUCCESS;

    char topic[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
	int size;
		

	size = HAL_Snprintf(topic, MAX_SIZE_OF_CLOUD_TOPIC, "$thing/up/property/%s/%s", iot_device_info_get()->product_id, iot_device_info_get()->device_name);	

	if (size < 0 || size > MAX_SIZE_OF_CLOUD_TOPIC - 1)
    {
        Log_e("buf size < topic length!");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

	rc = module_mqtt_pub(topic, QOS0, pJsonDoc);
	if(QCLOUD_RET_SUCCESS != rc)
	{
		Log_e("module mqtt pub fail,Ret:%d", rc);
	}


    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief 根据RequestParams、Method来给json填入type字段的值
 */
static int _set_template_json_type(char *pJsonDoc, size_t sizeOfBuffer, Method method)
{
    IOT_FUNC_ENTRY;

    int rc = QCLOUD_RET_SUCCESS;

    POINTER_SANITY_CHECK(pJsonDoc, QCLOUD_ERR_INVAL);
    char *method_str = NULL;
    switch (method) {
      case GET:
        method_str = GET_STATUS;
        break;
      case REPORT:
        method_str = REPORT_CMD;
        break;
      case RINFO:
        method_str = INFO_CMD;
		break;
      case REPLY:
        method_str = CONTROL_CMD_REPLY;
        break;
	  case CLEAR:
        method_str = CLEAR_CONTROL;
        break;	  
      default:
        Log_e("unexpected method!");
        rc = QCLOUD_ERR_INVAL;
        break;
    }
    if (rc != QCLOUD_RET_SUCCESS)
        IOT_FUNC_EXIT_RC(rc);

    size_t json_len = strlen(pJsonDoc);
    size_t remain_size = sizeOfBuffer - json_len;

    char json_node_str[64] = {0};
#ifdef QUOTES_TRANSFER_NEED	
    HAL_Snprintf(json_node_str, 64, "\\\"method\\\":\\\"%s\\\""T_", ", method_str);
#else
	 HAL_Snprintf(json_node_str, 64, "\"method\":\"%s\", ", method_str);
#endif

    size_t json_node_len = strlen(json_node_str);
    if (json_node_len >= remain_size - 1) {
        rc = QCLOUD_ERR_INVAL;
    } else {
        insert_str(pJsonDoc, json_node_str, 1);
    }

    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief 遍历列表, 对列表每个节点都执行一次传入的函数traverseHandle
 */
static void _traverse_template_list(Qcloud_IoT_Template *pTemplate, List *list, const char *pClientToken, const char *pType, TraverseTemplateHandle traverseHandle)
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

            traverseHandle(pTemplate, &node, list, pClientToken, pType);
        }

        list_iterator_destroy(iter);
    }
    HAL_MutexUnlock(pTemplate->mutex);

    IOT_FUNC_EXIT;
}

/**
 * @brief 执行过期的设备影子操作的回调函数
 */
static void _handle_template_expired_reply_callback(Qcloud_IoT_Template *pTemplate, ListNode **node, List *list, const char *pClientToken, const char *pType)
{
    IOT_FUNC_ENTRY;

    Request *request = (Request *)(*node)->val;
    if (NULL == request)
        IOT_FUNC_EXIT;

    if (expired(&request->timer))
    {
        if (request->callback != NULL) {
            request->callback(pTemplate, request->method, ACK_TIMEOUT, sg_template_cloud_rcv_buf, request->user_context);
        }

        list_remove(list, *node);
        *node = NULL;
    }

    IOT_FUNC_EXIT;
}


void handle_template_expired_reply(Qcloud_IoT_Template *pTemplate) {
    IOT_FUNC_ENTRY;

    _traverse_template_list(pTemplate, pTemplate->inner_data.reply_list, NULL, NULL, _handle_template_expired_reply_callback);

    IOT_FUNC_EXIT;
}

/**
 * @brief 将设备影子文档的操作请求保存在列表中
 */
static int _add_request_to_template_list(Qcloud_IoT_Template *pTemplate, const char *pClientToken, RequestParams *pParams)
{
    IOT_FUNC_ENTRY;

    HAL_MutexLock(pTemplate->mutex);
    if (pTemplate->inner_data.reply_list->len >= MAX_APPENDING_REQUEST_AT_ANY_GIVEN_TIME)
    {
        HAL_MutexUnlock(pTemplate->mutex);
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MAX_APPENDING_REQUEST);
    }

    Request *request = (Request *)HAL_Malloc(sizeof(Request));
    if (NULL == request) {
        HAL_MutexUnlock(pTemplate->mutex);
        Log_e("run memory malloc is error!");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

    request->callback = pParams->request_callback;
    strncpy(request->client_token, pClientToken, MAX_SIZE_OF_CLIENT_TOKEN);

    request->user_context = pParams->user_context;
    request->method = pParams->method;

    InitTimer(&(request->timer));
    countdown(&(request->timer), pParams->timeout_sec);

    ListNode *node = list_node_new(request);
    if (NULL == node) {
        HAL_MutexUnlock(pTemplate->mutex);
        Log_e("run list_node_new is error!");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

    list_rpush(pTemplate->inner_data.reply_list, node);

    HAL_MutexUnlock(pTemplate->mutex);

    IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS);
}


int send_template_request(Qcloud_IoT_Template *pTemplate, RequestParams *pParams, char *pJsonDoc, size_t sizeOfBuffer)
{
    IOT_FUNC_ENTRY;
    POINTER_SANITY_CHECK(pTemplate, QCLOUD_ERR_INVAL);
    POINTER_SANITY_CHECK(pJsonDoc, QCLOUD_ERR_INVAL);
    POINTER_SANITY_CHECK(pParams, QCLOUD_ERR_INVAL);
    int rc = QCLOUD_RET_SUCCESS;
	char* client_token = NULL;

	char *tempBuff = HAL_Malloc(strlen(pJsonDoc) + 1);
	if(NULL == tempBuff){
		Log_e("malloc mem fail");
		IOT_FUNC_EXIT_RC(QCLOUD_ERR_MALLOC);
	}
	strncpy(tempBuff, pJsonDoc, strlen(pJsonDoc));
	tempBuff[strlen(pJsonDoc)] = '\0';
	chr_strip(tempBuff, '\\');

    // 解析文档中的clientToken, 如果解析失败, 直接返回错误
    if (!parse_client_token(tempBuff, &client_token)) {
        Log_e("fail to parse client token!");
		HAL_Free(tempBuff);
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_INVAL);
    }
	HAL_Free(tempBuff);

    if (rc != QCLOUD_RET_SUCCESS)
        IOT_FUNC_EXIT_RC(rc);

    rc = _set_template_json_type(pJsonDoc, sizeOfBuffer, pParams->method);
    if (rc != QCLOUD_RET_SUCCESS)
        IOT_FUNC_EXIT_RC(rc);

	Log_d("Report Doc:%s", pJsonDoc);
    // 相应的 report topic 订阅成功或已经订阅
    if (rc == QCLOUD_RET_SUCCESS) {
        rc = _publish_to_template_upstream_topic(pTemplate, pParams->method, pJsonDoc);
    }

    if (rc == QCLOUD_RET_SUCCESS) {
#ifdef QUOTES_TRANSFER_NEED			
		at_strip(client_token, '\\');
#endif
        rc = _add_request_to_template_list(pTemplate, client_token, pParams);
    }

    HAL_Free(client_token);

    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief 处理注册属性的回调函数
 * 当订阅的$thing/down/property/{ProductId}/{DeviceName}返回消息时，
 * 若对应的method为control或者get的数据有control, 则执行该函数
 * 
 */
static void _handle_control(Qcloud_IoT_Template *pTemplate, char* control_str)
{
    IOT_FUNC_ENTRY;
    if (pTemplate->inner_data.property_handle_list->len) {
        ListIterator *iter;
        ListNode *node = NULL;
        PropertyHandler *property_handle = NULL;

        if (NULL == (iter = list_iterator_new(pTemplate->inner_data.property_handle_list, LIST_TAIL))) {
            HAL_MutexUnlock(pTemplate->mutex);
            IOT_FUNC_EXIT;
        }

        for (;;) {
            node = list_iterator_next(iter);
            if (NULL == node) {
                break;
            }

            property_handle = (PropertyHandler *)(node->val);
            if (NULL == property_handle) {
                Log_e("node's value is invalid!");
                continue;
            }

            if (property_handle->property != NULL) {
                if (update_value_if_key_match(control_str, property_handle->property))
                {
                	
                    if (property_handle->callback != NULL)
                    {
                        property_handle->callback(pTemplate, control_str, strlen(control_str), property_handle->property);
                    }
                    node = NULL;
                }
            }

        }

        list_iterator_destroy(iter);
    }

    IOT_FUNC_EXIT;
}


/**
 * @brief 执行数据模板操作的回调函数
 */
static void _handle_template_reply_callback(Qcloud_IoT_Template *pTemplate, ListNode **node, List *list, const char *pClientToken, const char *pType)
{
    IOT_FUNC_ENTRY;

	
    Request *request = (Request *)(*node)->val;
    if (NULL == request)
        IOT_FUNC_EXIT;


    if (strcmp(request->client_token, pClientToken) == 0)
    {
        RequestAck status = ACK_NONE;

        // 通过回复包的 code 来确定对应的操作是否成功，0：成功， 1：失败
        int32_t reply_code = 0;
        
        bool parse_success = parse_code_return(sg_template_cloud_rcv_buf, &reply_code);
        if (parse_success) {
        	if (reply_code == 0) {
				status = ACK_ACCEPTED;
			} else {
				status = ACK_REJECTED;
			}

			if (strcmp(pType, GET_STATUS_REPLY) == 0 && status == ACK_ACCEPTED){				
				char* control_str = NULL;
				if (parse_template_get_control(sg_template_cloud_rcv_buf, &control_str)) {
					Log_d("control data from get_status_reply");
					_set_control_clientToken(pClientToken);
					_handle_control(pTemplate, control_str);
					HAL_Free(control_str);
					*((ReplyAck *)request->user_context) = ACK_ACCEPTED; //prepare for clear_control
				}
			}
						
			if (request->callback != NULL) {
				request->callback(pTemplate, request->method, status, sg_template_cloud_rcv_buf, request);
			}
        }
        else {
        	Log_e("parse template operation result code failed.");
        }
        
        list_remove(list, *node);
        *node = NULL;

    }

    IOT_FUNC_EXIT;
}



/**
 * @brief 文档操作请求结果的回调函数
 * 客户端先订阅 $thing/down/property/{ProductId}/{DeviceName}, 收到该topic的消息则会调用该回调函数
 * 在这个回调函数中, 解析出各个设备影子文档操作的结果
 */
static void _on_template_downstream_topic_handler(char *msg, void *context)
{
	POINTER_SANITY_CHECK_RTN(msg);
	char *client_token = NULL;
	char *method_str = NULL;
	//Qcloud_IoT_Template *template_client = get_template_client();
	Qcloud_IoT_Template *template_client = (Qcloud_IoT_Template *)context;


    int cloud_rcv_len = min(CLOUD_IOT_JSON_RX_BUF_LEN - 1, strlen(msg));
    memcpy(sg_template_cloud_rcv_buf, msg, cloud_rcv_len + 1);
    sg_template_cloud_rcv_buf[cloud_rcv_len] = '\0';    // jsmn_parse relies on a string
	//Log_d("recv:%s", sg_template_cloud_rcv_buf);

	//解析数据模板 $thing/down/property 下行消息类型
    if (!parse_template_method_type(sg_template_cloud_rcv_buf, &method_str))
    {
        Log_e("Fail to parse method!");
        goto End;
    }

	//找到对应的client_token
    if (!parse_client_token(sg_template_cloud_rcv_buf, &client_token)) {
		Log_e("Fail to parse client token! Json=%s", sg_template_cloud_rcv_buf);
		goto End;
    }
	
	//处理control 消息
    if (!strcmp(method_str, CONTROL_CMD)) {
	    HAL_MutexLock(template_client->mutex);
	    char* control_str = NULL;
	    if (parse_template_cmd_control(sg_template_cloud_rcv_buf, &control_str)) {
			Log_d("control_str:%s", control_str);
			_set_control_clientToken(client_token);
	    	_handle_control(template_client, control_str);
	    	HAL_Free(control_str);
	    }

	    HAL_MutexUnlock(template_client->mutex);
	    goto End;
	}
   
	
    if (template_client != NULL)
        _traverse_template_list(template_client, template_client->inner_data.reply_list, client_token, method_str, _handle_template_reply_callback);
End:
    HAL_Free(method_str);
    HAL_Free(client_token);

    IOT_FUNC_EXIT;
}


int subscribe_template_downstream_topic(Qcloud_IoT_Template *pTemplate)
{
	eAtResault rc;
	int size;

	if (pTemplate->inner_data.downstream_topic == NULL) 
	{
		char *downstream_topic = (char *)HAL_Malloc(MAX_SIZE_OF_CLOUD_TOPIC * sizeof(char));

		if (downstream_topic == NULL) 
		{
			IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
		}
		memset(downstream_topic, 0x0, MAX_SIZE_OF_CLOUD_TOPIC);

		size = HAL_Snprintf(downstream_topic, MAX_SIZE_OF_CLOUD_TOPIC, "$thing/down/property/%s/%s", iot_device_info_get()->product_id, iot_device_info_get()->device_name);				
		if (size < 0 || size > MAX_SIZE_OF_CLOUD_TOPIC - 1)
		{
			Log_e("buf size < topic length!");
			HAL_Free(downstream_topic);
			IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
		}
			pTemplate->inner_data.downstream_topic = downstream_topic;
	}


	rc = module_mqtt_sub(pTemplate->inner_data.downstream_topic, QOS0, _on_template_downstream_topic_handler, (void *)pTemplate);
	if (rc != QCLOUD_RET_SUCCESS) 
	{
		Log_e("subscribe topic: %s failed: %d.", pTemplate->inner_data.downstream_topic, rc);
	}

	IOT_FUNC_EXIT_RC(rc);
}


#ifdef __cplusplus
}
#endif
