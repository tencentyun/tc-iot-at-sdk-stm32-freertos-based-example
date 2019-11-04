 /**
 ******************************************************************************
 * @file			:  at_utils.h
 * @brief			:  head file of api for at utils
 ******************************************************************************
 *
 * History:      <author>          <time>        <version>
 *               yougaliu          2019-3-20        1.0
 * Desc:          ORG.
 * Copyright (c) 2019 Tencent. 
 * All rights reserved.
 ******************************************************************************
 */
#ifndef _AT_UTILS_H_
#define _AT_UTILS_H_
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define AT_PRINT_RAW_CMD
#undef  AT_PRINT_RAW_CMD

#define WIDTH_SIZE           32

#ifndef __INT_MAX__
#define __INT_MAX__     2147483647
#endif
#define INT_MAX         (__INT_MAX__)

#define AT_CMD_COMMA_MARK              ','
#define AT_CMD_DQUOTES_MARK            '"'


#define __is_print(ch)       ((unsigned int)((ch) - ' ') < 127u - ' ')

int at_vprintfln(const char *format, va_list args);
void at_print_raw_cmd(const char *name, const char *cmd, int size);
const char *at_get_last_cmd(int *cmd_size);
int at_req_parse_args(const char *req_args, const char *req_expr, ...);
int at_sscanf(const char * buf, const char * fmt, va_list args);
void at_strip(char *str, const char patten);
void chr_strip(char *str, const char patten);



#endif
