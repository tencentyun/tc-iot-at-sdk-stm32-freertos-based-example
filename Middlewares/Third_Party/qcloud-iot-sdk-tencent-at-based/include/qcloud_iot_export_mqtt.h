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
 
#ifndef QCLOUD_IOT_EXPORT_MQTT_H_
#define QCLOUD_IOT_EXPORT_MQTT_H_
#include "stdint.h"
#include "qcloud_iot_export_error.h"



#define 	QCLOUD_IOT_MAX_SUB_TOPIC 								(5)		//最多可订阅topic数
#define 	MAX_TOPIC_NAME_LEN     									(138)	//topic名最长长度
#define 	MAX_TOPIC_PAYLOAD_LEN   								(1024)  //topic 最长payload

/* 设备ID的最大长度, 必须保持唯一 */
#define 	MAX_SIZE_OF_CLIENT_ID                                   (80)

/* MQTT 阻塞调用(包括连接, 订阅, 发布等)的超时时间, 单位:ms 建议5000ms */
#define QCLOUD_IOT_MQTT_COMMAND_TIMEOUT                             (10 * 1000)


/* 产品名称的最大长度 */
#define MAX_SIZE_OF_PRODUCT_ID                                    	(10)

/* 产品密钥的最大长度 ，动态设备注册需要*/
#define MAX_SIZE_OF_PRODUCT_SECRET                                  (48)


/* 设备ID的最大长度 */
#define MAX_SIZE_OF_DEVICE_NAME                                     (64)

/* psk最大长度 */
#define MAX_SIZE_OF_DEVICE_SERC  	 								(24)


/* 设备证书文件名的最大长度 */
#define MAX_SIZE_OF_DEVICE_CERT_FILE_NAME                           (128)

/* 设备私钥文件名的最大长度 */
#define MAX_SIZE_OF_DEVICE_KEY_FILE_NAME                            (128)


/* 云端保留主题的最大长度 */
#define MAX_SIZE_OF_CLOUD_TOPIC            ((MAX_SIZE_OF_DEVICE_NAME) + (MAX_SIZE_OF_PRODUCT_ID) + 64)

/**
 * @brief 接入鉴权方式
 */

typedef enum{
	eNO_TLS = 0,		//直连,无TLS
	ePSK_TLS = 1,		//PSK方式TLS
	eCERT_TLS = 2,		//证书方式TLS
}eTlsMode;

typedef enum _eClientState_{
	eCLIENT_UNINITIALIZED = 0,
	eCLIENT_INITIALIZED = 1,
}eClientState;


/**
 * @brief 服务质量等级
 *
 * 服务质量等级表示PUBLISH消息分发的质量等级
 */
typedef enum _QoS {
    QOS0 = 0,    // 至多分发一次
    QOS1 = 1,    // 至少分发一次, 消息的接收者需回复PUBACK报文
    QOS2 = 2     // 仅分发一次, 目前腾讯物联云不支持该等级
} QoS;

typedef struct {
	eTlsMode 					tlsmod;
    uint32_t					command_timeout;		 // 发布订阅信令读写超时时间 ms
    uint32_t					keep_alive_interval_ms;	 // 心跳周期, 单位: s
    uint8_t         			clean_session;			 // 清理会话标志位
    uint8_t                   	auto_connect_enable;     // 是否开启自动重连 1:启用自动重连 0：不启用自动重连  建议为1
} MQTTInitParams;


/**
 * @brief MQTT初始化参数结构体默认值定义
 */
#define DEFAULT_MQTTINIT_PARAMS {ePSK_TLS, QCLOUD_IOT_MQTT_COMMAND_TIMEOUT, 240, 1, 1}

/**
 * @brief MQTT PUBLISH 消息回调处理函数指针定义
 */
typedef void (*OnMessageHandler)(char *msg, void *context);


/**
 * @brief 客户端最大订阅 Topic 数，请根据业务场景，
 * 实际订阅的最大 Topic 数情况进行配置。
 */
typedef struct _SubscribeParams_ {
	char* topicFilter;
	QoS eqos;
	OnMessageHandler fp;
	void * context;
} SubscribeParams;


eAtResault register_sub_topic(SubscribeParams* subpara);
eAtResault deliver_message(const char *data, uint32_t size);


#endif
