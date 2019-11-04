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

#ifndef UTILS_METHOD_H_
#define UTILS_METHOD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_export.h"
#include "qcloud_iot_export_method.h"
#include "qcloud_iot_export_mqtt.h"




#define min(a,b) (a) < (b) ? (a) : (b)


/* 在任意给定时间内, 处于appending状态的请求最大个数 */
#define MAX_APPENDING_REQUEST_AT_ANY_GIVEN_TIME                     (10)

/* 在任意给定时间内, 可以同时操作设备的最大个数 */
#define MAX_DEVICE_HANDLED_AT_ANY_GIVEN_TIME                        (10)

/* 除设备名称之外, 云端保留主题的最大长度 */
#define MAX_SIZE_OF_CLOUD_TOPIC_WITHOUT_DEVICE_NAME                 (60)

/* 接收云端返回的JSON文档的buffer大小 */
#define CLOUD_IOT_JSON_RX_BUF_LEN                                   (CLINET_BUFF_LEN + 1)


/* 一个clientToken的最大长度 */
#define MAX_SIZE_OF_CLIENT_TOKEN                                    (MAX_SIZE_OF_CLIENT_ID + 10)

/* 一个仅包含clientToken字段的JSON文档的最大长度 */
#define MAX_SIZE_OF_JSON_WITH_CLIENT_TOKEN                          (MAX_SIZE_OF_CLIENT_TOKEN + 20)


#define CLIENT_TOKEN_FIELD     		"clientToken"
#define METHOD_FIELD	         	"method"
#define TYPE_FIELD	         		"type"
#define ACTION_ID_FIELD     		"actionId"
#define TIME_STAMP_FIELD     		"timestamp"

#define REPLY_CODE					"code"
#define REPLY_STATUS				"status"

#define GET_STATUS					"get_status"			//设备主动查询数据状态
#define GET_STATUS_REPLY			"get_status_reply"		//服务端回复数据状态查询

#define CONTROL_CMD					"control"				//服务端下行控制命令
#define CONTROL_CMD_REPLY			"control_reply"			//设备回复控制命令

#define CLEAR_CONTROL				"clear_control"

#define REPORT_CMD					"report"				//设备上行上报数据
#define REPORT_CMD_REPLY			"report_reply"			//服务端回复数据上报

#define INFO_CMD					"report_info"			//设备信息上报
#define INFO_CMD_REPLY				"report_info_reply"		//设备信息上报回复


#define GET_CONTROL_PARA			"data.control"
#define CMD_CONTROL_PARA			"params"

/**
 * @brief 文档操作请求的参数结构体定义
 */
typedef struct _RequestParam {

    Method               	method;              	// 文档请求方式: GET, REPORT, RINFO, REPLY,CLEAR

    uint32_t             	timeout_sec;         	// 请求超时时间, 单位:s

    OnRequestCallback    	request_callback;    	// 请求回调方法

    void                 	*user_context;          // 用户数据, 会通过回调方法OnRequestCallback返回

} RequestParams;

#define DEFAULT_REQUEST_PARAMS {GET, 4, NULL, NULL};

/**
 * @brief 代表一个文档请求
 */
typedef struct {
    char                   client_token[MAX_SIZE_OF_CLIENT_TOKEN];          // 标识该请求的clientToken字段
    Method                 method;                                          // 文档操作方式

    void                   *user_context;                                   // 用户数据
    Timer                  timer;                                           // 请求超时定时器

    OnRequestCallback      callback;                                        // 文档操作请求返回处理函数
} Request;

/**
 * @brief 用于生成不同的主题
 */
typedef enum {
    ACCEPTED, REJECTED, METHOD
} RequestType;


/**
 * @brief 该结构体用于保存已登记的设备属性及设备属性处理的回调方法
 */
typedef struct {

    void *property;							// 设备属性

    OnPropRegCallback callback;      // 回调处理函数

} PropertyHandler;


/**
 * @brief save the action registed and its callback
 */
typedef struct {

    void *action;							

    OnActionHandleCallback callback;      

} ActionHandler;


/**
 * @brief 检查函数snprintf的返回值
 *
 * @param returnCode       函数snprintf的返回值
 * @param maxSizeOfWrite   可写最大字节数
 * @return                 返回QCLOUD_ERR_JSON, 表示出错; 返回QCLOUD_ERR_JSON_BUFFER_TRUNCATED, 表示截断
 */

int check_snprintf_return(int32_t returnCode, size_t maxSizeOfWrite);

/**
 * @brief 初始化请求参数
 *
 * @param pParams   待初始化请求参数指针
 * @param method    请求方法
 * @param callback  请求回调
 * @param userContext   用户数据
 */
void init_request_params(RequestParams *pParams, Method method, OnRequestCallback callback, void *userContext, uint8_t timeout_sec);


/**
 * @brief 将字源符串插入到目标字符串指定位置
 *
 * @param pSourceStr   源字符串
 * @param pDestStr     目标输出字符串
 * @param pos          目标串插入位置
 */
void insert_str(char *pDestStr, char *pSourceStr, int pos);


/**
 * 将一个JSON节点写入到JSON串中
 *
 * @param jsonBuffer   	JSON串
 * @param sizeOfBuffer  可写入大小
 * @param pKey          JSON节点的key
 * @param pData         JSON节点的value
 * @param type          JSON节点value的数据类型
 * @return              返回QCLOUD_ERR_SUCCESS, 表示成功
 */
int put_json_node(char *jsonBuffer, size_t sizeOfBuffer, const char *pKey, void *pData, JsonDataType type);

/**
 * 将一个JSON节点写入到JSON串中,物模型对bool类型的处理有区分。
 *
 * @param jsonBuffer   	JSON串
 * @param sizeOfBuffer  可写入大小
 * @param pKey          JSON节点的key
 * @param pData         JSON节点的value
 * @param type          JSON节点value的数据类型
 * @return              返回QCLOUD_ERR_SUCCESS, 表示成功
 */
int template_put_json_node(char *jsonBuffer, size_t sizeOfBuffer, const char *pKey, void *pData, JsonDataType type);


/**
 * @brief 返回一个ClientToken
 *
 * @param pStrBuffer    存储ClientToken的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 * @param tokenNumber   shadow的token值，函数内部每次执行完会自增
 * @return              返回QCLOUD_ERR_SUCCESS, 表示成功
 */
int generate_client_token(char *pStrBuffer, size_t sizeOfBuffer, uint32_t *tokenNumber);

/**
 * @brief 为GET和DELETE请求构造一个只带有clientToken字段的JSON文档
 *
 * @param tokenNumber   shadow的token值，函数内部每次执行完会自增
 * @param pJsonBuffer 存储JSON文档的字符串缓冲区
 */
void build_empty_json(uint32_t *tokenNumber, char *pJsonBuffer);

/**
 * @brief 从JSON文档中解析出clientToken字段
 *
 * @param pJsonDoc       待解析的JSON文档
 * @param pClientToken   ClientToken字段
 * @return               返回true, 表示解析成功
 */
bool parse_client_token(char *pJsonDoc, char **pClientToken);


/**
 * @brief parse field of aciont_id from JSON string
 *
 * @param pJsonDoc       source JSON string
 * @param pActionID   	 pointer to field of action_id
 * @return               true for success
 */
bool parse_action_id(char *pJsonDoc, char **pActionID);

/**
 * @brief parse field of timestamp from JSON string
 *
 * @param pJsonDoc       source JSON string
 * @param pTimestamp     pointer to field of timestamp
 * @return               true for success
 */
bool parse_timestamp(char *pJsonDoc, int32_t *pTimestamp);


/**
 * @brief parse field of input from JSON string
 *
 * @param pJsonDoc       source JSON string
 * @param pActionInput   filed of params as action input parameters
 * @return               true for success
 */ 
bool parse_action_input(char *pJsonDoc, char **pActionInput);


/**
 * @brief 从JSON文档中解析出status字段,事件回复
 *
 * @param pJsonDoc       待解析的JSON文档
 * @param pStatus   	 status字段
 * @return               返回true, 表示解析成功
 */
bool parse_status_return(char *pJsonDoc, char **pStatus);

/**
 * @brief 从JSON文档中解析出code字段,事件回复
 *
 * @param pJsonDoc       待解析的JSON文档
 * @param pCode   		 Code字段
 * @return               返回true, 表示解析成功
 */
bool parse_code_return(char *pJsonDoc, int32_t *pCode);


/**
 * @brief 如果JSON文档中的key与某个设备属性的key匹配的话, 则更新该设备属性, 该设备属性的值不能为OBJECT类型
 *
 * @param pJsonDoc       JSON文档
 * @param pProperty      设备属性
 * @return               返回true, 表示成功
 */
bool update_value_if_key_match(char *pJsonDoc, DeviceProperty *pProperty);

/**
 * @brief 从JSON文档中解析出method字段
 *
 * @param pJsonDoc         待解析的JSON文档
 * @param pErrorMessage    输出method字段
 * @return                 返回true, 表示解析成功
 */
bool parse_template_method_type(char *pJsonDoc, char **pMethod);

/**
 * @brief 从get_status_reply 的JSON文档中解析出control字段
 *
 * @param pJsonDoc         待解析的JSON文档
 * @param pErrorMessage    输出control字段
 * @return                 返回true, 表示解析成功
 */
bool parse_template_get_control(char *pJsonDoc, char **control);

/**
 * @brief 从control 的JSON文档中解析出control字段
 *
 * @param pJsonDoc         待解析的JSON文档
 * @param pErrorMessage    输出control字段
 * @return                 返回true, 表示解析成功
 */
bool parse_template_cmd_control(char *pJsonDoc, char **control);


#ifdef __cplusplus
}
#endif

#endif /* QCLOUD_IOT_EXPORT_SHADOW_H_ */
