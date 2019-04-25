##  qcloud-iot-sdk-tecent-at-based 

### 介绍

qcloud-iot-sdk-tecent-at-based 面向使用支持腾讯AT指令的模组(2/3/4/5G、NB、WIFI等)接入腾讯物联网平台的终端设备开发者，典型应用场景为MCU+腾讯定制AT模组，SDK完成实现了MCU和模组数据交互的AT框架，并基于AT框架配合腾讯AT指令，实现了MQTT、影子及数据模板的功能，同时提供了示例sample。开发者需要实现的HAL层适配接口见hal_export.h，需要实现串口的收发接口(中断接收)，延时函数，模组上下电及os相关接口适配（互斥锁、动态内存申请释放、线程创建），适配层接口单独剥离在port目录。

### qcloud-iot-sdk-tecent-at-based软件架构

<img src="https://main.qcloudimg.com/raw/bfbb342b888712edcdcc69a3b0665507.png"/>

### 目录结构

| 名称            | 说明 |
| ----            | ---- |
| docs            | 文档目录，包含腾讯AT指令集定义 |
| port            | HAL层移植目录，需要实现串口的收发接口(中断接收)，延时函数，模组上下电及os相关接口|
| sample          | 应用示例，示例使用MQTT、影子、数据模板的使用方式|
| src             | AT框架及协议逻辑实现 |
|   ├───── event  | 事件功能协议封装 |
|   	├───── module_at  |at client抽象，实现RX解析，命令下行，urc匹配，resp异步匹配|
|   	├───── shadow  |基于AT框架的shadow逻辑实现|
|   	├───── mqtt  |基于AT框架的mqtt协议实现|
|   	├───── utils  |json、timer、链表等应用|
|   	├───── include  |SDK对外头文件及设备信息配置头文件|
| tools         | 代码生成脚本 |
| README.md       | SDK使用说明 |

### 移植指导
根据所选的嵌入式平台，适配 hal_export.h 头文件对应的hal层API的移植实现。主要有串口收发（中断接收）、模组开关机、任务/线程创建、动态内存申请/释放、时延、打印等API。可参考基于STM32+FreeRTOS的AT-SDK[移植示例](http://git.code.oa.com/iotcloud_teamIII/tc-iot-at-sdk-stm32-freertos-based-example.git)

##### 2.1 **hal_export.h**：hal层对外的API接口及HAL层宏开关控制。

| 序号 | 宏定义                       | 说明                                |
| ----| ---------------------------- | -----------------------------------|
| 1   | PARSE_THREAD_STACK_SIZE      | 串口at解析线程栈大小                          							|
| 2   | OS_USED                      | 是否使用OS，目前的AT-SDK是基于多线程框架的，所以OS是必须的                  |
| 3   | AUTH_MODE_KEY                | 认证方式，证书认证还是秘钥认证                       					  |
| 4   | DEBUG_DEV_INFO_USED          | 默认使能该宏，设备信息使用调试信息，正式量产关闭该宏，并实现设备信息存取接口     |

#####2.2  **hal_os.c:**该源文件主要实现打印、延时、时间戳、锁、线程创建、设备信息存取等

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

#####2.3  **hal_at.c:**该源文件主要实现AT串口初始化、串口收发、模组开关机

| 序号  | 方法名                         | 说明                                 		|
| ---- | -------------------------------| ----------------------------------		|
| 1    | module_power_on                | 模组开机，AT串口初始化，必选实现              |
| 1    | module_power_off               | 模组关机，低功耗需要，可选实现                |
| 2    | AT_UART_IRQHandler             | 串口中断接收，将收取到的数据放入ringbuff中，AT解析线程会实时解析数据，必选实现|
| 3    | at_send_data                   | AT串口发送接口                             |

#####2.4  **module_api_inf.c:**：配网/注网 API业务适配
该源文件基于腾讯定义的AT指令实现了MQTT的交互，但有一个关于联网/注网的API(module_register_network)需要根据模组适配。
示例基于[ESP8266腾讯定制AT固件](http://git.code.oa.com/iotcloud_teamIII/qcloud-iot-at-esp8266-wifi.git)示例了WIFI直连的方式连接网络，但更常用的场景是根据特定事件（譬如按键）触发配网（softAP/一键配网），这块的逻辑各具体业务逻辑自行实现。ESP8266有封装配网指令和示例APP。对于蜂窝模组，则是使用特定的网络注册指令。开发者参照module_handshake应用AT-SDK的AT框架添加和模组的AT指令交互。 

<img src="https://main.qcloudimg.com/raw/3d8e6135365099c15ab67bbe816b7a01.jpg"/>

#####2.5 设备信息修改
调试时，在hal_export.h将设备信息调试宏定义打开。量产时需要关闭该宏定义，实现hal-os中序列17-26的设备信息存取API
```
#define 	DEBUG_DEV_INFO_USED
```
修改下面宏定义的设备信息，则系统将会使用调试信息。
```
#ifdef  DEBUG_DEV_INFO_USED

static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1]	 = "03UKNYBUZG";
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1]  = "at_dev";

#ifdef AUTH_MODE_CERT
static char sg_device_cert_file_name[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1]      = "YOUR_DEVICE_NAME_cert.crt";
static char sg_device_privatekey_file_name[MAX_SIZE_OF_DEVICE_KEY_FILE_NAME + 1] = "YOUR_DEVICE_NAME_private.key";
#else
char sg_device_secret[MAX_SIZE_OF_DEVICE_SERC + 1] = "ttOARy0PjYgzd9OSs4Z3RA==";
#endif

#endif
```
#####2.6 示例说明
Smaple目录一共有四个示例，分别是mqtt_sample.c、shadow_sample.c、data_template_sample.c、light_data_template_sample.c。
各示例说明如下：

| 序号  | 示例名称                        | 说明                                 		|
| ---- | -------------------------------| ----------------------------------		|
| 1    | mqtt_sample.c                  | MQTT示例，该示例示例基于定制的AT指令如何便捷的接入腾讯物联网平台及收发数据|
| 1    | shadow_sample.c                | 影子示例，基于AT实现的MQTT协议，进一步封装的影子协议               |
| 2    | data_template_sample.c         | 通用数据模板及事件功能示例，示例如何基于腾讯物联网平台的数据模板功能快速开发产品|
| 3    | light_data_template_sample.c   | 基于智能灯的控制场景，示例具体的产品如何应用数据模板及事件功能                |

**数据模板功能说明**

**相关文档链接**   
[影子协议说明](https://cloud.tencent.com/document/product/634/11918)  
[影子快速入门](https://cloud.tencent.com/document/product/634/11914#c-sdk-.E6.93.8D.E4.BD.9C.E6.AD.A5.E9.AA.A4)  
[数据模板说明](https://cloud.tencent.com/document/product/634/11914#c-sdk-.E6.93.8D.E4.BD.9C.E6.AD.A5.E9.AA.A4)


### SDK接口说明
 关于 SDK 的更多使用方式及接口了解, 参见 qcloud_iot_api_export.h
