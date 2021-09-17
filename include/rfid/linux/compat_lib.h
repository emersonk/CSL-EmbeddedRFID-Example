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
 * Description: The core of the cross-platform library.  Will be included
 *              by all other headers in the library.
 ****************************************************************************/

#ifndef COMPAT_LIB_
#define COMPAT_LIB_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <wchar.h>

#include "rfid_platform_types.h"



/****************************************************************************
 *  Function:    CPL_HostToMac16
 *  Description: Converts a 16-bit value from host endianness to the
 *               little-endian format used by the MAC.
 *  Parameters:  hostWord   [IN] The value to convert.
 *  Returns:     The converted value.
 ****************************************************************************/
INT16U CPL_HostToMac16(INT16U hostWord);

/****************************************************************************
 *  Function:    CPL_HostToMac32
 *  Description: Converts a 32-bit value from host endianness to the
 *               little-endian format used by the MAC.
 *  Parameters:  hostDword  [IN] The value to convert.
 *  Returns:     The converted value.
 ****************************************************************************/
INT32U CPL_HostToMac32(INT32U hostDword);

/****************************************************************************
 *  Function:    CPL_HostToMac64
 *  Description: Converts a 64-bit value from host endianness to the
 *               little-endian format used by the MAC.
 *  Parameters:  hostQword  [IN] The value to convert.
 *  Returns:     The converted value.
 ****************************************************************************/
INT64U CPL_HostToMac64(INT64U hostQword);

/****************************************************************************
 *  Function:    CPL_MacToHost16
 *  Description: Converts a 16-bit value from the little-endian format used
 *               by the MAC to the host's endianness.
 *  Parameters:  macWord   [IN] The value to convert.
 *  Returns:     The converted value.
 ****************************************************************************/
INT16U CPL_MacToHost16(INT16U macWord);

/****************************************************************************
 *  Function:    CPL_MacToHost32
 *  Description: Converts a 32-bit value from the little-endian format used
 *               by the MAC to the host's endianness.
 *  Parameters:  macDword  [IN] The value to convert.
 *  Returns:     The converted value.
 ****************************************************************************/
INT32U CPL_MacToHost32(INT32U macDword);

/****************************************************************************
 *  Function:    CPL_MacToHost64
 *  Description: Converts a 64-bit value from the little-endian format used
 *               by the MAC to the host's endianness.
 *  Parameters:  macQword  [IN] The value to convert.
 *  Returns:     The converted value.
 ****************************************************************************/
INT64U CPL_MacToHost64(INT64U macQword);

static inline int CPL_StrCaseCmp(const char *s1, const char *s2) {
    return strcasecmp(s1,s2);
}
static inline int CPL_StrNCaseCmp(const char *s1, const char *s2, INT32U n) {
    return strncasecmp(s1,s2,n);
}
#ifdef _GNU_SOURCE
static inline int CPL_WideStrCaseCmp(const wchar_t *s1, const wchar_t *s2) {
    return wcscasecmp(s1,s2);
}
static inline int CPL_WideStrNCaseCmp(const wchar_t *s1, const wchar_t *s2,
        INT32U n){
    return wcsncasecmp(s1,s2,n);
}
#endif /* _GNU_SOURCE */
static inline char *CPL_StrToken(char *s, char *delim, char **ptrptr) {
    return strtok_r(s, delim, ptrptr);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPAT_LIB_ */
