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
 * vio.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: Om J Omer
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <string>

#include "kernel/vio_drv_lib.h"
#include "dma_regs.h"
#include "vio.h"
#include "vio_priv.h"
#include "vio_regs.h"
#include "vio_utils.h"

using namespace std;

VIO_Module::VIO_Module()
{
    //resource = VIO_Resource::get_instance();
}

VIO_Module::~VIO_Module()
{
    //VIO_Resource::delete_instance(); 
}

//#define NEON_MEMCPY 1
#if NEON_MEMCPY
#include <arm_neon.h>
#endif
inline void
move_data_2_hw(const void *src, void *dst, size_t size)
{
    assert((src != NULL) && (dst != NULL) && (size > 0));
    PRINTF("move_data_2_hw(): src:%p, dst:%p, size:0x%08x\n",
           src, dst, size);
    assert( ! (size & 0x3));
    assert( ! ((unsigned long)dst & 0xf));
#if NEON_MEMCPY
    uint8x16_t vec;
    uint8_t *ps = (uint8_t *) src; uint8_t *pd = (uint8_t *)dst;
    uint8x16_t tmp_vec; uint8_t *ptr = (uint8_t *)&tmp_vec;
    for(size_t i = 0; i < size/16; i++) {
        vec = vld1q_u8(ps);
        //vst1q_u8(pd, vec);
        
        for(int j = 0; j < 1; j++) {
            ((uint8x16_t*)pd)[j] = ((uint8x16_t*)ps)[j];
        }
        for(int j = 0; j < 1; j++) {
            ((uint8x16_t*)ptr)[j] = ((uint8x16_t*)ps)[j];
        }
        //vst1q_u8(ptr, vec);
        if((i == 0) || (i == 1)) {
            uint8_t *tmp = (uint8_t *)pd;
            printf("fpga ");
            for(int j = 0; j < 16; j++) {
                printf("%02x ", (uint32_t)tmp[j]);
            }
            printf("\n");
            
            tmp = (uint8_t *)ptr;
            printf("reg  ");
            for(int j = 0; j < 16; j++) {
                printf("%02x ", (uint32_t)tmp[j]);
            }
            printf("\n");
        }
        ps += 16; pd += 16;
    }
#else
    uint32_t *ps = (uint32_t *)src;
    uint32_t *pd = (uint32_t *)dst;
    for(size_t i = size/4; i--; ) {
        *pd++ = *ps++;
    }
    while((unsigned long)pd & 0xf) {
        *pd++ = 0;
    }
#endif
}


void
VIO_Module::wait_dma_wr()
{
    dma_2_hw_wait();
}


static void
move_data_2_cpu(void *src, void *dst, size_t size) 
{
    PRINTF("move_data_2_cpu(): src:%p, dst:%p, size:0x%08x\n",
           src, dst, size);
    assert((src != NULL) && (dst != NULL) && (size > 0));
#if 1
    memcpy(dst, src, size);
#else
    assert( ! (size & 0x3));
    uint32_t *ps = (uint32_t *)src;
    uint32_t *pd = (uint32_t *)dst;
    for(size_t i = size/4; i--; ) {       
        *pd++ = *ps++;
    }
#endif
}

bool
VIO_Module::test_sram()
{
    const int BLOCK_SIZE = (4 * 1024);
    char *buf = (char *)malloc(BLOCK_SIZE); assert(buf);
    srand(time(NULL));
    for(int i = 0; i < BLOCK_SIZE; i++) {
        buf[i] = (rand() & 0xff);
    }
    //AXI_Bus &sram = resource->get_bus(VIO_SRAM);
    bool success = true; uint32_t off;
    for(off = 0; off < vio_sram_size(); off += BLOCK_SIZE) {
        memcpy(vio_sram_ptr() + off, buf, BLOCK_SIZE);
        if(0 != memcmp(buf, vio_sram_ptr() + off, BLOCK_SIZE)) {
            success = false;
            break;
        }
    }
    printf("%s: SRAM test %s\n", success ? "I":"E", success ? "passed":"failed");
    if (!success) {
        printf("E: failed addr %p\n", (void *)(vio_sram_base() + off));
    }
    free(buf);
    return success;
}

void
VIO_Module::write_register(Bus_Type type, const char *str, uint32_t reg, uint32_t val)
{
    vio_write_register(type, str, reg, val);
}

uint32_t
VIO_Module::read_register(Bus_Type type, const char *str, uint32_t reg)
{
    return vio_read_register(type, str, reg);
}

uint32_t
VIO_Module::read_register(Bus_Type type, const char *str, uint32_t reg, uint32_t init)
{
    return vio_read_register(type, str, reg, init);
}

void
VIO_Module::check_dma_2_hw(const void *src, uint64_t dst_off, size_t size)
{
    uint64_t src_pa = vio_phys_addr(src, size);
    ASSERT(src_pa, "Check DMA supported for DMA surface only, src:%p", src);
    printf("-D- CHECK DMA, src:%p dst:%p \n", src, vio_sram_ptr() + dst_off);
    if(memcmp(src, vio_sram_ptr() + dst_off, size)) {
        size_t i, fail = 0; 
        volatile auto p_src = (uint32_t*)src;
        volatile auto p_dst = (uint32_t*)(vio_sram_ptr() + dst_off);
        for(i = 0; i < size / sizeof(p_dst[0]); i++) {
            if(*p_dst != *p_src) {
                fail++;
                PRINTF("-D- %3d: src val = %08x, dst val = %08x *** \n", i, *p_src, *p_dst); 
            } else {
                //PRINTF("-D- %3d: src val = %08x, dst val = %08x     \n", i, *p_src, *p_dst); 
            }
            p_dst++; p_src++;
        }
        PRINTF("E[DMA]: Failed cpu_2_hw data tx, src: 0x%llx, dst: 0x%llx, size: 0x%x, off: 0x%x, mismatches: %d/%d\n",
               src_pa, vio_sram_base() + dst_off, size, i, fail, size);
        exit(1);
    } else {
        fprintf(stderr, ".");
    }
}

#define CHECK_DMA 0
void
VIO_Module::move_dma_2_hw(const void *src, uint64_t dst_off, size_t size,
                          bool nb)
{
    if(src == NULL) {return;}
    
    uint64_t src_pa = 0;
    src_pa = vio_phys_addr(src, size);
    ASSERT(src_pa || !nb, "Non blocking supported for DMA surface only, src %p", src);
    
    if(src_pa) {
        size_t dma_size = ALIGN_16(size); //(size & (~0xf)); // Due VIO address decode 128-bit requirement
        if(dma_size) {
            uint32_t cookie;
            vio_map_buf(src, dma_size, true /*to_dev*/, true, cookie);
            dma_data_2_hw(src_pa, dst_off, dma_size, nb);
            vio_map_buf(src, dma_size, true /*to_dev*/, false, cookie);
        }
#if 0 // Disabled as current VIO address decode expect full 128-bit write
        if(size - dma_size) {
            memcpy(vio_sram_ptr() + dst_off + dma_size, p_src + dma_size , size - dma_size);
        }
#endif
#if CHECK_DMA
        check_dma_2_hw(src, dst_off, size);
#endif
    } else {
        move_data_2_hw(src, vio_sram_ptr() + dst_off, size);
    }
}

dma_hdl_t
VIO_Module::sgdma_2_hw(const void *src, uint64_t dst_off, size_t size,
                       bool first, bool last)
{
    if((src == NULL) || (size == 0)) {return 0;}
    
    uint64_t src_pa = vio_phys_addr(src, size);
    ASSERT(src_pa, "SGDMA currently supported for DMA surface only, src%p", src);
    size_t dma_size = ALIGN_16(size);
    dma_hdl_t hdl = sgdma_2_hw_add(src_pa, dst_off, dma_size, first, last);
    sgdma_2_hw_wait(hdl);
    check_dma_2_hw(src, dst_off, size);
    return hdl;
}

void
VIO_Module::move_dma_2_cpu(uint64_t src_off, void *dst, size_t size)
{
    if(dst == NULL) {
        return;
    }
    
    uint64_t dst_pa = 0;
#if 1 //ENABLE_DMA
    dst_pa = vio_phys_addr(dst, size);
#endif
    //AXI_Bus &sram = resource->get_bus(VIO_SRAM);
    if(dst_pa) {
        size_t dma_size = (size & (~0xf));
        if(dma_size) {
            uint32_t cookie;
            vio_map_buf(dst, dma_size, false, true, cookie);
            dma_data_2_cpu(src_off, dst_pa, dma_size);
            vio_map_buf(dst, dma_size, false, false, cookie);
        }
        char *p_dst = (char*)dst;
        if(size - dma_size) {
            memcpy(p_dst + dma_size, vio_sram_ptr() + src_off + dma_size, size - dma_size);
        }
#if CHECK_DMA
        printf("-D- CHECK DMA for src = 0x%llx, dst = 0x%llx (va: %p), size: 0x%lx \n",
               vio_sram_base() + src_off, dst_pa, dst, size);
        if(memcmp(vio_sram_ptr() + src_off, dst, size)) {
            size_t i;
            int fail = 0;
            uint8_t *p_src = vio_sram_ptr() + src_off;
            // p_dst += 16;
            for(i = 0; i < size; i++) {
                if(*p_dst != *p_src) {
                    PRINTF("-D- %3d: src val = %02x, dst val = %02x *** \n", i, *p_src, *p_dst);
                    fail ++;
                    // break;
                } else {
                    PRINTF("-D- %3d: src val = %02x, dst val = %02x \n", i, *p_src, *p_dst);
                }
                p_dst++; p_src ++;
            }
            if (fail) { printf("E[DMA]: Failed hw_2_cpu data tx, src: 0x%llx, dst: 0x%llx, size: 0x%lx, off: 0x%x, mismatches: %d/%d\n",
                               vio_sram_base() + src_off, dst_pa, size, i, fail, size); }
        }
        // exit(0);
#endif
    } else {
        move_data_2_cpu(vio_sram_ptr() + src_off, dst, size);
    }
}

void*
VIO_Module::get_sram_buf(size_t off)
{
    //AXI_Bus &sram = resource->get_bus(VIO_SRAM);
    return (vio_sram_ptr() + off);
}
