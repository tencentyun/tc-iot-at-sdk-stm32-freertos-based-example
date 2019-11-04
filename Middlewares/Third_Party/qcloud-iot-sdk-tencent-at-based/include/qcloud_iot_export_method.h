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

#ifndef QCLOUD_IOT_EXPORT_METHOD_H_
#define QCLOUD_IOT_EXPORT_METHOD_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 请求响应返回的类型
 */
typedef enum {
    ACK_NONE = -3,      // 请求超时
    ACK_TIMEOUT = -2,   // 请求超时
    ACK_REJECTED = -1,  // 请求拒绝
    ACK_ACCEPTED = 0    // 请求接受
} RequestAck;

typedef RequestAck ReplyAck;

/**
 * @brief 操作云端设备文档可以使用的三种方式
 */
typedef enum {
    GET = 0,     // 获取云端设备文档
    REPORT = 1,  // 更新或创建云端设备文档    
    RINFO = 2,   // 上报系统信息
    REPLY = 3,   // 数据模板服务端下行消息回复
    CLEAR = 4,   // 删除离线期间的control数据
} Method;

/**
 * @brief JSON文档中支持的数据类型
 */
typedef enum {
    JINT32,     // 32位有符号整型
    JINT16,     // 16位有符号整型
    JINT8,      // 8位有符号整型
    JUINT32,    // 32位无符号整型
    JUINT16,    // 16位无符号整型
    JUINT8,     // 8位无符号整型
    JFLOAT,     // 单精度浮点型
    JDOUBLE,    // 双精度浮点型
    JBOOL,      // 布尔型
    JSTRING,    // 字符串
    JOBJECT     // JSON对象
} JsonDataType;

/**
 * @brief 定义设备的某个属性, 实际就是一个JSON文档节点
 */
typedef struct _JSONNode {
    char   		 *key;    // 该JSON节点的Key
    void         *data;   // 该JSON节点的Value
    uint16_t     data_buff_len;  // data buff len, for string type value update
    JsonDataType type;    // 该JSON节点的数据类型
} DeviceProperty;


/**
 * @brief Define a device action
 */
typedef struct {
    char    *pActionId;     // action id
    uint32_t timestamp;	    // action timestamp
    uint8_t  input_num;     // input num
    uint8_t  output_num;    // output mun
    DeviceProperty *pInput;  // input 
    DeviceProperty *pOutput; // output
} DeviceAction;


/**
 * @brief 每次文档请求响应的回调函数
 *
 * @param method         文档操作方式
 * @param requestAck     请求响应类型
 * @param pJsonDocument  云端响应返回的文档
 * @param userContext      用户数据
 *
 */
typedef void (*OnRequestCallback)(void *pClient, Method method, RequestAck requestAck, const char *pJsonDocument, void *userContext);


/**
 * @brief  上行消息响应的回调函数
 *
 * @param method         回复的消息方法类型
 * @param replyAck       请求响应类型
 * @param pJsonDocument  云端响应返回的文档
 * @param userContext      用户数据
 *
 */
typedef void (*OnReplyCallback)(void *pClient, Method method, ReplyAck replyAck, const char *pJsonDocument, void *userContext);


/**
 * @brief 设备属性处理回调函数
 *
 * @param pJsonValueBuffer 设备属性值
 * @param valueLength      设备属性值长度
 * @param DeviceProperty   设备属性结构体
 */
typedef void (*OnPropRegCallback)(void *pClient, const char *pJsonValueBuffer, uint32_t valueLength, DeviceProperty *pProperty);


/**
 * @brief action handle callback 
 *
 * @param pAction   action with input data
 */
typedef void (*OnActionHandleCallback)(void *pClient, const char *pClientToken, DeviceAction *pAction);

#ifdef __cplusplus
}
#endif

#endif /* QCLOUD_IOT_EXPORT_METHOD_H_ */
