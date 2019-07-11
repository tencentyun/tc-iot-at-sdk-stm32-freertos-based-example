/**
 ******************************************************************************
 * @file			:  at_api_export.h
 * @brief			:  all api export to application
 ******************************************************************************
 *
 * History:      <author>          <time>        <version>
 *               yougaliu          2019-3-20        1.0
 * Desc:          ORG.
 * Copyright (c) 2019 Tencent. 
 * All rights reserved.
 ******************************************************************************
 */
#ifndef _QCLOUD_API_EXPORT_H_
#define _QCLOUD_API_EXPORT_H_

/* IoT C-SDK version info */
#define QCLOUD_IOT_AT_SDK_VERSION       "1.0.0"
//#define MQTT_SHADOW_ENABLE
//#define EVENT_POST_ENABLED
#define MODULE_TYPE_WIFI


#define AT_CMD_MAX_LEN                 1024
#define RING_BUFF_LEN         		   AT_CMD_MAX_LEN	 //uart ring buffer len

#define MAX_PAYLOAD_LEN_PUB			   200			//AT+TCMQTTPUB 最长支持的数据长度，大于这个长度需要启用AT+TCMQTTPUBL




#define TRANSFER_LABEL_NEED			1		// 双引号/逗号是否需要转义。不同模组处理有些小差别,默认不需要。不定义则双引号和逗号都不需要转义
#ifdef  TRANSFER_LABEL_NEED	 
#define T_	"\\" 							//需要转义的情况下，逗号是否需要转义。目前只有ESP8266逗号需要转义
#else
#define T_	
#endif

#include "at_log.h"
#include "hal_export.h"
#include "dev_config.h"
#include "module_api_inf.h"
#include "at_sanity_check.h"
#include "qcloud_iot_export_mqtt.h"
#include "qcloud_iot_export_shadow.h"
#include "qcloud_iot_export_data_template.h"
#include "qcloud_iot_export_error.h"
#include "qcloud_iot_export_method.h"




#ifdef OS_USED
#include "cmsis_os.h"
#endif

#endif
