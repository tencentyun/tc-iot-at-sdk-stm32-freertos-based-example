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

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "at_log.h"
#include "hal_export.h"


#define MAX_LOG_MSG_LEN 			(512)

#if defined(__linux__)
	#undef  MAX_LOG_MSG_LEN
	#define MAX_LOG_MSG_LEN                  (1023)
#endif

static char *level_str[] = {
    "DBG", "INF", "WRN", "ERR",
};

LOG_LEVEL g_log_level = LOG_DEBUG;

static const char *_get_filename(const char *p)
{
    char ch = '/';
    const char *q = strrchr(p,ch);
    if(q == NULL)
    {
        q = p;
    }
    else
    {
        q++;
    }
    return q;
}

void At_Log_Set_Level(LOG_LEVEL logLevel) {
    g_log_level = logLevel;
}

LOG_LEVEL At_Log_Get_Level(void) {
    return g_log_level;
}

static bool log_lock = false; //support multi thread 
void Log_writter(const char *file, const char *func, const int line, const int level, const char *fmt, ...)
{
	if (level < g_log_level) {
		return;
	}

	if(log_lock){
		return;
	}else{
		log_lock = true;
	}
		
	const char *file_name = _get_filename(file);

    HAL_Printf("%s|%s|%s(%d): ", level_str[level],  file_name, func, line);

    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    HAL_Printf("\r\n");

	log_lock = false;
    return;
}

void HexDump(const    uint8_t *pData, unsigned int len)
{
	for(int i = 0; i < len; i++)
	{
		HAL_Printf("%02x ",pData[i]);
	}
	HAL_Printf("\n\r");
}


#ifdef __cplusplus
}
#endif
