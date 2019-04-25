/**
******************************************************************************
* @file    ringbuff.h 
* @author  yougaliu
* @version V1.0
* @date    2018-09-12
* @brief   This file contains function ring buffer operation
******************************************************************************
*/ 
#ifndef __RING_BUFF_H__
#define __RING_BUFF_H__

#include "stdbool.h"
#include "stdint.h"



#define RINGBUFF_OK            0    /* No error, everything OK. */
#define RINGBUFF_ERR          -1    /* Out of memory error.     */
#define RINGBUFF_EMPTY        -3    /* Timeout.                	    */
#define RINGBUFF_FULL         -4    /* Routing problem.          */
#define RINGBUFF_TOO_SHORT    -5 




typedef struct _ring_buff_
{
  uint32_t  size;
  uint32_t  readpoint;
  uint32_t  writepoint;
  char*  buffer;
  bool full;
} sRingbuff;

typedef sRingbuff*  ring_buff_t;

int ring_buff_init(sRingbuff* ring_buff, char* buff, uint32_t size );
int ring_buff_flush(sRingbuff* ring_buff);
int ring_buff_push_data(sRingbuff* ring_buff, uint8_t *pData, int len);
int ring_buff_pop_data(sRingbuff* ring_buff, uint8_t *pData, int len);
#endif // __ringbuff_h__


