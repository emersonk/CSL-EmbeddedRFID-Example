/*******************************************************************************
 *  INTEL CONFIDENTIAL
 *  Copyright 2006 Intel Corporation All Rights Reserved.
 *
 *  The source code contained or described herein and all documents related to
 *  the source code ("Material") are owned by Intel Corporation or its suppliers
 *  or licensors. Title to the Material remains with Intel Corporation or its
 *  suppliers and licensors. The Material may contain trade secrets and
 *  proprietary and confidential information of Intel Corporation and its
 *  suppliers and licensors, and is protected by worldwide copyright and trade
 *  secret laws and treaty provisions. No part of the Material may be used,
 *  copied, reproduced, modified, published, uploaded, posted, transmitted,
 *  distributed, or disclosed in any way without Intel's prior express written
 *  permission. 
 *  
 *  No license under any patent, copyright, trade secret or other intellectual
 *  property right is granted to or conferred upon you by disclosure or delivery
 *  of the Materials, either expressly, by implication, inducement, estoppel or
 *  otherwise. Any license under such intellectual property rights must be
 *  express and approved by Intel in writing.
 *
 *  Unless otherwise agreed by Intel in writing, you may not remove or alter
 *  this notice or any other notice embedded in Materials by Intel or Intel's
 *  suppliers or licensors in any way.
 ******************************************************************************/

/******************************************************************************
 *
 * Description:
 *   This header file defines the cross-platform types in terms of types for
 *   the Linux compilers
 *
 ******************************************************************************/

#ifndef RFID_PLATFORM_TYPES_H_INCLUDED
#define RFID_PLATFORM_TYPES_H_INCLUDED

#include <stdint.h>

typedef int8_t      INT8S;
typedef uint8_t     INT8U;
typedef int16_t     INT16S;
typedef uint16_t    INT16U;
typedef int32_t     INT32S;
typedef uint32_t    INT32U;
typedef int64_t     INT64S;
typedef uint64_t    INT64U;
typedef int32_t     BOOL32;
typedef uint32_t    HANDLE32;
typedef uint64_t    HANDLE64;

/* Specify the calling convention for function callbacks */
#define RFID_CALLBACK

/* Macro to use to resolve warnings for unreferenced variables */
#define RFID_UNREFERENCED_LOCAL(v)

#endif  /* #ifndef RFID_PLATFORM_TYPES_H_INCLUDED */
