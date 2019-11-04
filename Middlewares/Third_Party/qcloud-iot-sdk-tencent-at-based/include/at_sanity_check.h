#ifndef _AT_SANITY_CHECK_H_
#define _AT_SANITY_CHECK_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "at_log.h"


#define NUMBERIC_SANITY_CHECK(num, err) \
    do { \
        if (0 == (num)) { \
            Log_e("Invalid argument, numeric 0"); \
            return (err); \
        } \
    } while(0)

#define NUMBERIC_SANITY_CHECK_RTN(num) \
    do { \
        if (0 == (num)) { \
            Log_e("Invalid argument, numeric 0"); \
            return; \
        } \
    } while(0)

#define POINTER_SANITY_CHECK(ptr, err) \
    do { \
        if (NULL == (ptr)) { \
            Log_e("Invalid argument, %s = %p", #ptr, ptr); \
            return (err); \
        } \
    } while(0)

#define POINTER_SANITY_CHECK_RTN(ptr) \
    do { \
        if (NULL == (ptr)) { \
            Log_e("Invalid argument, %s = %p", #ptr, ptr); \
            return; \
        } \
    } while(0)

#define STRING_PTR_SANITY_CHECK(ptr, err) \
    do { \
        if (NULL == (ptr)) { \
            Log_e("Invalid argument, %s = %p", #ptr, (ptr)); \
            return (err); \
        } \
        if (0 == strlen((ptr))) { \
            Log_e("Invalid argument, %s = '%s'", #ptr, (ptr)); \
            return (err); \
        } \
    } while(0)

#define STRING_PTR_SANITY_CHECK_RTN(ptr) \
    do { \
        if (NULL == (ptr)) { \
            Log_e("Invalid argument, %s = %p", #ptr, (ptr)); \
            return; \
        } \
        if (0 == strlen((ptr))) { \
            Log_e("Invalid argument, %s = '%s'", #ptr, (ptr)); \
            return; \
        } \
    } while(0)

#if defined(__cplusplus)
}
#endif

#endif 
