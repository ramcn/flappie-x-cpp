/**********************************************************************************************
   INTEL TOP SECRET
   Copyright 2016 Intel Corporation All Rights Reserved.

   The source code contained or described herein and all documents related to the source code
   ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the
   Material remains with Intel Corporation or its suppliers and licensors. The Material may
   contain trade secrets and proprietary and confidential information of Intel Corporation
   and its suppliers and licensors, and is protected by worldwide copyright and trade secret
   laws and treaty provisions. No part of the Material may be used, copied, reproduced,
   modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way
   without Intel's prior express written permission.
   No license under any patent, copyright, trade secret or other intellectual property right
   is granted to or conferred upon you by disclosure or delivery of the Materials, either
   expressly, by implication, inducement, estoppel or otherwise. Any license under such
   intellectual property rights must be express and approved by Intel in writing.
**********************************************************************************************/

/*
 * VIO HW Accelerators
 * vio_utils.h
 *
 *  Created on: Sep 20, 2016
 *      Author: Om J Omer
 */
#pragma once

#include <sys/time.h>

#define PROFILE 0
#if PROFILE
#define PRINTF 
#else
#define PRINTF  printf
#endif

#define ASSERT(cond, ...)  \
    if(!(cond)) {        \
        printf("Assert: %s %d : ", __FILE__, __LINE__);    \
        printf(__VA_ARGS__); printf("\n");                 \
        exit(1); \
    }

#define WARN(cond, ...)  \
    if((cond)) {        \
        printf("Warn: %s %d : ", __FILE__, __LINE__);    \
        printf(__VA_ARGS__); printf("\n");               \
    }

#if PROFILE
#define PROFILE_START(x) \
    struct timeval pr_##x##_start, pr_##x##_end; \
    gettimeofday(&pr_##x##_start, NULL); 

#define PROFILE_END(x) \
    gettimeofday(&pr_##x##_end, NULL); 
#define PROFILE_REPORT(x)  \
    printf("%-24s took %5lld us\n", #x, (int64_t)(pr_##x##_end.tv_usec - pr_##x##_start.tv_usec) \
           + ((int64_t)(pr_##x##_end.tv_sec - pr_##x##_start.tv_sec) * 1000 * 1000))
#else
#define PROFILE_START(x)
#define PROFILE_END(x)
#define PROFILE_REPORT(x)
#endif

#if ARM
#include <asm/cachectl.h>
#define CACHE_FLUSH(addr, size, type) assert(cacheflush(addr, size, type) == 0)
#else
#define CACHE_FLUSH(addr, size, type)
#endif

#define ALIGN_16(x)  (((x) + 0xf) & (~0xf))

using namespace std;
