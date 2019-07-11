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

#ifndef QCLOUD_IOT_EXPORT_SHADOW_H_
#define QCLOUD_IOT_EXPORT_SHADOW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qcloud_iot_export_mqtt.h"
#include "qcloud_iot_export_method.h"

/**
 * @brief 构造ShadowClient
 *
 * @param eType 影子类型
 *
 * @return @see eAtResault
 */
eAtResault IOT_Shadow_Construct(void **client);


/**
 * @brief 获取ShadowClient
 *
 * @return ShadowClient
 */
void *get_shadow_client(void);


/**
 * @brief 客户端目前是否已连接
 *
 * @param handle     影子client
 * @return 返回true, 表示客户端已连接
 */
bool IOT_Shadow_IsConnected(void *handle);

/**
 * @brief 销毁ShadowClient 关闭MQTT连接
 *
 * @param pClient ShadowClient对象
 *
 * @return 返回AT_ERR_SUCCESS, 表示成功
 */
int IOT_Shadow_Destroy(void *handle);

/**
 * @brief 消息接收, 心跳包管理, 超时请求处理
 *
 * @param handle 	 影子client
 * @param timeout_ms 超时时间, 单位:ms
 * @return           返回AT_ERR_SUCCESS, 表示调用成功
 */
void IOT_Shadow_Yield(void *handle, uint32_t timeout_ms);

/**
 * @brief 异步方式更新设备影子文档
 *
 * @param pClient           Client结构体
 * @param pJsonDoc          更新到云端的设备文档
 * @param sizeOfBuffer      文档长度
 * @param callback          请求响应处理回调函数
 * @param userContext       用户数据, 请求响应返回时通过回调函数返回
 * @param timeout_ms        请求超时时间, 单位:ms
 * @return                  返回AT_ERR_SUCCESS, 表示请求成功
 */
int IOT_Shadow_Update(void *handle, char *pJsonDoc, size_t sizeOfBuffer, OnRequestCallback callback, void *userContext, uint32_t timeout_ms);

/**
 * @brief 同步方式更新设备影子文档
 *
 * @param pClient           Client结构体
 * @param pJsonDoc          更新到云端的设备文档
 * @param sizeOfBuffer      文档长度
 * @param timeout_ms        请求超时时间, 单位:ms
 * @return                  AT_ERR_SUCCESS 请求成功
 *                          QCLOUD_ERR_SHADOW_UPDATE_TIMEOUT 请求超时
 *                          QCLOUD_ERR_SHADOW_UPDATE_REJECTED 请求被拒绝
 */
int IOT_Shadow_Update_Sync(void *handle, char *pJsonDoc, size_t sizeOfBuffer, uint32_t timeout_ms);

/**
 * @brief 获取设备影子文档
 *
 * @param pClient           Client结构体
 * @param callback          请求响应处理回调函数
 * @param userContext       用户数据, 请求响应返回时通过回调函数返回
 * @param timeout_ms        请求超时时间, 单位:s
 * @return                  返回AT_ERR_SUCCESS, 表示请求成功
 */
int IOT_Shadow_Get(void *handle, OnRequestCallback callback, void *userContext, uint32_t timeout_ms);

/**
 * @brief 获取设备影子文档
 *
 * @param pClient           Client结构体
 * @param timeout_ms        请求超时时间, 单位:s
 * @return                  AT_ERR_SUCCESS 请求成功
 *                          QCLOUD_ERR_SHADOW_GET_TIMEOUT 请求超时
 *                          QCLOUD_ERR_SHADOW_GET_REJECTED 请求被拒绝
 */
int IOT_Shadow_Get_Sync(void *handle, uint32_t timeout_ms);

/**
 * @brief 注册当前设备的设备属性
 *
 * @param pClient    Client结构体
 * @param pProperty  设备属性
 * @param callback   设备属性更新回调处理函数
 * @return           返回AT_ERR_SUCCESS, 表示请求成功
 */
int IOT_Shadow_Register_Property(void *handle, DeviceProperty *pProperty, OnPropRegCallback callback);

/**
 * @brief 删除已经注册过的设备属性
 *
 * @param pClient    Client结构体
 * @param pProperty  设备属性
 * @return           返回AT_ERR_SUCCESS, 表示请求成功
 */
int IOT_Shadow_UnRegister_Property(void *handle, DeviceProperty *pProperty);

/**
 * @brief 在JSON文档中添加reported字段，不覆盖更新
 *
 *
 * @param jsonBuffer    为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 * @param count         可变参数的个数, 即需上报的设备属性的个数
 * @return              返回AT_ERR_SUCCESS, 表示成功
 */
int IOT_Shadow_JSON_ConstructReport(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, ...);



/**
 * @brief 在JSON文档中添加reported字段，不覆盖更新
 *
 *
 * @param jsonBuffer    为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 * @param count         需上报的设备属性的个数
 * @param pDeviceProperties         需上报的设备属性的个数
 * @return              返回AT_ERR_SUCCESS, 表示成功
 */

int IOT_Shadow_JSON_ConstructReportArray(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, DeviceProperty *pDeviceProperties[]);


/**
 * @brief 在JSON文档中添加reported字段，覆盖更新
 *
 *
 * @param jsonBuffer    为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 * @param overwrite		重写字段
 * @param count         可变参数的个数, 即需上报的设备属性的个数
 * @return              返回AT_ERR_SUCCESS, 表示成功
 */
int IOT_Shadow_JSON_Construct_OverwriteReport(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, ...);

/**
 * @brief 在JSON文档中添加reported字段，同时清空desired字段
 *
 *
 * @param jsonBuffer    为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 * @param count         可变参数的个数, 即需上报的设备属性的个数
 * @return              返回AT_ERR_SUCCESS, 表示成功
 */
int IOT_Shadow_JSON_ConstructReportAndDesireAllNull(void *handle, char *jsonBuffer, size_t sizeOfBuffer, uint8_t count, ...);

/**
 * @brief 在JSON文档中添加 "desired": null 字段
 *
 * @param jsonBuffer   为存储JSON文档准备的字符串缓冲区
 * @param sizeOfBuffer  缓冲区大小
 */
int IOT_Shadow_JSON_ConstructDesireAllNull(void *handle, char *jsonBuffer, size_t sizeOfBuffer);

#ifdef __cplusplus
}
#endif

#endif /* QCLOUD_IOT_EXPORT_SHADOW_H_ */

