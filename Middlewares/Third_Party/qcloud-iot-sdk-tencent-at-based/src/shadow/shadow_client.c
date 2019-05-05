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
#include <stdio.h>
#include <string.h>
#include "shadow_client.h"
#include "shadow_client_json.h"
#include "shadow_client_common.h"
#include "qcloud_iot_api_export.h"


static Qcloud_IoT_Shadow sg_shadow_client = {0};


void *get_shadow_client(void)
{
	return (void *)&sg_shadow_client;
}


static eAtResault _qcloud_iot_shadow_init(Qcloud_IoT_Shadow *pShadow) 
{
	POINTER_SANITY_CHECK(pShadow, AT_ERR_INVAL);

	pShadow->inner_data.version = 0;
	pShadow->mutex = HAL_MutexCreate();
	if (pShadow->mutex == NULL)
	{
		IOT_FUNC_EXIT_RC(AT_ERR_FAILURE);
	}


	pShadow->inner_data.property_handle_list = list_new();
	if (pShadow->inner_data.property_handle_list)
	{
		pShadow->inner_data.property_handle_list->free = HAL_Free;
	}
	else 
	{
		Log_e("no memory to allocate property_handle_list");
		IOT_FUNC_EXIT_RC(AT_ERR_FAILURE);
	}

	pShadow->inner_data.request_list = list_new();
	if (pShadow->inner_data.request_list)
	{
		pShadow->inner_data.request_list->free = HAL_Free;
	} 
	else 
	{
		Log_e("no memory to allocate request_list");
		IOT_FUNC_EXIT_RC(AT_ERR_FAILURE);
	}


	IOT_FUNC_EXIT_RC(AT_ERR_SUCCESS);
}


 eAtResault IOT_Shadow_Construct(eShadowType eType)
{
	eAtResault rc;
	Qcloud_IoT_Shadow *shadow_client = get_shadow_client();

	shadow_client->status = eSHADOW_UNINITIALIZED;
	shadow_client->shadow_type = eType;
	shadow_client->inner_data.result_topic = NULL;
	shadow_client->inner_data.version = 0;
	shadow_client->inner_data.token_num = 0;
	
	

	rc = _qcloud_iot_shadow_init(shadow_client);	//初始化delta
	if (rc != AT_ERR_SUCCESS) 
	{
		Log_e("shadow init err,rc:%d", rc);
	}
	
	rc = subscribe_operation_result_to_cloud(shadow_client);
	if(rc != AT_ERR_SUCCESS)
	{
		Log_e("Subcribe $shadow/operation/results fail!");
	}
	else 
	{
		//sync logic to be sure
		shadow_client->inner_data.sync_status = rc;		
	}

	shadow_client->status = eSHADOW_INITIALIZED;


	return rc;
}

 int IOT_Shadow_Destroy(void *handle)
 {
	 IOT_FUNC_ENTRY;
 
	 POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
 
	 Qcloud_IoT_Shadow* shadow_client = (Qcloud_IoT_Shadow*)handle;
	 qcloud_iot_shadow_reset(handle);
 
	 module_mqtt_discon();
 
	 if (NULL != shadow_client->mutex) {
		 HAL_MutexDestroy(shadow_client->mutex);
	 }
 
	 if (NULL != shadow_client->inner_data.result_topic) {
		 HAL_Free(shadow_client->inner_data.result_topic);
		 shadow_client->inner_data.result_topic = NULL;
	 }
 
	 HAL_Free(handle);
 
	 IOT_FUNC_EXIT_RC(AT_ERR_SUCCESS)
 }


void IOT_Shadow_Yield(uint32_t timeout_ms)
{	
	Qcloud_IoT_Shadow *pshadow = get_shadow_client();
	
	handle_expired_request(pshadow);
	HAL_SleepMs(1);
}

static void _init_request_params(RequestParams *pParams, Method method, OnRequestCallback callback, void *userContext, uint8_t timeout_sec) {
	pParams->method 			=		method;
	pParams->user_context 		= 		userContext;
	pParams->timeout_sec 		= 		timeout_sec;
	pParams->request_callback 	= 		callback;
}



/**
 * @brief 检查函数snprintf的返回值
 *
 * @param returnCode       函数snprintf的返回值
 * @param maxSizeOfWrite   可写最大字节数
 * @return                 返回AT_ERR_JSON, 表示出错; 返回AT_ERR_JSON_BUFFER_TRUNCATED, 表示截断
 */
static inline int _check_snprintf_return(int32_t returnCode, size_t maxSizeOfWrite) {

    if (returnCode >= maxSizeOfWrite) {
        return AT_ERR_JSON_BUFFER_TRUNCATED;
    } else if (returnCode < 0) { // 写入出错
        return AT_ERR_JSON;
    }

    return AT_ERR_SUCCESS;
}

static void _update_ack_cb(void *pClient, Method method, RequestAck requestAck, const char *pReceivedJsonDocument, void *pUserdata) 
{
	Log_d("requestAck=%d", requestAck);

    if (NULL != pReceivedJsonDocument) {
        Log_d("Received Json Document=%s", pReceivedJsonDocument);
    } else {
        Log_d("Received Json Document is NULL");
    }

    *((RequestAck *)pUserdata) = requestAck;
}


int IOT_Shadow_Register_Property(void *handle, DeviceProperty *pProperty, OnPropRegCallback callback) {

    IOT_FUNC_ENTRY;
	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);

	Qcloud_IoT_Shadow* pshadow = (Qcloud_IoT_Shadow*)handle;
	int rc;

    if (IOT_MQTT_IsConnected() == false) {
        IOT_FUNC_EXIT_RC(AT_ERR_MQTT_NO_CONN);
    }

	if (shadow_common_check_property_existence(pshadow, pProperty)) 
		IOT_FUNC_EXIT_RC(AT_ERR_SHADOW_PROPERTY_EXIST);

    rc = shadow_common_register_property_on_delta(pshadow, pProperty, callback);

    IOT_FUNC_EXIT_RC(rc);
}

int IOT_Shadow_UnRegister_Property(void *handle, DeviceProperty *pProperty) {
	IOT_FUNC_ENTRY;
	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	Qcloud_IoT_Shadow* pshadow = (Qcloud_IoT_Shadow*)handle;

	if (IOT_MQTT_IsConnected() == false) {
        IOT_FUNC_EXIT_RC(AT_ERR_MQTT_NO_CONN);
    }

	if (!shadow_common_check_property_existence(pshadow, pProperty)) {
        IOT_FUNC_EXIT_RC(AT_ERR_SHADOW_NOT_PROPERTY_EXIST);
    }
	int rc =  shadow_common_remove_property(pshadow, pProperty);
	IOT_FUNC_EXIT_RC(rc);
}

int IOT_Shadow_Update(void *handle, char *pJsonDoc, size_t sizeOfBuffer, OnRequestCallback callback, void *userContext, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc;

	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, AT_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, AT_ERR_INVAL);

	Qcloud_IoT_Shadow* shadow = (Qcloud_IoT_Shadow*)handle;

	if (IOT_MQTT_IsConnected() == false) {
		Log_e("shadow is disconnected");
		IOT_FUNC_EXIT_RC(AT_ERR_MQTT_NO_CONN);
	}

	// 如果没有之前没有订阅$shadow/operation/result成功，再一次订阅
	if (shadow->inner_data.sync_status < 0) {
		subscribe_operation_result_to_cloud(shadow);
	}

	Log_d("UPDATE Request Document: %s", pJsonDoc);

	RequestParams request_params = DEFAULT_REQUEST_PARAMS;
	_init_request_params(&request_params, UPDATE, callback, userContext, timeout_ms/1000);

	rc = do_shadow_request(shadow, &request_params, pJsonDoc, sizeOfBuffer);
	IOT_FUNC_EXIT_RC(rc);
}

int IOT_Shadow_Update_Sync(void *handle, char *pJsonDoc, size_t sizeOfBuffer, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc = AT_ERR_SUCCESS;

	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	POINTER_SANITY_CHECK(pJsonDoc, AT_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, AT_ERR_INVAL);

	if (IOT_MQTT_IsConnected() == false) {
		Log_e("shadow is disconnected");
		IOT_FUNC_EXIT_RC(AT_ERR_MQTT_NO_CONN);
	}

	RequestAck ack_update = ACK_NONE;
	rc = IOT_Shadow_Update(handle, pJsonDoc, sizeOfBuffer, _update_ack_cb, &ack_update, timeout_ms);
	if (rc != AT_ERR_SUCCESS) IOT_FUNC_EXIT_RC(rc);

	while (ACK_NONE == ack_update) {
        IOT_Shadow_Yield(200);
    }

	if (ACK_ACCEPTED == ack_update) {
        rc = AT_ERR_SUCCESS;
    } else if (ACK_TIMEOUT == ack_update) {
        rc = AT_ERR_SHADOW_UPDATE_TIMEOUT;
    } else if (ACK_REJECTED == ack_update) {
        rc = AT_ERR_SHADOW_UPDATE_REJECTED;
    }
	
	IOT_FUNC_EXIT_RC(rc);
}

int IOT_Shadow_Get(void *handle, OnRequestCallback callback, void *userContext, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc;

	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	POINTER_SANITY_CHECK(callback, AT_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, AT_ERR_INVAL);

	Qcloud_IoT_Shadow* shadow = (Qcloud_IoT_Shadow*)handle;

	if (IOT_MQTT_IsConnected() == false) {
		IOT_FUNC_EXIT_RC(AT_ERR_MQTT_NO_CONN);
	}

	// 如果没有之前没有订阅$shadow/operation/result成功，再一次订阅
	if (shadow->inner_data.sync_status < 0) {
		subscribe_operation_result_to_cloud(shadow);
	}

	char getRequestJsonDoc[MAX_SIZE_OF_JSON_WITH_CLIENT_TOKEN];
	build_empty_json(&(shadow->inner_data.token_num), getRequestJsonDoc);
	Log_d("GET Request Document: %s", getRequestJsonDoc);

	RequestParams request_params = DEFAULT_REQUEST_PARAMS;
	_init_request_params(&request_params, GET, callback, userContext, timeout_ms/1000);

	rc = do_shadow_request(shadow, &request_params, getRequestJsonDoc, MAX_SIZE_OF_JSON_WITH_CLIENT_TOKEN);
	IOT_FUNC_EXIT_RC(rc);
}

int IOT_Shadow_Get_Sync(void *handle, uint32_t timeout_ms) {

    IOT_FUNC_ENTRY;
	int rc = AT_ERR_SUCCESS;

	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	NUMBERIC_SANITY_CHECK(timeout_ms, AT_ERR_INVAL);


	if (IOT_MQTT_IsConnected() == false) {
		IOT_FUNC_EXIT_RC(AT_ERR_MQTT_NO_CONN);
	}

	RequestAck ack_update = ACK_NONE;
	rc = IOT_Shadow_Get(handle, _update_ack_cb, &ack_update, timeout_ms);
	if (rc != AT_ERR_SUCCESS) IOT_FUNC_EXIT_RC(rc);
	
	while (ACK_NONE == ack_update) {		
        IOT_Shadow_Yield(2000);
    }

	if (ACK_ACCEPTED == ack_update) {
        rc = AT_ERR_SUCCESS;
    } else if (ACK_TIMEOUT == ack_update) {
        rc = AT_ERR_SHADOW_GET_TIMEOUT;
    } else if (ACK_REJECTED == ack_update) {
        rc = AT_ERR_SHADOW_GET_REJECTED;
    }

	IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief 初始化一个JSON文档
 *
 * 本函数主要是为JSON文档添加state字段, 即 "{\"state\":{", 所以在生成JSON文档时, 请先调用该方法
 *
 * @param jsonBuffer   为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 * @return              返回AT_ERR_SUCCESS, 表示初始化成功
 */
static int IOT_Shadow_JSON_Init(Qcloud_IoT_Shadow *pShadow, char *jsonBuffer, size_t sizeOfBuffer, bool overwrite) {

    if (jsonBuffer == NULL) {
        return AT_ERR_INVAL;
    }
	
    int32_t rc_of_snprintf = 0;
    if (overwrite) {
#ifdef TRANSFER_LABEL_NEED
		rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\\\"version\\\":%d\\, \\\"overwriteUpdate\\\":true\\, \\\"state\\\":{", pShadow->inner_data.version);
#else
    	rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\"version\":%d, \"overwriteUpdate\":true, \"state\":{", pShadow->inner_data.version);
#endif
    }
    else {
#ifdef TRANSFER_LABEL_NEED
		rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\\\"version\\\":%d\\, \\\"state\\\":{", pShadow->inner_data.version);
#else
    	rc_of_snprintf = HAL_Snprintf(jsonBuffer, sizeOfBuffer, "{\"version\":%d, \"state\":{", pShadow->inner_data.version);
#endif
    }

    return _check_snprintf_return(rc_of_snprintf, sizeOfBuffer);
}

/**
 * @brief 在JSON文档中添加结尾部分的内容, 包括clientToken字段、version字段
 *
 * @param jsonBuffer    为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer   缓冲区大小
 * @return               返回AT_ERR_SUCCESS, 表示成功
 */
static int IOT_Shadow_JSON_Finalize(Qcloud_IoT_Shadow *pShadow, char *jsonBuffer, size_t sizeOfBuffer) 
{
	int rc;
	size_t remain_size = 0;
	int32_t rc_of_snprintf = 0;

	if (jsonBuffer == NULL) {
		return AT_ERR_INVAL;
	}

	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
		return AT_ERR_JSON_BUFFER_TOO_SMALL;
	}
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "}\\, \\\"%s\\\":\\\"", CLIENT_TOKEN_FIELD);
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "}, \"%s\":\"", CLIENT_TOKEN_FIELD);
#endif
	rc = _check_snprintf_return(rc_of_snprintf, remain_size);
	if (rc != AT_ERR_SUCCESS) {
		return rc;
	}

	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
		return AT_ERR_JSON_BUFFER_TOO_SMALL;
	}

	rc_of_snprintf = generate_client_token(jsonBuffer + strlen(jsonBuffer), remain_size, &(pShadow->inner_data.token_num));
	rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != AT_ERR_SUCCESS) {
		return rc;
	}

	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
		return AT_ERR_JSON_BUFFER_TOO_SMALL;
	}

#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"}");
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"}");
#endif
	rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	return rc;
}

int IOT_Shadow_JSON_ConstructReport(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, ...) 
{
	Qcloud_IoT_Shadow* pshadow = (Qcloud_IoT_Shadow*)handle;
	POINTER_SANITY_CHECK(pshadow, AT_ERR_INVAL);
	
	int rc = IOT_Shadow_JSON_Init(pshadow, jsonBuffer, sizeOfBuffer, false);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json init failed: %d", rc);
		return rc;
	}

    size_t remain_size = 0;
    int32_t rc_of_snprintf = 0;
    int8_t i;

    if (jsonBuffer == NULL) {
        return AT_ERR_INVAL;
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
	
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"reported\\\":{");
#else
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"reported\":{");
#endif

    rc = _check_snprintf_return(rc_of_snprintf, remain_size);
    if (rc != AT_ERR_SUCCESS) {
        return rc;
    }

	va_list pArgs;
    va_start(pArgs, count);

    for (i = 0; i < count; i++) {
		DeviceProperty *pJsonNode;
        pJsonNode = va_arg(pArgs, DeviceProperty *);
        if (pJsonNode != NULL && pJsonNode->key != NULL) {
            rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);

            if (rc != AT_ERR_SUCCESS) {
                va_end(pArgs);
                return rc;
            }
        } else {
            va_end(pArgs);
            return AT_ERR_INVAL;
        }
    }

    va_end(pArgs);
    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "},");
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json add report failed: %d", rc);
		return rc;
	}

	rc = IOT_Shadow_JSON_Finalize(pshadow, jsonBuffer, sizeOfBuffer);
	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json finalize failed: %d", rc);
	}

	return rc;
}


int IOT_Shadow_JSON_ConstructReportArray(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, DeviceProperty *pDeviceProperties[]) 
{
	Qcloud_IoT_Shadow* pshadow = (Qcloud_IoT_Shadow*)handle;
	POINTER_SANITY_CHECK(pshadow, AT_ERR_INVAL);
	POINTER_SANITY_CHECK(pDeviceProperties, AT_ERR_INVAL);
	
	int rc = IOT_Shadow_JSON_Init(pshadow, jsonBuffer, sizeOfBuffer, false);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json init failed: %d", rc);
		return rc;
	}

    size_t remain_size = 0;
    int32_t rc_of_snprintf = 0;
    int8_t i;

    if (jsonBuffer == NULL) {
        return AT_ERR_INVAL;
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"reported\\\":{");
#else
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"reported\":{");
#endif
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

    if (rc != AT_ERR_SUCCESS) {
        return rc;
    }

    for (i = 0; i < count; i++) {
		DeviceProperty *pJsonNode = pDeviceProperties[i];
        if (pJsonNode != NULL && pJsonNode->key != NULL) {
            rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);

            if (rc != AT_ERR_SUCCESS) {
                return rc;
            }
        } else {
            return AT_ERR_INVAL;
        }
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "},");
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json add report failed: %d", rc);
		return rc;
	}

	rc = IOT_Shadow_JSON_Finalize(pshadow, jsonBuffer, sizeOfBuffer);
	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json finalize failed: %d", rc);
	}

	return rc;
}


int IOT_Shadow_JSON_Construct_OverwriteReport(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, ...)
{
	Qcloud_IoT_Shadow* pshadow = (Qcloud_IoT_Shadow*)handle;
	POINTER_SANITY_CHECK(pshadow, AT_ERR_INVAL);

	int rc = IOT_Shadow_JSON_Init(pshadow, jsonBuffer, sizeOfBuffer, true);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json init failed: %d", rc);
		return rc;
	}

    size_t remain_size = 0;
    int32_t rc_of_snprintf = 0;
    int8_t i;

    if (jsonBuffer == NULL) {
        return AT_ERR_INVAL;
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"reported\\\":{");
#else
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"reported\":{");
#endif
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

    if (rc != AT_ERR_SUCCESS) {
        return rc;
    }

	va_list pArgs;
    va_start(pArgs, count);

    for (i = 0; i < count; i++) {
		DeviceProperty *pJsonNode;
        pJsonNode = va_arg(pArgs, DeviceProperty *);
        if (pJsonNode != NULL && pJsonNode->key != NULL) {
            rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);

            if (rc != AT_ERR_SUCCESS) {
                va_end(pArgs);
                return rc;
            }
        } else {
            va_end(pArgs);
            return AT_ERR_INVAL;
        }
    }

    va_end(pArgs);
    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "},");
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json add report failed: %d", rc);
		return rc;
	}

	rc = IOT_Shadow_JSON_Finalize(pshadow, jsonBuffer, sizeOfBuffer);
	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json finalize failed: %d", rc);
	}

	return rc;
}

int IOT_Shadow_JSON_ConstructReportAndDesireAllNull(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, ...)
{
	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	Qcloud_IoT_Shadow* pshadow = (Qcloud_IoT_Shadow*)handle;
	
	int rc = IOT_Shadow_JSON_Init(pshadow, jsonBuffer, sizeOfBuffer, false);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json init failed: %d", rc);
		return rc;
	}

    size_t remain_size = 0;
    int32_t rc_of_snprintf = 0;
    int8_t i;

    if (jsonBuffer == NULL) {
        return AT_ERR_INVAL;
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"reported\\\":{");
#else
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"reported\":{");
#endif
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

    if (rc != AT_ERR_SUCCESS) {
        return rc;
    }

	va_list pArgs;
    va_start(pArgs, count);

    for (i = 0; i < count; i++) {
		DeviceProperty *pJsonNode;
        pJsonNode = va_arg(pArgs, DeviceProperty *);
        if (pJsonNode != NULL && pJsonNode->key != NULL) {
            rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);

            if (rc != AT_ERR_SUCCESS) {
                va_end(pArgs);
                return rc;
            }
        } else {
            va_end(pArgs);
            return AT_ERR_INVAL;
        }
    }

    va_end(pArgs);
    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "},");
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json add report failed: %d", rc);
		return rc;
	}
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"desired\\\": null ");
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"desired\": null ");
#endif
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	rc = IOT_Shadow_JSON_Finalize(pshadow, jsonBuffer, sizeOfBuffer);
	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json finalize failed: %d", rc);
	}

	return rc;
}

int IOT_Shadow_JSON_ConstructDesireAllNull(void *handle, char *jsonBuffer, size_t sizeOfBuffer) 
{
	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	Qcloud_IoT_Shadow* shadow = (Qcloud_IoT_Shadow*)handle;

	int rc = IOT_Shadow_JSON_Init(shadow, jsonBuffer, sizeOfBuffer, false);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json init failed: %d", rc);
		return rc;
	}

	size_t remain_size = 0;
	int32_t rc_of_snprintf = 0;

	if (jsonBuffer == NULL) {
		return AT_ERR_INVAL;
	}

	if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
		return AT_ERR_JSON_BUFFER_TOO_SMALL;
	}
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"desired\\\": null ");
#else
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"desired\": null ");
#endif
	rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != AT_ERR_SUCCESS) {
		return rc;
	}

	rc = IOT_Shadow_JSON_Finalize(shadow, jsonBuffer, sizeOfBuffer);
	return rc;
}

int IOT_Shadow_JSON_ConstructDesirePropNull(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, ...) 
{
	POINTER_SANITY_CHECK(handle, AT_ERR_INVAL);
	Qcloud_IoT_Shadow* shadow = (Qcloud_IoT_Shadow*)handle;

	int rc = IOT_Shadow_JSON_Init(shadow, jsonBuffer, sizeOfBuffer, false);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json init failed: %d", rc);
		return rc;
	}

    size_t remain_size = 0;
    int32_t rc_of_snprintf = 0;
    int8_t i;

    if (jsonBuffer == NULL) {
        return AT_ERR_INVAL;
    }

    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
#ifdef TRANSFER_LABEL_NEED
	rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\\\"desired\\\":{");
#else
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer), remain_size, "\"desired\":{");
#endif
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

    if (rc != AT_ERR_SUCCESS) {
        return rc;
    }

	va_list pArgs;
    va_start(pArgs, count);

    for (i = 0; i < count; i++) {
		DeviceProperty *pJsonNode;
        pJsonNode = va_arg (pArgs, DeviceProperty *);
        if (pJsonNode != NULL && pJsonNode->key != NULL) {
            rc = put_json_node(jsonBuffer, remain_size, pJsonNode->key, pJsonNode->data, pJsonNode->type);
            if (rc != AT_ERR_SUCCESS) {
				va_end(pArgs);
                return rc;
            }
        } else {
			va_end(pArgs);
            return AT_ERR_INVAL;
        }
    }

    va_end(pArgs);
    if ((remain_size = sizeOfBuffer - strlen(jsonBuffer)) <= 1) {
        return AT_ERR_JSON_BUFFER_TOO_SMALL;
    }
    // strlen(jsonBuffer) - 1 是为了把最后一项的逗号去掉
    rc_of_snprintf = HAL_Snprintf(jsonBuffer + strlen(jsonBuffer) - 1, remain_size, "},");
    rc = _check_snprintf_return(rc_of_snprintf, remain_size);

	if (rc != AT_ERR_SUCCESS) {
		Log_e("shadow json add desired failed: %d", rc);
		return rc;
	}

	rc = IOT_Shadow_JSON_Finalize(shadow, jsonBuffer, sizeOfBuffer);
	return rc;
}


