 /**
 ******************************************************************************
 * @file			:  module_api_inf.h
 * @brief			:  head file of api for application based on at
 ******************************************************************************
 *
 * History:      <author>          <time>        <version>
 *               yougaliu          2019-3-20        1.0
 * Desc:          ORG.
 * Copyright (c) 2019 Tencent. 
 * All rights reserved.
 ******************************************************************************
 */
#ifndef _MODULE_API_INF_H_
#define _MODULE_API_INF_H_
#include "stdint.h"
#include "dev_config.h"
#include "qcloud_iot_export_mqtt.h"

typedef enum{
	eDISCONNECTED = 0,  //未连接
	eCONNECTED = 1,		//已连接
}eMqtt_State;


typedef enum{
	eMODULE_CELLULAR = 0, //2/3/4/5G NB etc.
	eMODULE_WIFI = 1,
	eMODULE_ESP8266 = 2,
	eMODULE_N21 = 3,
	eMODULE_L206D = 4,
	eMODULE_DEFAULT = 0xFF,
}eModuleType;

eAtResault module_init(eModuleType eType);
eAtResault module_handshake(uint32_t timeout);
eAtResault module_info_set(DeviceInfo *pInfo, eTlsMode eMode);
eAtResault module_mqtt_conn(MQTTInitParams init_params);
eAtResault module_mqtt_discon(void);
eAtResault module_mqtt_pub(const char *topic, QoS eQos, char *payload);
eAtResault module_mqtt_publ(const char *topic, QoS eQos, char *payload);
eAtResault module_mqtt_sub(char *topic, QoS eQos, OnMessageHandler cb, void *contex);
eAtResault module_mqtt_unsub(const char *topic);
eAtResault module_mqtt_state(eMqtt_State *pState);
eAtResault set_module_debug_level(int Log_level);
eAtResault module_register_network(eModuleType eType);


bool IOT_MQTT_IsConnected(void);



#endif
