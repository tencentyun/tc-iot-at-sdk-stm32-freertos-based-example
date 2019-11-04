/**
 ******************************************************************************
 * @file           : at_client.h
 * @brief          : at client header file
 ******************************************************************************
 *
 * Copyright (c) 2019 Tencent. 
 * All rights reserved.
 ******************************************************************************
 */
  
#ifndef __AT_CLIENT_H__
#define __AT_CLIENT_H__

#include "ringbuff.h"
#include "module_api_inf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AT_FRAME_VERSION               "1.0.0"

#define AT_CMD_NAME_LEN                16
#define AT_END_MARK_LEN                4


//#define CLINET_BUFF_LEN				   (4*RING_BUFF_LEN)MAX_TOPIC_PAYLOAD_LEN
#define CLINET_BUFF_LEN				   (MAX_TOPIC_PAYLOAD_LEN)
#define GET_CHAR_TIMEOUT_MS			   (5000)
#define CMD_TIMEOUT_MS			   	   (10000)
#define CMD_RESPONSE_INTERVAL_MS 	   (100)


typedef void (*ParserFunc)(void *userContex);

typedef enum 
{
    AT_STATUS_UNINITIALIZED = 0,
    AT_STATUS_INITIALIZED,
    AT_STATUS_BUSY,
}at_status;


enum at_resp_status
{
     AT_RESP_OK = 0,                   /* AT response end is OK */
     AT_RESP_ERROR = -1,               /* AT response end is ERROR */
     AT_RESP_TIMEOUT = -2,             /* AT response is timeout */
     AT_RESP_BUFF_FULL= -3,            /* AT response buffer is full */
};
typedef enum at_resp_status at_resp_status_t;

typedef struct _at_response_
{
    /* response buffer */
    char *buf;
    /* the maximum response buffer size */
    int buf_size;
    /* the number of setting response lines
     * == 0: the response data will auto return when received 'OK' or 'ERROR'
     * != 0: the response data will return when received setting lines number data */
    int line_num;
    /* the count of received response lines */
    int line_counts;
    /* the maximum response time */
    uint32_t timeout;
}at_response;


typedef  at_response * at_response_t;

/* URC(Unsolicited Result Code) object, such as: 'RING', 'READY' request by AT server */
typedef struct _at_urc_
{
    const char *cmd_prefix;
    const char *cmd_suffix;
    void (*func)(const char *data, uint32_t size);
}at_urc;

typedef at_urc *at_urc_t;

typedef struct _at_client_
{
    at_status status;
    char end_sign;
	
	ring_buff_t pRingBuff;

    char *recv_buffer;
    uint32_t recv_bufsz;
    uint32_t cur_recv_len;
	void *lock;      //pre cmd take the lock wait for resp , another cmd need wait for unlock
	
    at_response_t resp;
    at_resp_status_t resp_status;
	bool resp_notice;

    const at_urc *urc_table;
    uint16_t urc_table_size;

    ParserFunc parser; //RX parser
}at_client;

typedef at_client *at_client_t;


/* AT client initialize and start*/
eAtResault at_client_init(at_client_t pClient);

/* get AT client handle*/
at_client_t at_client_get(void);

/*AT connect detect*/
eAtResault at_client_wait_connect(uint32_t timeout);

/* ========================== multiple AT client function ============================ */
/* set AT client a line end sign */
void at_set_end_sign(char ch);

/* Set URC(Unsolicited Result Code) table */
void at_set_urc_table(at_client_t client, const at_urc_t table, uint32_t size);

/* AT client send commands to AT server and waiter response */
int at_client_send(at_client_t client, char *buf, int size);

eAtResault at_obj_exec_cmd(at_response_t resp, const char *cmd_expr, ...);
#define at_exec_cmd(resp, ...)                   at_obj_exec_cmd(resp, __VA_ARGS__)

/* AT response object create and delete */
at_response_t at_create_resp(uint32_t buf_size, uint32_t line_num, uint32_t timeout);
void at_delete_resp(at_response_t resp);


/* AT response line buffer get and parse response buffer arguments */
const char *at_resp_get_line(at_response_t resp, uint32_t resp_line);
const char *at_resp_get_line_by_kw(at_response_t resp, const char *keyword);
int at_resp_parse_line_args(at_response_t resp, uint32_t resp_line, const char *resp_expr, ...);
int at_resp_parse_line_args_by_kw(at_response_t resp, const char *keyword, const char *resp_expr, ...);

/* ========================== single AT client function ============================ */
#ifdef __cplusplus
}
#endif

#endif /* __AT_H__ */

