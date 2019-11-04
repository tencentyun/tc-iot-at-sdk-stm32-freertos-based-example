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

#ifndef QCLOUD_IOT_EXPORT_DATA_TEMPLATE_H_
#define QCLOUD_IOT_EXPORT_DATA_TEMPLATE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
#include "qcloud_iot_export_mqtt.h"
#include "qcloud_iot_export_method.h"

#define  MAX_CONTORL_REPLY_STATUS_LEN		64		   // control回复消息中，status字串的最大长度，可以更具具体产品场景修改

/**
 * @brief 定义数据模板的数据点类型
 */
#define TYPE_TEMPLATE_INT    	JINT32
#define TYPE_TEMPLATE_ENUM    	JINT32
#define TYPE_TEMPLATE_FLOAT  	JFLOAT
#define TYPE_TEMPLATE_BOOL   	JINT8
#define TYPE_TEMPLATE_STRING 	JSTRING
#define TYPE_TEMPLATE_TIME 		JUINT32
#define TYPE_TEMPLATE_JOBJECT 	JOBJECT

typedef int32_t   TYPE_DEF_TEMPLATE_INT;
typedef int32_t   TYPE_DEF_TEMPLATE_ENUM;
typedef float     TYPE_DEF_TEMPLATE_FLOAT;
typedef char      TYPE_DEF_TEMPLATE_BOOL;
typedef char      TYPE_DEF_TEMPLATE_STRING;
typedef uint32_t  TYPE_DEF_TEMPLATE_TIME;
typedef void *    TYPE_DEF_TEMPLATE_OBJECT;

#ifdef EVENT_POST_ENABLED					//是否使能数据模板的事件功能

#define TYPE_STR_INFO			"info"
#define TYPE_STR_ALERT			"alert"
#define TYPE_STR_FAULT			"fault"

//如果使用事件时间戳，必须保证时间戳是准确的UTC时间ms，否则会判断为错误
//#define  EVENT_TIMESTAMP_USED			 

#define  FLAG_EVENT0 			(1U<<0)
#define  FLAG_EVENT1			(1U<<1)
#define  FLAG_EVENT2			(1U<<2)
#define  FLAG_EVENT3			(1U<<3)
#define  FLAG_EVENT4 			(1U<<4)
#define  FLAG_EVENT5			(1U<<5)
#define  FLAG_EVENT6			(1U<<6)
#define  FLAG_EVENT7			(1U<<7)
#define  FLAG_EVENT8 			(1U<<8)
#define  FLAG_EVENT9			(1U<<9)

#define  ALL_EVENTS_MASK		(0xFFFFFFFF)


typedef enum {
	eEVENT_INFO,
	eEVENT_ALERT,    
    eEVENT_FAULT, 
}eEventType;

typedef struct  _sEvent_{
	char 	 *event_name;		 //事件名称	
	char 	 *type;			 	 //事件类型	
    uint32_t timestamp;			 //事件时戳	
	uint8_t eventDataNum;		 //事件属性点个数
    DeviceProperty *pEventData;  //事件属性点
} sEvent;

#endif

typedef enum _eControlReplyCode_{
		eDEAL_SUCCESS = 0,
		eDEAL_FAIL = -1,
}eReplyCode;

/**
 * @brief control msg reply 参数
 */
typedef struct _sControlReplyPara {

    uint32_t  timeout_ms;         						      // 请求超时时间, 单位:ms
    
    eReplyCode   code;    							  // 回复code，0：成功 非0：失败

    char      status_msg[MAX_CONTORL_REPLY_STATUS_LEN];       // 附加信息

} sReplyPara;


/**
 * @brief 定义数据模板的属性状态
 */
typedef enum _eDataState_{
    eNOCHANGE = 0,
	eCHANGED = 1,	
} eDataState;

/**
 * @brief 定义数据模板的属性结构
 */
typedef struct {
    DeviceProperty data_property;
    eDataState state;
} sDataPoint;

/**
 * @brief 获取data template client
 *
 * @return ShadowClient
 */
void *get_template_client(void);


/**
 * @brief 构造TemplateClient
 *
 * @param client 待返回的构造成功的data template client
 *
 * @return 返回NULL: 构造失败
 */
int IOT_Template_Construct(void **client);

/**
 * @brief 客户端目前是否已连接
 *
 * @param pClient Template Client结构体
 * @return 返回true, 表示客户端已连接
 */
bool IOT_Template_IsConnected(void *handle);

/**
 * @brief 销毁TemplateClient 关闭MQTT连接
 *
 * @param pClient TemplateClient对象
 *
 * @return 返回QCLOUD_ERR_SUCCESS, 表示成功
 */
int IOT_Template_Destroy(void *handle);

/**
 * @brief 消息接收, 事件回复超时管理, 超时请求处理
 *
 * @param handle     data template client
 * @param timeout_ms 超时时间, 单位:ms
 */
void IOT_Template_Yield(void *handle, uint32_t timeout_ms);


/**
 * @brief 注册当前设备的设备属性
 *
 * @param pClient    Client结构体
 * @param pProperty  设备属性
 * @param callback   设备属性更新回调处理函数
 * @return           返回QCLOUD_ERR_SUCCESS, 表示请求成功
 */
int IOT_Template_Register_Property(void *handle, DeviceProperty *pProperty, OnPropRegCallback callback);

/**
 * @brief 删除已经注册过的设备属性
 *
 * @param pClient    Client结构体
 * @param pProperty  设备属性
 * @return           返回QCLOUD_ERR_SUCCESS, 表示请求成功
 */
int IOT_Template_UnRegister_Property(void *handle, DeviceProperty *pProperty);

/**
 * @brief 在JSON文档中添加reported字段，不覆盖更新
 *
 *
 * @param jsonBuffer    为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 * @param count         可变参数的个数, 即需上报的设备属性的个数
 * @return              返回QCLOUD_ERR_SUCCESS, 表示成功
 */
int IOT_Template_JSON_ConstructReportArray(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, DeviceProperty *pDeviceProperties[]); 

/**
 * @brief  数据模板异步方式上报数据
 *
 * @param pClient             data template client
 * @param pJsonDoc          待上报的数据模板的文档数据   
 * @param sizeOfBuffer      文档长度
 * @param callback            上报数据的 响应处理回调函数
 * @param userContext       用户数据, 请求响应返回时通过回调函数返回
 * @param timeout_ms        请求超时时间, 单位:ms
 * @return                  返回QCLOUD_ERR_SUCCESS, 表示上报成功
 */
int IOT_Template_Report(void *handle, char *pJsonDoc, size_t sizeOfBuffer, OnReplyCallback callback, void *userContext, uint32_t timeout_ms);

/**
 * @brief  数据模板同步方式上报数据
 *
 * @param pClient             data template client
 * @param pJsonDoc          待上报的数据模板的文档数据   
 * @param sizeOfBuffer      文档长度
 * @param timeout_ms       请求超时时间, 单位:ms
 * @return                  返回QCLOUD_ERR_SUCCESS, 表示上报成功
 */
int IOT_Template_Report_Sync(void *handle, char *pJsonDoc, size_t sizeOfBuffer, uint32_t timeout_ms);


 /**
  * @brief	异步方式获取数据云端数据模板状态
  *                 一般用于同步设备离线期间服务端的下行数据
  *
  * @param pClient		   data template client
  * @param callback 		   数据获取请求的 响应处理回调函数
  * @param userContext	   用户数据, 请求响应返回时通过回调函数返回
  * @param timeout_ms	   请求超时时间, 单位:ms
  * @return 				  返回QCLOUD_ERR_SUCCESS, 表示请求成功
  */
int IOT_Template_GetStatus(void *handle, OnReplyCallback callback, void *userContext, uint32_t timeout_ms);

 /**
  * @brief	同步方式获取数据云端数据模板状态
  * @param pClient		   data template client
  * @param timeout_ms	   请求超时时间, 单位:ms
  * @return 				  返回QCLOUD_ERR_SUCCESS, 表示请求成功
  */
int IOT_Template_GetStatus_sync(void *handle, uint32_t timeout_ms);

/**
 * @brief  删除离线期间的control data
 * @param pClient		  data template client
 * @param code	  		  0：成功     非0：失败
 * @param pClientToken	  对应的get_status_reply	中的clientToken
 * @return					 返回QCLOUD_ERR_SUCCESS, 表示处理成功
 */
int IOT_Template_ClearControl(void *handle, char *pClientToken, OnRequestCallback callback, uint32_t timeout_ms); 


/**
 * @brief  回复下行control消息
 * @param pClient		   data template client
 * @param jsonBuffer		   用于构造回复包的数据buffer
 * @param sizeOfBuffer	  数据长度
 *	@param replyPara		  回复参数，成功/失败及对应的附加信息  
 * @return					 返回QCLOUD_ERR_SUCCESS, 表示构造成功
 */ 
int IOT_Template_ControlReply(void *handle, char *pJsonDoc, size_t sizeOfBuffer, sReplyPara *replyPara);

/**
 * @brief  构造系统信息上报json数据包
 * @param pClient		   data template client
 * @param jsonBuffer	   用于构造数据buffer
 * @param sizeOfBuffer	   数据长度
 * @param pPlatInfo		   平台系统信息，必须 
 * @param pSelfInfo		   设备商自定义的系统信息，可选
 * @return				   返回QCLOUD_ERR_SUCCESS, 表示构造成功

 *系统信息上报包格式
* {
*  "method": "report_info",
*  "clientToken": "client-token1618",
*  "params": {
*  "module_hardinfo": "模组具体硬件型号  N10,,带模组设备必填",
*  "module_softinfo":  "模组软件版本,带模组设备必填",
*  "fw_ver":       "mcu固件版本,必填",
*  "imei":       "设备imei号，可选上报",
*  "lat":        "纬度，需换算为10进制,可选上报,需要位置服务设备必填,移动设备需要实时获取"
*  "lon":        "经度，需换算为10进制,可选上报,需要位置服务设备必填,移动设备需要实时获取",
*  "device_label": {
*    "append_info": "设备商自定义的产品附加信息"
* ...
*   }
* }
*/
int IOT_Template_JSON_ConstructSysInfo(void *handle, char *jsonBuffer, size_t sizeOfBuffer, DeviceProperty *pPlatInfo, DeviceProperty *pSelfInfo); 

/**
 * @brief 异步方式上报系统信息
 * @param pClient		   data template client
 * @param jsonBuffer	   系统信息Json包
 * @param sizeOfBuffer	   系统信息Json包长度
 * @param callback		   响应回调
 * @param userContext	   用户数据, 响应返回时通过回调函数返回
 * @param timeout_ms	   响应超时时间, 单位:ms
 * @return				   返回QCLOUD_ERR_SUCCESS, 表示上报成功
*/
int IOT_Template_Report_SysInfo(void *handle, char *pJsonDoc, size_t sizeOfBuffer, OnReplyCallback callback, void *userContext, uint32_t timeout_ms);

/**
 * @brief 同步方式上报系统信息
 * @param pClient		   data template client
 * @param jsonBuffer	   系统信息Json包
 * @param sizeOfBuffer	   系统信息Json包长度
 * @param timeout_ms	   同步等待响应超时时间, 单位:ms
 * @return				   返回QCLOUD_ERR_SUCCESS, 表示上报成功
*/
int IOT_Template_Report_SysInfo_Sync(void *handle, char *pJsonDoc, size_t sizeOfBuffer, uint32_t timeout_ms);


#ifdef EVENT_POST_ENABLED
/**
 * @brief 事件上报回复回调。
 *
 * @param msg    	 事件响应返回的文档
 * @param context    data template client
 *
 */
typedef void (*OnEventReplyCallback)(char *msg, void *context);


/**
 * @brief 设置事件标记
 *
 * @param  flag  设置发生的事件集
 */
void IOT_Event_setFlag(void *client, uint32_t flag);

/**
 * @brief 清除事件标记
 *
 * @param  flag  待清除的事件集
 */
void IOT_Event_clearFlag(void *client, uint32_t flag);

/**
 * @brief 获取已置位的事件集
 *
 * @return 已置位的事件集
 */
uint32_t IOT_Event_getFlag(void *client);

/**
 * @brief 事件client初始化，使用事件功能前需先调用
 *
 * @param c    shadow 实例指针
 */
int IOT_Event_Init(void *c);

/**
 * @brief 处理事件队列中已经超时的请求
 * 
 * @param client   data template client
 */

void handle_template_expired_event(void *client);


/**
 * @brief 事件上报，传入事件数组，SDK完成事件的json格式封装
 * @param pClient shadow 实例指针
 * @param pJsonDoc    用于构建json格式上报信息的buffer
 * @param sizeOfBuffer    用于构建json格式上报信息的buffer大小
 * @param event_count     待上报的事件个数
 * @param pEventArry	  待上报的事件数组指
 * @param replyCb	  事件回复消息的回调 
 * @return @see IoT_Error_Code	  
 */
int IOT_Post_Event(void *pClient, char *pJsonDoc, size_t sizeOfBuffer, uint8_t event_count, sEvent *pEventArry[], OnEventReplyCallback replyCb);                                            

/**
 * @brief 事件上报，用户传入已构建好的事件的json格式，SDK增加事件头部即上报
 * @param pClient shadow 实例指针
 * @param pJsonDoc    用于构建json格式上报信息的buffer
 * @param sizeOfBuffer    用于构建json格式上报信息的buffer大小
 * @param pEventMsg     待上报的事件json信息 
 *  json事件格式：
 *  单个事件：
 *	 {"method": "event_post",
 *		"clientToken": "123",
 *		"version": "1.0",
 *		"eventId": "PowerAlarm",
 *		"type": "fatal",
 *		"timestamp": 1212121221,
 *		"params": {
 *			"Voltage": 2.8,
 *			"Percent": 20
 *		}
 *	}
 *
 *  多个事件：
 *	 {
 *		 "eventId": "PowerAlarm",
 *		 "type": "fatal",
 *		 "timestamp": 1212121221,
 *		 "params": {
 *			 "Voltage": 2.8,
 *			 "Percent": 20
 *		 }
 *	 },
 *	 {
 *		 "name": "PowerAlarm",
 *		 "type": "fatal",
 *		 "timestamp": 1212121223,
 *		 "params": {
 *			 "Voltage": 2.1,
 *			 "Percent": 10
 *		 }
 *	 },
 *   ....
 *
 * @param replyCb	  事件回复消息的回调 
 * @return @see IoT_Error_Code	  
 */
int IOT_Post_Event_Raw(void *pClient, char *pJsonDoc, size_t sizeOfBuffer, char *pEventMsg, OnEventReplyCallback replyCb);                                            


#endif

#ifdef ACTION_ENABLED
/**
 * @brief 注册当前设备的行为
 *
 * @param pClient    Client结构体
 * @param pProperty  设备属性
 * @param callback   设备属性更新回调处理函数
 * @return           返回QCLOUD_ERR_SUCCESS, 表示请求成功
 */

int IOT_Template_Register_Action(void *handle, DeviceAction *pAction, OnActionHandleCallback callback);

/**
 * @brief 删除已经注册过的设备行为
 *
 * @param pClient    Client结构体
 * @param pProperty  设备属性
 * @return           返回QCLOUD_ERR_SUCCESS, 表示请求成功
 */

int IOT_Template_UnRegister_Action(void *handle, DeviceAction *pAction); 

/**
* @brief 设备行为回复 
* @param pClient		  handle to data_template client
* @param pClientToken	  correspond to the clientToken of action msg 
* @param pJsonDoc	  	  data buffer for reply
* @param sizeOfBuffer     length of data buffer
* @param pAction 		  pointer of action 	
* @param replyPara        action reply info
* @return				  QCLOUD_RET_SUCCESS when success, or err code for failure
*/ 

int IOT_ACTION_REPLY(void *pClient, const char *pClientToken, char *pJsonDoc, size_t sizeOfBuffer, DeviceAction *pAction, sReplyPara *replyPara);

#endif



#ifdef __cplusplus
}
#endif

#endif /* QCLOUD_IOT_EXPORT_TEMPLATE_H_ */
