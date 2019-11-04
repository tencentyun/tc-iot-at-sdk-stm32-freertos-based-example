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
 * @brief 数据模板相关的一些接口
 *
 */
#ifndef IOT_TEMPLATE_CLIENT_H_
#define IOT_TEMPLATE_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "at_client.h"
#include "utils_list.h"
#include "data_template_client_json.h"
#include "qcloud_iot_export_data_template.h"

#define  TEMPLATE_SYNC_TIMEOUT				(2000)
#define  MAX_CLEAE_DOC_LEN					256


typedef struct _TemplateInnerData {
    uint32_t token_num;
    int32_t sync_status;
	uint32_t eventflags;
    List *pMsgList;
	List *event_list;
    List *reply_list;
	List *action_handle_list;
    List *property_handle_list;
	char *upstream_topic;		//上行topic
    char *downstream_topic;		//下行topic
} TemplateInnerData;

typedef struct _Template {
    void *mutex;
	eClientState status;
    TemplateInnerData inner_data;
} Qcloud_IoT_Template;

typedef struct _DownStreamMsg
{
    char *data;
    int  size;
}DownStreamMsg;


/**
 * @brief 初始化data template client
 * 
 * @param pTemplate   data template client
 */
int qcloud_iot_template_init(Qcloud_IoT_Template *pTemplate);

/**
 * @brief 去初始化data template client 列表及消息订阅
 * 
 * @param pClient   data template client
 */

void qcloud_iot_template_reset(void *pClient);

/**
 * @brief 处理上行消息已经超时的回复等待
 * 
 * @param pTemplate   data template client
 */
void handle_template_expired_reply(Qcloud_IoT_Template *pTemplate);

/**
 * @brief 获取control消息的clientToken, 用于control_reply
 * 
 * @param void
 * @return clientToken
 */
char *  get_control_clientToken(void);

/**
 * @brief 数据模板消息上行
 *
 * @param pTemplate     data template client
 * @param pParams  		请求参数
 * @param pJsonDoc 		请求文档
 * @param sizeOfBuffer 	文档缓冲区大小
 * @return         		返回QCLOUD_ERR_SUCCESS, 表示成功
 */

int send_template_request(Qcloud_IoT_Template *pTemplate, RequestParams *pParams, char *pJsonDoc, size_t sizeOfBuffer);

/**
 * @brief 订阅数据模板下行topic
 *
 * @param pTemplate     template client
 * @return         		返回QCLOUD_ERR_SUCCESS, 表示成功
 */
int subscribe_template_downstream_topic(Qcloud_IoT_Template *pTemplate);

#ifdef __cplusplus
}
#endif

#endif /* IOT_TEMPLATE_CLIENT_H_ */
