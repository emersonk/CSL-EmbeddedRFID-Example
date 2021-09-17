/****************************************************************************
 *  INTEL CONFIDENTIAL
 *  Copyright 2006 Intel Corporation All Rights Reserved.
 *
 *  The source code contained or described herein and all documents related
 *  to the source code ("Material") are owned by Intel Corporation or its
 *  suppliers or licensors. Title to the Material remains with Intel
 *  Corporation or its suppliers and licensors. The Material may contain
 *  trade secrets and proprietary and confidential information of Intel
 *  Corporation and its suppliers and licensors, and is protected by
 *  worldwide copyright and trade secret laws and treaty provisions. No part
 *  of the Material may be used, copied, reproduced, modified, published,
 *  uploaded, posted, transmitted, distributed, or disclosed in any way
 *  without Intel's prior express written permission.
 *
 *  No license under any patent, copyright, trade secret or other
 *  intellectual property right is granted to or conferred upon you by
 *  disclosure or delivery of the Materials, either expressly, by
 *  implication, inducement, estoppel or otherwise. Any license under such
 *  intellectual property rights must be express and approved by Intel in
 *  writing.
 *
 *  Unless otherwise agreed by Intel in writing, you may not remove or alter
 *  this notice or any other notice embedded in Materials by Intel or Intel's
 *  suppliers or licensors in any way.
 ****************************************************************************/

/****************************************************************************
 * Description: Defines the error-handling functions and the portable error
 *              codes.
 ****************************************************************************/

#ifndef COMPAT_ERROR_
#define COMPAT_ERROR_

#include <errno.h>
#include <netdb.h>

#include "compat_lib.h"

enum {
    CPL_SUCCESS = 0,
    CPL_ERROR_ACCESSDENIED = EACCES,
    CPL_ERROR_BUFFERTOOSMALL = EOVERFLOW,
    CPL_ERROR_BUSY = EBUSY,
    CPL_ERROR_DEVICEFAILURE = 0xDE7FA17D,
    CPL_ERROR_DEVICEGONE = ENODEV,
    CPL_ERROR_EXISTS = EEXIST,
    CPL_ERROR_INVALID = EINVAL,
    CPL_ERROR_NOMEMORY = ENOMEM,
    CPL_ERROR_NOTFOUND = ENOENT,
    CPL_ERROR_INVALID_FUNC = ENXIO,
    CPL_ERROR_TOOMUCHDATA = ENOBUFS,
    CPL_ERROR_RXOVERFLOW  = 0xDA7A2817,
    CPL_WARN_CANCELLED = 0xCA2CE1D,
    CPL_WARN_ENDOFLIST = 0x32D0F17,
    CPL_WARN_NEWDEVICE = 0x15EE02E,
    CPL_WARN_TIMEOUT = ETIMEDOUT,
    CPL_WARN_WOULDBLOCK = EAGAIN,

    // network & socket specific errors

    CPL_ENET_INTERRUPT	        = EINTR,
    CPL_ENET_BAD_FILE	        = EBADF,
    CPL_ENET_ACCESS	            = EACCES,
    CPL_ENET_FAULT	            = EFAULT,
    CPL_ENET_INVAL	            = EINVAL,
    CPL_ENET_TOO_MANY_FILES	    = EMFILE,
    CPL_ENET_WOULD_BLOCK	    = EWOULDBLOCK,
    CPL_ENET_IN_PROGRESS	    = EINPROGRESS,
    CPL_ENET_ALREADY	        = EALREADY,
    CPL_ENET_NOT_SOCK	        = ENOTSOCK,
    CPL_ENET_DEST_ADDR_REQUIRED	= EDESTADDRREQ,
    CPL_ENET_MSG_SIZE	        = EMSGSIZE,
    CPL_ENET_PROTO	            = EPROTO,
    CPL_ENET_PROTO_OPTION	    = ENOPROTOOPT,
    CPL_ENET_PROTO_NOSUPPORT	= EPROTONOSUPPORT,
    CPL_ENET_SOCK_NOSUPPORT     = EOPNOTSUPP,        // ??
    CPL_ENET_OP_NOSUPPORT	    = EOPNOTSUPP,        
    CPL_ENET_PF_NOSUPPORT	    = EPROTONOSUPPORT,
    CPL_ENET_AF_NOSUPPORT	    = EAFNOSUPPORT,
    CPL_ENET_ADDR_INUSE	        = EADDRINUSE,
    CPL_ENET_ADDR_NOTAVAILABLE	= EADDRNOTAVAIL,
    CPL_ENET_DOWN	            = ENETDOWN,
    CPL_ENET_NET_UNREACHABLE	= ENETUNREACH,
    CPL_ENET_NET_RESET	        = ENETRESET,
    CPL_ENET_CONN_ABORTED	    = ECONNABORTED,
    CPL_ENET_NO_BUFFS	        = ENOBUFS,
    CPL_ENET_IS_CONNECTED	    = EISCONN,
    CPL_ENET_NOT_CONNECTED	    = ENOTCONN,
    CPL_ENET_SHUTDOWN	        = ENOTCONN,         // ??
    CPL_ENET_TOO_MANY_REFS	    = 0XE1E100,         // make up val for now...??
    CPL_ENET_TIMED_OUT	        = ETIMEDOUT,
    CPL_ENET_CONN_REFUSED	    = ECONNREFUSED,
    CPL_ENET_LOOP	            = ELOOP,
    CPL_ENET_NAME_TOO_LONG	    = ENAMETOOLONG,
    CPL_ENET_HOST_DOWN	        = EHOSTUNREACH,     // ?? should be same as unreach
    CPL_ENET_HOST_UNREACHABLE	= EHOSTUNREACH,
    CPL_ENET_HOST_NOT_FOUND	    = HOST_NOT_FOUND,
    CPL_ENET_HOST_TRY_AGAIN	    = TRY_AGAIN,
    CPL_ENET_HOST_NO_RECOVERY	= NO_RECOVERY,
    CPL_ENET_HOST_NO_DATA	    = TRY_AGAIN
    

};

/****************************************************************************
 *  Function:    CPL_GetError
 *  Description: Returns the most recent system-level error for this thread.
 *  Parameters:  None.
 *  Returns:     The most recent error code.
 ****************************************************************************/
static inline INT32U CPL_GetError(void) { return errno; }

/****************************************************************************
 *  Function:    CPL_SetError
 *  Description: Sets the system-level error code for the current thread.
 *  Parameters:  errCode   [IN] The value to set the error to.
 *  Returns:     Its argument, for convenience.
 ****************************************************************************/
static inline INT32U CPL_SetError(INT32U err) { return errno = err; }

#endif /* COMPAT_ERROR_ */
