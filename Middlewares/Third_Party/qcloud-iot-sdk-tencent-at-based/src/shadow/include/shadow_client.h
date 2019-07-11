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

/**
 * @brief 设备影子文档操作相关的一些接口
 *
 * 这里提供一些接口用于管理设备影子文档或与设备影子文档进行交互; 通过DeviceName,
 * 可以与设备影子进行交互, 包括当前设备的设备影子和其他设备的设备影子; 一
 * 个设备一共有三种不同操作与设备影子交互:
 *     1. Get
 *     2. Update
 *     3. Delete
 *
 * 以上三种操作, 底层还是基于MQTT协议, 工作原理也是基于发布/订阅模型, 当执行
 * 上述操作是, 会收到相应的响应: 1. accepted; 2. rejected。例如, 我们执行
 * Get与设备影子进行交互, 设备端将发送和接收到一下信息:
 *     1. 发布MQTT主题: $shadow/get/{productName}/{deviceName};
 *     2. 订阅MQTT主题: $shadow/get/accepted/{productName}/{deviceName} 和 $shadow/get/rejected/{productName}/{deviceName}
 *     3. 如果整个请求成功的话, 设备端会收到accepted主题, 以及相应设备的json文档。
 */
#ifndef IOT_SHADOW_CLIENT_H_
#define IOT_SHADOW_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "at_client.h"
#include "utils_list.h"
#include "utils_method.h"
#include "qcloud_iot_export_shadow.h"


typedef struct _ShadowInnerData {
    uint32_t token_num;
    int32_t sync_status;
    List *request_list;
    List *property_handle_list;
    char *result_topic;
} ShadowInnerData;

typedef struct _Shadow {
    void *mutex;
    ShadowInnerData inner_data;
	eClientState status;
} Qcloud_IoT_Shadow;




void qcloud_iot_shadow_reset(void *pClient);

/**
 * @brief 处理请求队列中已经超时的请求
 * 
 * @param pShadow   shadow client
 */
void handle_expired_request(Qcloud_IoT_Shadow *pShadow);

/**
 * @brief 所有的云端设备文档操作请求, 通过该方法进行中转分发
 *
 * @param pShadow       shadow client
 * @param pParams  		请求参数
 * @param pJsonDoc 		请求文档
 * @param sizeOfBuffer 	文档缓冲区大小
 * @return         		返回AT_ERR_SUCCESS, 表示成功
 */
int do_shadow_request(Qcloud_IoT_Shadow *pShadow, RequestParams *pParams, char *pJsonDoc, size_t sizeOfBuffer);

/**
 * @brief 订阅设备影子操作结果topic
 *
 * @param pShadow       shadow client
 * @param pParams  		请求参数
 * @param pJsonDoc 		请求文档
 * @param sizeOfBuffer 	文档缓冲区大小
 * @return         		返回AT_ERR_SUCCESS, 表示成功
 */
eAtResault subscribe_operation_result_to_cloud(Qcloud_IoT_Shadow *pShadow);

#ifdef __cplusplus
}
#endif

#endif /* IOT_SHADOW_CLIENT_H_ */
