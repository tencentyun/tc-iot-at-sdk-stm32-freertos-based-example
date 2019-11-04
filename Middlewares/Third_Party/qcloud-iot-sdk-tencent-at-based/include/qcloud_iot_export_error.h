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

#ifndef QCLOUD_IOT_EXPORT_ERROR_H_
#define QCLOUD_IOT_EXPORT_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * AT SDK 错误码
 */

typedef enum _eAtResault_{
    QCLOUD_RET_SUCCESS   									 = 0,      // 表示成功返回
    QCLOUD_ERR_FAILURE   									 = -100,   // 表示失败返回
    QCLOUD_ERR_INVAL     									 = -101,   // 表示参数无效错误
    QCLOUD_ERR_NULL       									 = -102,   // 表示空指针
    QCLOUD_ERR_MALLOC     									 = -103,   // 表示内存申请失败
    QCLOUD_ERR_SEND_DATA   									 = -104,   // 表示AT串口发送失败
   

	QCLOUD_ERR_TIMEOUT	  									 = -201,	//响应超时	
	QCLOUD_ERR_RESP_NULL  									 = -202,	//响应数据指针无效
	QCLOUD_ERR_EXCEED_MAX_TOPICS 							 = -203,  //超过最多topic可订阅数
	QCLOUD_ERR_MQTT_NO_CONN									 = -204,  //MQTT未连接


    QCLOUD_ERR_JSON_PARSE                                    = -301,    // 表示JSON解析错误
    QCLOUD_ERR_JSON_BUFFER_TRUNCATED                         = -302,    // 表示JSON文档会被截断
    QCLOUD_ERR_JSON_BUFFER_TOO_SMALL                         = -303,    // 表示存储JSON文档的缓冲区太小
    QCLOUD_ERR_JSON                                          = -304,    // 表示JSON文档生成错误
    QCLOUD_ERR_MAX_JSON_TOKEN                                = -305,    // 表示超过JSON文档中的最大Token数
    QCLOUD_ERR_MAX_APPENDING_REQUEST                         = -306,    // 表示超过同时最大的文档请求
    QCLOUD_ERR_MAX_TOPIC_LENGTH                              = -307,    // 表示超过规定最大的topic长度


	QCLOUD_ERR_PROPERTY_EXIST                        		 = -401,    // 表示注册的属性已经存在
    QCLOUD_ERR_PROPERTY_NOT_EXIST                     		 = -402,    // 表示注册的属性不存在
    QCLOUD_ERR_REPORT_TIMEOUT               		         = -403,    // 表示更新设备影子文档超时
    QCLOUD_ERR_REPORT_REJECTED                     			 = -404,    // 表示更新设备影子文档被拒绝
    QCLOUD_ERR_GET_TIMEOUT                            	     = -405,    // 表示拉取设备影子文档超时
    QCLOUD_ERR_GET_REJECTED                          	     = -406,    // 表示拉取设备影子文档被拒绝
	QCLOUD_ERR_ACTION_EXIST                                  = -407,    // acion already exist
    QCLOUD_ERR_ACTION_NOT_EXIST                              = -408,    // acion not exist
} eAtResault;


#ifdef __cplusplus
}
#endif

#endif /* QCLOUD_IOT_EXPORT_ERROR_H_ */
