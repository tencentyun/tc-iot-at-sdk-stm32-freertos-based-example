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
#define EVENT_POST_ENABLED

#define  MODULE_TYPE					eMODULE_ESP8266
#define  TRANSFER_LABEL_NEED			1					// 转义符号是否需要。模组对json格式数据处理有些小差别


#define AT_CMD_MAX_LEN                 1024
#define RING_BUFF_LEN         		   AT_CMD_MAX_LEN	 //uart ring buffer len

#define MAX_PAYLOAD_LEN_PUB			   200			//AT+TCMQTTPUB 最长支持的数据长度，大于这个长度需要启用AT+TCMQTTPUBL

#include "at_log.h"
#include "hal_export.h"
#include "dev_config.h"
#include "module_api_inf.h"
#include "at_sanity_check.h"
#include "qcloud_iot_export_mqtt.h"
#include "qcloud_iot_export_shadow.h"
#include "qcloud_iot_export_event.h"
#include "qcloud_iot_export_error.h"



#ifdef OS_USED
#include "cmsis_os.h"
#endif

#endif
