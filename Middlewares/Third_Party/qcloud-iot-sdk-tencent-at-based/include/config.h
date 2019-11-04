#ifndef _QCLOUD_CONFIG_H_
#define _QCLOUD_CONFIG_H_

#define AUTH_MODE_KEY

#define EVENT_POST_ENABLED
#define ACTION_ENABLED
#define DEBUG_DEV_INFO_USED

#define OS_USED

//#define MODULE_TYPE_ESP8266
//#define MODULE_TYPE_N21
#define MODULE_TYPE_L206D


#define AT_CMD_MAX_LEN                 1024
#define RING_BUFF_LEN         		   AT_CMD_MAX_LEN	 //uart ring buffer len

#define MAX_PAYLOAD_LEN_PUB			   200				//AT+TCMQTTPUB max payload len£¬AT+TCMQTTPUBL should use if payload len bigger than MAX_PAYLOAD_LEN_PUB

//#define QUOTES_TRANSFER_NEED			1				// " need transfer to \" or not, most module needed excepted L206D
//#define COMMA_TRANSFER_NEED				1				// , need transfer to \, or not, most module not needed except ESP8266

#ifdef  COMMA_TRANSFER_NEED	 
#define T_	"\\" 							
#else
#define T_	
#endif


/* #undef AUTH_MODE_CERT */
/* #undef AUTH_WITH_NOTLS */
/* #undef SYSTEM_COMM */

/* #undef DEV_DYN_REG_ENABLED */
/* #undef LOG_UPLOAD */
/* #undef IOT_DEBUG */
/* #undef DEBUG_DEV_INFO_USED */
/* #undef AT_TCP_ENABLED */
/* #undef AT_UART_RECV_IRQ */
/* #undef AT_OS_USED */
/* #undef AT_DEBUG */
#endif
