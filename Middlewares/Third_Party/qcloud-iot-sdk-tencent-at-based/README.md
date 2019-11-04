##  qcloud-iot-sdk-tecent-at-based 

### 介绍

qcloud-iot-sdk-tecent-at-based 面向使用支持腾讯MQTT AT指令的模组(2/3/4/5G、NB、WIFI等)接入腾讯物联网平台的终端设备开发者，典型应用场景为MCU+腾讯定制AT模组，MCU侧集成AT_SDK,AT_SDK完成实现了MCU和模组数据交互的AT框架，并基于AT框架配合腾讯AT指令，实现了MQTT、影子及数据模板的功能，同时提供了示例sample。开发者需要实现的HAL层适配接口见hal_export.h，需要实现串口的收发接口(中断接收)，延时函数，模组上下电及os相关接口适配（互斥锁、动态内存申请释放、线程创建），适配层接口单独剥离在port目录。

### qcloud-iot-sdk-tecent-at-based 软件架构

<img src="https://main.qcloudimg.com/raw/0ed501d1164b79be88e984356a9d6f45.jpg"/>

### 目录结构

| 名称            | 说明 |
| ----            | ---- |
| docs            | 文档目录，包含腾讯AT指令集定义 |
| port            | HAL层移植目录，需要实现串口的收发接口(中断接收)，延时函数，模组上下电及os相关接口|
| sample          | 应用示例，示例使用MQTT、影子、数据模板的使用方式|
| src             | AT框架及协议逻辑实现 |
|   ├───── event  | 事件功能协议封装 |
|   	├───── module_at  |at client抽象，实现RX解析，命令下行，urc匹配，resp异步匹配|
|   	├───── data_template  |基于AT框架的数据模板协议|
|   	├───── mqtt  |基于AT框架的mqtt协议实现|
|   	├───── utils  |json、timer、链表等应用|
|   	├───── include  |SDK对外头文件及设备信息配置头文件|
| tools         | 代码生成脚本 |
| README.md       | SDK使用说明 |

### 定制腾讯MQTT AT指令模组说明
  腾讯定义的MQTT AT指令见文档目录，模组侧把MQTT协议封装在模组固件中，各家模组商遵循统一的MQTT通信的AT指令名称、参数及命令返回，并实现了通过设置的设备参数及鉴权方式实现和平台的鉴权。已经支持的[模组列表](https://github.com/tencentyun/qcloud-iot-explorer-sdk-embedded-c/blob/master/docs/MCU%2B%E8%85%BE%E8%AE%AF%E4%BA%91%E5%AE%9A%E5%88%B6AT%E6%A8%A1%E7%BB%84.md)。

##### 1.1 使用MQTT AT定制模组实现MQTT通信
AT_SDK中module_api_inf.c把docs目录下的《腾讯云IoT AT指令集-V3.1.3.pdf》各AT指令独立实现为单独的函数，在mqtt_sample.c中示例了如何组合这些命令序列，包括初始化、模组上下电、联网/注册网络、设备信息写入、MQTT连接、主题订阅、消息发布、订阅消息回调处理。如果不源移植AT_SDK，可以基于如下AT指令交互流程使用MQTT功能：

![](https://main.qcloudimg.com/raw/81942fc551f6df8696490ffd5985cce9.jpg)

##### 1.2 数据模板功能
数据模板功能是基于MQTT的基础上，通过订阅特定的topic，payload部分基于为json格式实现数据协议交互，AT_SDK已是实现数据协议的封装和示例模板，可以参考示例data_template_sample.c及light_data_template_sample.c。

[数据模板协议说明](https://cloud.tencent.com/document/product/1081/34916)

### 移植指导
根据所选的嵌入式平台，适配 hal_export.h 头文件对应的hal层API的移植实现。主要有串口收发（中断接收）、模组开关机、任务/线程创建、动态内存申请/释放、时延、打印等API。可参考基于STM32+FreeRTOS的AT-SDK[移植示例](https://github.com/tencentyun/tc-iot-at-sdk-stm32-freertos-based-example.git)

##### 2.1 **hal_export.h**：
hal层对外的API接口及HAL层宏开关控制。

| 序号 | 宏定义                       | 说明                                |
| ----| ---------------------------- | -----------------------------------|
| 1   | PARSE_THREAD_STACK_SIZE      | 串口at解析线程栈大小                          							|
| 2   | OS_USED                      | 是否使用OS，目前的AT-SDK是基于多线程框架的，所以OS是必须的                  |
| 3   | AUTH_MODE_KEY                | 认证方式，证书认证还是秘钥认证                       					  |
| 4   | DEBUG_DEV_INFO_USED          | 默认使能该宏，设备信息使用调试信息，正式量产关闭该宏，并实现设备信息存取接口     |

##### 2.2 **hal_os.c**:
该源文件主要实现打印、延时、时间戳、锁、线程创建、设备信息存取等。

| 序号  | HAL_API                            | 说明                                 |
| ---- | -----------------------------------| ----------------------------------  |
| 1    | HAL_Printf                         | 打印函数，log输出需要，可选实现                    |
| 2    | HAL_Snprintf                       | 格式化打印，json数据处理需要，必须实现              |
| 3    | HAL_Vsnprintf                      | 格式化输出， 可选实现                             |
| 4    | HAL_DelayMs                        | 毫秒延时，必选实现                                |
| 5    | HAL_DelayUs                        | 微妙延时，可选实现                                |
| 6    | HAL_GetTimeMs                      | 获取毫秒数，必选实现                              |
| 7    | HAL_GetTimeSeconds                 | 获取时间戳，必须实现，时戳不需绝对准确，但不可重复    |
| 8    | hal_thread_create                  | 线程创建，必选实现                                |
| 9    | hal_thread_destroy                 | 线程销毁，必选实现                                |
| 10   | HAL_SleepMs                   		| 放权延时，必选实现                                |
| 11   | HAL_MutexCreate                 	| 互斥锁创建，必选实现                              |
| 12   | HAL_MutexDestroy                  	| 互斥锁销毁，必选实现                              |
| 13   | HAL_MutexLock                     	| 获取互斥锁，必选实现                              |
| 14   | HAL_MutexUnlock                   	| 释放互斥锁，必选实现                              |
| 15   | HAL_Malloc                			| 动态内存申请，必选实现                            |
| 16   | HAL_Free                          	| 动态内存释放，必选实现                            |
| 17   | HAL_GetDevInfo                 	| 获取设备信息，必选实现   							 |
| 18   | HAL_SetDevInfo                  	| 设置设备信息，必须存放在非易失性存储介质，必选实现    |

##### 2.3 **hal_at.c**:
该源文件主要实现AT串口初始化、串口收发、模组开关机。

| 序号  | HAL_API                         | 说明                                 		|
| ---- | -------------------------------| ----------------------------------		|
| 1    | module_power_on                | 模组开机，AT串口初始化，必选实现              |
| 1    | module_power_off               | 模组关机，低功耗需要，可选实现                |
| 2    | AT_UART_IRQHandler             | 串口接收中断ISR，将收取到的数据放入ringbuff中，AT解析线程会实时解析数据，必选实现|
| 3    | at_send_data                   | AT串口发送接口                             |

##### 2.4 **module_api_inf.c**：
配网/注网 API业务适配,该源文件基于腾讯定义的AT指令实现了MQTT的交互，但有一个关于联网/注网的API(module_register_network)需要根据模组适配。代码基于[ESP8266腾讯定制AT固件](https://main.qcloudimg.com/raw/6811fc7631dcf0ce5509ccbdba5c72b7.zip)示例了WIFI直连的方式连接网络，但更常用的场景是根据特定事件（譬如按键）触发配网（softAP/一键配网），这块的逻辑各具体业务逻辑自行实现。ESP8266有封装配网指令和示例APP。对于蜂窝模组，则是使用特定的网络注册指令。开发者参照module_handshake应用AT-SDK的AT框架添加和模组的AT指令交互。 

```	
//模组联网（NB/2/3/4G注册网络）、wifi配网（一键配网/softAP）暂时很难统一,需要用户根据具体模组适配。
//开发者参照 module_handshake API使用AT框架的API和模组交互，实现适配。
eAtResault module_register_network(eModuleType eType)
{
	eAtResault result = AT_ERR_SUCCESS;
	
#if (MODULE_TYPE == eMODULE_ESP8266)
	#define WIFI_SSID	"youga_wifi"
	#define WIFI_PW		"Iot@2018"


	/*此处示例传递热点名字直接联网，通常的做法是特定产品根据特定的事件（譬如按键）触发wifi配网（一键配网/softAP）*/
	result = wifi_connect(WIFI_SSID, WIFI_PW);
	//result |= wifi_set_test_server_ip("111.230.126.244");
	if(AT_ERR_SUCCESS != result)
	{
		Log_e("wifi connect fail,ret:%d", result);	
	}
	
#else
	/*模组网络注册、或者wifi配网需要用户根据所选模组实现*/			
#endif

	return result;
}
```

##### 2.5 设备信息修改
调试时，在hal_export.h将设备信息调试宏定义打开。量产时需要关闭该宏定义，实现hal-os中序列17-26的设备信息存取API
```
#define 	DEBUG_DEV_INFO_USED
```
修改下面宏定义的设备信息，则系统将会使用调试信息。
```
#ifdef  DEBUG_DEV_INFO_USED

static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1]	 = "PRODUCT_ID";
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1]  = "YOUR_DEVICE_NAME";

#ifdef AUTH_MODE_CERT
static char sg_device_cert_file_name[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1]      = "YOUR_DEVICE_NAME_cert.crt";
static char sg_device_privatekey_file_name[MAX_SIZE_OF_DEVICE_KEY_FILE_NAME + 1] = "YOUR_DEVICE_NAME_private.key";
#else
char sg_device_secret[MAX_SIZE_OF_DEVICE_SERC + 1] = "YOUR_IOT_PSK";
#endif

#endif
```

##### 2.6业务逻辑开发
自动生成的代码data_template_usr_logic.c，已实现数据、事件收发及响应的通用处理逻辑。但是具体的数据处理的业务逻辑需要用户自己根据业务逻辑添加，上下行业务逻辑添加的入口函数分别为deal_up_stream_user_logic 、deal_down_stream_user_logic，可以参考基于场景的示例light_data_template_sample.c添加业务处理逻辑。

**下行业务逻辑实现：**
```
/*用户需要实现的下行数据的业务逻辑,pData除字符串变量已实现用户定义的所有其他变量值解析赋值，待用户实现业务逻辑*/
static void deal_down_stream_user_logic(ProductDataDefine   * pData)
{
	Log_d("someting about your own product logic wait to be done");
}
```

**上行业务逻辑实现：**
```
/*用户需要实现的上行数据的业务逻辑,此处仅供示例*/
static int deal_up_stream_user_logic(DeviceProperty *pReportDataList[], int *pCount)
{
	int i, j;
	
     for (i = 0, j = 0; i < TOTAL_PROPERTY_COUNT; i++) {       
        if(eCHANGED == sg_DataTemplate[i].state) {
            pReportDataList[j++] = &(sg_DataTemplate[i].data_property);
			sg_DataTemplate[i].state = eNOCHANGE;
        }
    }
	*pCount = j;

	return (*pCount > 0)?AT_ERR_SUCCESS:AT_ERR_FAILURE;
}
```

##### 2.7 示例说明
Smaple目录一共有3个示例，分别是mqtt_sample.c、data_template_sample.c、light_data_template_sample.c。
各示例说明如下：

| 序号  | 示例名称                        | 说明                                 		|
| ---- | -------------------------------| ----------------------------------		|
| 1    | mqtt_sample.c                  | MQTT示例，该示例示例基于定制的AT指令如何便捷的接入腾讯物联网平台及收发数据|
| 2    | data_template_sample.c         | 通用数据模板及事件功能示例，示例如何基于腾讯物联网平台的数据模板功能快速开发产品|
| 3    | light_data_template_sample.c   | 基于智能灯的控制场景，示例具体的产品如何应用数据模板及事件功能                |

### SDK接口说明
 关于 SDK 的更多使用方式及接口了解, 参见 qcloud_iot_api_export.h



