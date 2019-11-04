/**
******************************************************************************
* @file           : at_client.c
* @brief          : at client header file
******************************************************************************
*
* Copyright (c) 2019 Tencent. 
* All rights reserved.
******************************************************************************
*/	

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "qcloud_iot_api_export.h"
#include "at_sanity_check.h"
#include "at_client.h"
#include "at_utils.h"






#define AT_RESP_END_OK                 "OK"
#define AT_RESP_END_ERROR              "ERROR"
#define AT_RESP_END_FAIL               "FAIL"
#define AT_END_CR_LF                   "\r\n"

#ifndef OS_USED
#define MAX_RESP_COUNT  10
#define RESP_BUFF_LEN	512

static char g_respbuff[MAX_RESP_COUNT][RESP_BUFF_LEN];
static at_response  g_resplist[MAX_RESP_COUNT];

static uint8_t sg_client_recv_buff[CLINET_BUFF_LEN];
static uint8_t sg_Uart_Recv_Buff[RING_BUFF_LEN];
#endif

sRingbuff g_ring_buff;	
static at_client sg_at_client;




#ifdef OS_USED	
/**
 * Create response object.
 *
 * @param buf_size the maximum response buffer size
 * @param line_num the number of setting response lines
 *		   = 0: the response data will auto return when received 'OK' or 'ERROR'
 *		  != 0: the response data will return when received setting lines number data
 * @param timeout the maximum response time
 *
 * @return != NULL: response object
 *			= NULL: no memory
 */
at_response_t at_create_resp(uint32_t buf_size, uint32_t line_num, uint32_t timeout)
{
	at_response_t resp = NULL;

	resp = (at_response_t) HAL_Malloc(sizeof(at_response));
	if (resp == NULL)
	{
		Log_e("AT create response object failed! No memory for response object!");
		return NULL;
	}

	resp->buf = (char *) HAL_Malloc(buf_size);
	if (resp->buf == NULL)
	{
		Log_e("AT create response object failed! No memory for response buffer!");
		HAL_Free(resp);
		return NULL;
	}

	resp->buf_size = buf_size;
	resp->line_num = line_num;
	resp->line_counts = 0;
	resp->timeout = timeout;

	return resp;
}


/**
 * Delete and free response object.
 *
 * @param resp response object
 */
void at_delete_resp(at_response_t resp)
{
    if (resp && resp->buf)
    {
        HAL_Free(resp->buf);
    }

    if (resp)
    {
        HAL_Free(resp);
        resp = NULL;
    }
}
#else
at_response_t _find_empty_resp(at_response_t p_resplist) 
{

	int i;

	if(NULL == p_resplist)
	{
		Log_e("null input");
		return NULL;
	}


    for (i = 0; i < MAX_RESP_COUNT; i++) 
	{
        if (NULL == p_resplist[i].buf) 
		{
			memset(g_respbuff[i], 0, RESP_BUFF_LEN);
			p_resplist[i].buf = g_respbuff[i];
			return &p_resplist[i];
        }
    }

	Log_e("resp list is full");	
    return NULL;
}



at_response_t at_create_resp(uint32_t buf_size, uint32_t line_num, uint32_t timeout)
{
	at_response_t resp;

	resp = _find_empty_resp(g_resplist);
	if(NULL == resp)
	{
		Log_e("no resp find");
		return NULL;
	}

	resp->buf_size = buf_size;
	resp->line_num = line_num;
	resp->line_counts = 0;
	resp->timeout = timeout;

	return resp;
}

void at_delete_resp(at_response_t p_resp)
{
  	POINTER_SANITY_CHECK_RTN(p_resp);
   
    memset(p_resp->buf, 0, p_resp->buf_size);
	p_resp->buf = NULL;
	
    return ;
}

#endif

/**
 * Get one line AT response buffer by line number.
 *
 * @param resp response object
 * @param resp_line line number, start from '1'
 *
 * @return != NULL: response line buffer
 *			= NULL: input response line error
 */
const char *at_resp_get_line(at_response_t resp, uint32_t resp_line)
{
	char *resp_buf = resp->buf;
	char *resp_line_buf = NULL;
	int line_num = 1;

	POINTER_SANITY_CHECK(resp, NULL);

	if (resp_line > resp->line_counts || resp_line <= 0)
	{
		Log_e("AT response get line failed! Input response line(%d) error!", resp_line);
		return NULL;
	}

	for (line_num = 1; line_num <= resp->line_counts; line_num++)
	{
		if (resp_line == line_num)
		{
			resp_line_buf = resp_buf;

			return resp_line_buf;
		}

		resp_buf += strlen(resp_buf) + 1;
	}

	return NULL;
}

/**
 * Get one line AT response buffer by keyword
 *
 * @param resp response object
 * @param keyword query keyword
 *
 * @return != NULL: response line buffer
 *			= NULL: no matching data
 */
const char *at_resp_get_line_by_kw(at_response_t resp, const char *keyword)
{
	char *resp_buf = resp->buf;
	char *resp_line_buf = NULL;
	int line_num = 1;

	POINTER_SANITY_CHECK(resp, NULL);
	POINTER_SANITY_CHECK(keyword, NULL);

	for (line_num = 1; line_num <= resp->line_counts; line_num++)
	{
		if (strstr(resp_buf, keyword))
		{
			resp_line_buf = resp_buf;

			return resp_line_buf;
		}

		resp_buf += strlen(resp_buf) + 1;
	}

	return NULL;
}

/**
 * Get and parse AT response buffer arguments by line number.
 *
 * @param resp response object
 * @param resp_line line number, start from '1'
 * @param resp_expr response buffer expression
 *
 * @return -1 : input response line number error or get line buffer error
 *			0 : parsed without match
 *		   >0 : the number of arguments successfully parsed
 */
int at_resp_parse_line_args(at_response_t resp, uint32_t resp_line, const char *resp_expr, ...)
{
	va_list args;
	int resp_args_num = 0;
	const char *resp_line_buf = NULL;


	POINTER_SANITY_CHECK(resp, -1);
	POINTER_SANITY_CHECK(resp_expr, -1);
	if ((resp_line_buf = at_resp_get_line(resp, resp_line)) == NULL)
	{
		return -1;
	}

	va_start(args, resp_expr);

	resp_args_num = vsscanf(resp_line_buf, resp_expr, args);

	va_end(args);

	return resp_args_num;
}

/**
 * Get and parse AT response buffer arguments by keyword.
 *
 * @param resp response object
 * @param keyword query keyword
 * @param resp_expr response buffer expression
 *
 * @return -1 : input keyword error or get line buffer error
 *			0 : parsed without match
 *		   >0 : the number of arguments successfully parsed
 */
int at_resp_parse_line_args_by_kw(at_response_t resp, const char *keyword, const char *resp_expr, ...)
{
	va_list args;
	int resp_args_num = 0;
	const char *resp_line_buf = NULL;


	POINTER_SANITY_CHECK(resp, -1); 
	POINTER_SANITY_CHECK(resp_expr, -1); 
	if ((resp_line_buf = at_resp_get_line_by_kw(resp, keyword)) == NULL)
	{
		return -1;
	}

	va_start(args, resp_expr);

	resp_args_num = vsscanf(resp_line_buf, resp_expr, args);

	va_end(args);

	return resp_args_num;
}

/**
 * Send commands to AT server and wait response.
 *
 * @param client current AT client object
 * @param resp AT response object, using NULL when you don't care response
 * @param cmd_expr AT commands expression
 *
 * @return 0 : success
 *		  -1 : response status error
 *		  -2 : wait timeout
 */
eAtResault at_obj_exec_cmd(at_response_t resp, const char *cmd_expr, ...)
{
	POINTER_SANITY_CHECK(cmd_expr, QCLOUD_ERR_NULL);

	va_list args;
	int cmd_size = 0;
	Timer timer;
	eAtResault result = QCLOUD_RET_SUCCESS;
	const char *cmd = NULL;
	at_client_t client = at_client_get();
	

	if (client == NULL)
	{
		Log_e("input AT Client object is NULL, please create or get AT Client object!");
		return QCLOUD_ERR_FAILURE;
	}
#ifdef OS_USED
	HAL_MutexLock(client->lock);
#endif
	resp->line_counts = 0;
	client->resp_status = AT_RESP_OK;
	client->resp = resp;

	va_start(args, cmd_expr);
	at_vprintfln(cmd_expr, args);
	va_end(args);

	if (resp != NULL)
	{		
		HAL_Timer_countdown_ms(&timer, resp->timeout);
		do
		{
			if(client->resp_notice)
			{
				if (client->resp_status != AT_RESP_OK)
				{
					cmd = at_get_last_cmd(&cmd_size);
					Log_e("execute command (%.*s) failed!", cmd_size, cmd);
					result = QCLOUD_ERR_FAILURE;
					goto __exit;
				}
				break;
			}
		}while (!HAL_Timer_expired(&timer));

		if(HAL_Timer_expired(&timer))
		{
			cmd = at_get_last_cmd(&cmd_size);
			Log_e("execute command (%.*s) timeout %dms!", cmd_size, cmd, resp->timeout);
			client->resp_status = AT_RESP_TIMEOUT;
			result = QCLOUD_ERR_RESP_NULL;
		}
	}

__exit:
	client->resp = NULL;
	client->resp_notice = false;

#ifdef OS_USED
	HAL_MutexUnlock(client->lock);
#endif
	
	return result;
}

/**
 * Waiting for connection to external devices.
 *
 * @param client current AT client object
 * @param timeout millisecond for timeout
 *
 * @return 0 : success
 *		  -2 : timeout
 *		  -5 : no memory
 */
eAtResault at_client_wait_connect(uint32_t timeout)
{
	eAtResault result = QCLOUD_RET_SUCCESS;
	at_response_t resp = NULL;
	at_client_t client = at_client_get();
	Timer timer;


	if (client == NULL)
	{
		Log_e("input AT Client object is NULL, please create or get AT Client object!");
		return QCLOUD_ERR_FAILURE;
	}

	resp = at_create_resp(16, 0, CMD_TIMEOUT_MS);
	if (resp == NULL)
	{
		Log_e("No memory for response object!");
		return QCLOUD_ERR_RESP_NULL;
	}

#ifdef OS_USED
	HAL_MutexLock(client->lock);
#endif

	client->resp = resp;
	resp->line_counts = 0;

	HAL_Timer_countdown_ms(&timer, timeout);



	/* Check whether it is already connected */	
	do
	{		
		at_send_data("AT\r\n", 4);	
		//Log_d("AT cmd send");
#ifdef OS_USED	
		HAL_SleepMs(CMD_RESPONSE_INTERVAL_MS);
#else
		HAL_DelayMs(CMD_RESPONSE_INTERVAL_MS);	
#endif	
		if (client->resp_notice)
			break;
		else
			continue;
	}while (!HAL_Timer_expired(&timer));

	if(HAL_Timer_expired(&timer))
	{
		Log_d("read ring buff timeout");
		result = QCLOUD_ERR_TIMEOUT;
	}

	at_delete_resp(resp);

	client->resp = NULL;
	
#ifdef OS_USED
	HAL_MutexUnlock(client->lock);
#endif
	return result;
}

/**
 * Send data to AT server, send data don't have end sign(eg: \r\n).
 *
 * @param client current AT client object
 * @param buf	send data buffer
 * @param size	send fixed data size
 *
 * @return >0: send data size
 *		   =0: send failed
 */
int at_client_send(at_client_t client, char *buf, int size)
{
	POINTER_SANITY_CHECK(buf, 0);

	if (client == NULL)
	{
		Log_e("input AT Client object is NULL, please create or get AT Client object!");
		return 0;
	}

#ifdef AT_PRINT_RAW_CMD
	at_print_raw_cmd("send", buf, size);
#endif

	return at_send_data((uint8_t *)buf, size);
}

static eAtResault at_client_getchar(at_client_t client, char *pch, uint32_t timeout)
{
	eAtResault ret = QCLOUD_RET_SUCCESS;
	Timer timer;

	HAL_Timer_countdown_ms(&timer, timeout);
	do   
    {
    	if(0 == ring_buff_pop_data(client->pRingBuff, (uint8_t *)pch, 1))
    	{
			continue;
		}
		else
		{
			break;
		}    
    } while (!HAL_Timer_expired(&timer));

    if(HAL_Timer_expired(&timer))
    {
		//Log_i("read ring buff timeout");
		ret = QCLOUD_ERR_TIMEOUT;
	}
	

	return ret;
}

/**
 * AT client receive fixed-length data.
 *
 * @param client current AT client object
 * @param buf	receive data buffer
 * @param size	receive fixed data size
 * @param timeout  receive data timeout (ms)
 *
 * @note this function can only be used in execution function of URC data
 *
 * @return >0: receive data size
 *		   =0: receive failed
 */
int at_client_obj_recv(at_client_t client, char *buf, int size, int timeout)
{
	int read_idx = 0;
	eAtResault result = QCLOUD_RET_SUCCESS;
	char ch;

	POINTER_SANITY_CHECK(buf, 0);

	if (client == NULL)
	{
		Log_e("input AT Client object is NULL, please create or get AT Client object!");
		return 0;
	}

	while (1)
	{
		if (read_idx < size)
		{
			result = at_client_getchar(client, &ch, timeout);
			if (result != QCLOUD_RET_SUCCESS)
			{
				Log_e("AT Client receive failed, uart device get data error(%d)", result);
				return 0;
			}

			buf[read_idx++] = ch;
		}
		else
		{
			break;
		}
	}

#ifdef AT_PRINT_RAW_CMD
	at_print_raw_cmd("urc_recv", buf, size);
#endif

	return read_idx;
}

/**
 *	AT client set end sign.
 *
 * @param client current AT client object
 * @param ch the end sign, can not be used when it is '\0'
 */
void at_set_end_sign(char ch)
{
	at_client_t client = at_client_get();
	
	if (client == NULL)
	{
		Log_e("input AT Client object is NULL, please create or get AT Client object!");
		return;
	}

	client->end_sign = ch;
}

/**
 * set URC(Unsolicited Result Code) table
 *
 * @param client current AT client object
 * @param table URC table
 * @param size table size
 */
void at_set_urc_table(at_client_t client, const at_urc_t urc_table, uint32_t table_sz)
{
	int idx;

	if (client == NULL)
	{
		Log_e("input AT Client object is NULL, please create or get AT Client object!");
		return;
	}

	for (idx = 0; idx < table_sz; idx++)
	{
		POINTER_SANITY_CHECK_RTN(urc_table[idx].cmd_prefix);
		POINTER_SANITY_CHECK_RTN(urc_table[idx].cmd_suffix);		
	}

	client->urc_table = urc_table;
	client->urc_table_size = table_sz;
}


at_client_t at_client_get(void)
{
    return &sg_at_client;
}

static const at_urc *get_urc_obj(at_client_t client)
{
	int i, prefix_len, suffix_len;
	int buf_sz;
	char *buffer = NULL;

	if (client->urc_table == NULL)
	{
		return NULL;
	}

	buffer = client->recv_buffer;
	buf_sz = client->cur_recv_len;

	for (i = 0; i < client->urc_table_size; i++)
	{
		prefix_len = strlen(client->urc_table[i].cmd_prefix);
		suffix_len = strlen(client->urc_table[i].cmd_suffix);
		if (buf_sz < prefix_len + suffix_len)
		{
			continue;
		}
		if ((prefix_len ? !strncmp(buffer, client->urc_table[i].cmd_prefix, prefix_len) : 1)
				&& (suffix_len ? !strncmp(buffer + buf_sz - suffix_len, client->urc_table[i].cmd_suffix, suffix_len) : 1))
		{
			//Log_d("matched:%s", client->urc_table[i].cmd_prefix);
			return &client->urc_table[i];
		}
	}

	return NULL;
}

static int at_recv_readline(at_client_t client)
{
	int read_len = 0;
	char ch = 0, last_ch = 0;
	bool is_full = false;
	eAtResault ret;

	memset(client->recv_buffer, 0x00, client->recv_bufsz);
	client->cur_recv_len = 0;

	while (1)
	{
		ret = at_client_getchar(client, &ch, GET_CHAR_TIMEOUT_MS);
		if(QCLOUD_RET_SUCCESS != ret)
		{
			return ret;
		}

		if (read_len < client->recv_bufsz)
		{
			client->recv_buffer[read_len++] = ch;
			client->cur_recv_len = read_len;
		}
		else
		{
			is_full = true;
		}

		/* is newline or URC data */
		if ((ch == '\n' && last_ch == '\r') ||(client->end_sign != 0 && ch == client->end_sign)
				|| get_urc_obj(client))
		{
			if (is_full)
			{
				Log_e("read line failed. The line data length is out of buffer size(%d)!", client->recv_bufsz);
				memset(client->recv_buffer, 0x00, client->recv_bufsz);
				client->cur_recv_len = 0;
				ring_buff_flush(client->pRingBuff);
				return QCLOUD_ERR_FAILURE;
			}
			break;
		}
		last_ch = ch;
	}

#ifdef AT_PRINT_RAW_CMD
	at_print_raw_cmd("recvline", client->recv_buffer, read_len);
#endif

	return read_len;
}

static void client_parser(void *userContex)
{
	int resp_buf_len = 0;
	const at_urc *urc;
	int line_counts = 0;
	at_client_t client = at_client_get();


	Log_d("client_parser start...");
	client->status = AT_STATUS_INITIALIZED;
	while(1)
	{
		if (at_recv_readline(client) > 0)
		{
		
#ifdef 	AT_PRINT_RAW_CMD	
			const char *cmd = NULL;
			int cmdsize = 0;
			cmd = at_get_last_cmd(&cmdsize);
			Log_d("last_cmd:(%.*s), readline:%s",  cmdsize, cmd, client->recv_buffer);
#endif			
			if ((urc = get_urc_obj(client)) != NULL)
			{
				/* current receive is request, try to execute related operations */
				if (urc->func != NULL)
				{
					urc->func(client->recv_buffer, client->cur_recv_len);
				}
			}
			else if (client->resp != NULL)
			{
				/* current receive is response */
				client->recv_buffer[client->cur_recv_len - 1] = '\0';
				if (resp_buf_len + client->cur_recv_len < client->resp->buf_size)
				{
					/* copy response lines, separated by '\0' */
					memcpy(client->resp->buf + resp_buf_len, client->recv_buffer, client->cur_recv_len);
					resp_buf_len += client->cur_recv_len;

					line_counts++;					
				}
				else
				{
					client->resp_status = AT_RESP_BUFF_FULL;
					Log_e("Read response buffer failed. The Response buffer size is out of buffer size(%d)!", client->resp->buf_size);
				}
				/* check response result */
				if (memcmp(client->recv_buffer, AT_RESP_END_OK, strlen(AT_RESP_END_OK)) == 0
						&& client->resp->line_num == 0)
				{
					/* get the end data by response result, return response state END_OK. */
					client->resp_status = AT_RESP_OK;
				}
				else if (strstr(client->recv_buffer, AT_RESP_END_ERROR)
						|| (memcmp(client->recv_buffer, AT_RESP_END_FAIL, strlen(AT_RESP_END_FAIL)) == 0))
				{
					client->resp_status = AT_RESP_ERROR;
				}
				else if (line_counts == client->resp->line_num && client->resp->line_num)
				{
					/* get the end data by response line, return response state END_OK.*/
					client->resp_status = AT_RESP_OK;
				}
				else
				{
					continue;
				}
				client->resp->line_counts = line_counts;

				client->resp = NULL;
				client->resp_notice = true;
				resp_buf_len = 0;
				line_counts = 0;
			}
			else
			{
//				  Log_d("unrecognized line: %.*s", client->cur_recv_len, client->recv_buffer);
			}
		}
		else
		{
			//Log_d("read no new line");
		}
		
	}
}

/* initialize the client parameters */
eAtResault at_client_para_init(at_client_t client)
{

	client->status = AT_STATUS_UNINITIALIZED;

#ifdef OS_USED
	client->lock = HAL_MutexCreate();
	if(NULL == client->lock)
	{
		Log_e("create lock err");
		return QCLOUD_ERR_FAILURE;
	}

	char * ringBuff = HAL_Malloc(RING_BUFF_LEN);
	if(NULL == ringBuff)
	{
		Log_e("malloc ringbuff err");
		return QCLOUD_ERR_FAILURE;
	}
	ring_buff_init(&g_ring_buff, ringBuff,  RING_BUFF_LEN);

	char * recvBuff = HAL_Malloc(CLINET_BUFF_LEN);
	if(NULL == recvBuff)
	{
		Log_e("malloc recvbuff err");
		return QCLOUD_ERR_FAILURE;
	}
	client->recv_buffer = recvBuff;
#else
	ring_buff_init(&g_ring_buff, sg_Uart_Recv_Buff,  RING_BUFF_LEN);
	client->recv_buffer = sg_client_recv_buff;
#endif	

	client->pRingBuff = &g_ring_buff;
	client->recv_bufsz = CLINET_BUFF_LEN;	
	client->cur_recv_len = 0;

	
	client->resp = NULL;
	client->resp_notice = false;

	//client->urc_table = NULL;
	//client->urc_table_size = 0;

	client->parser = client_parser;


	return QCLOUD_RET_SUCCESS;
}


/**
 * AT client initialize.
 *
 * @param pClient pinter of at client which to be inited
 * @return @see eTidResault
 */
eAtResault at_client_init(at_client_t pClient)
{
	POINTER_SANITY_CHECK(pClient, QCLOUD_ERR_NULL); 	
	eAtResault result;
	
	result = at_client_para_init(pClient);
	if (result == QCLOUD_RET_SUCCESS)
	{
		Log_d("AT client(V%s) initialize success.", AT_FRAME_VERSION);
		//pClient->status = AT_STATUS_INITIALIZED;
		
#ifdef OS_USED	
		osThreadId threadId;
		//	Parser Func should run in a separate thread
		if(NULL != pClient->parser)
		{
			//pClient->parser(pClient, NULL);
			hal_thread_create(&threadId, PARSE_THREAD_STACK_SIZE, osPriorityNormal, pClient->parser, pClient);
		}	
#endif
	}
	else
	{
		Log_e("AT client(V%s) initialize failed(%d).", AT_FRAME_VERSION, result);
	}

	return result;
}


