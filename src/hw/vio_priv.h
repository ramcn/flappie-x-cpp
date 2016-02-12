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
 * vio_priv.cpp
 *
 *  Created on: Sep 20, 2016
 *      Author: Om J Omer
 */

#pragma once
#include <assert.h>
#include <string>
#include "vio.h"
using namespace std;

struct DMA_t;
class  VIO_Mem;

class AXI_Bus {
public:
    AXI_Bus();
    ~AXI_Bus();
    void configure(off_t offset, size_t _size, string _name);
    uint8_t *ptr;
    size_t base;
    size_t size() {return m_size;};

private:
    int fd;
    size_t m_size;
    string name;
};

enum DMA_Type{
    RD_DMA,
    WR_DMA,
    MIPI_DMA,
    NUM_DMA
};

#define LOW_32(x)  ((x) & 0xffffffff)
#define HIGH_32(x) (((x) >> 32) & 0xffffffff)

class VIO_Resource {
public:
    static VIO_Resource* get_instance() {
        assert(m_instance);
        return m_instance;
    }
    static void delete_instance() {};
    VIO_Resource();
    ~VIO_Resource();
    AXI_Bus &get_bus(Bus_Type type) {
        assert(type < VIO_NUM_BUS);
        return bus[type];
    }
    VIO_Mem *get_mem() {return vio_mem;};
    DMA_t &get_dma(DMA_Type type) {assert(type < NUM_DMA); return m_dma[type];}
    void write_register(Bus_Type type, const char* str, uint32_t reg, uint32_t val);
    uint32_t read_register(Bus_Type type, const char*str, uint32_t reg);
    uint32_t read_register(Bus_Type type, const char*str, uint32_t reg, uint32_t init); 
    void dma_data_2_cpu(uint64_t src_pa, uint64_t dst_pa, size_t size);
    void dma_data_2_hw(uint64_t src_pa, uint64_t dst_pa, size_t size, bool nb);
    void mipi_dma_2_hw(uint64_t src_pa, uint64_t dst_pa, size_t size, bool nb);
    dma_hdl_t sgdma_2_hw_add(uint64_t src_pa, uint64_t dst_pa, size_t size, bool first, bool last);
    void sgdma_2_hw_wait(dma_hdl_t &hdl);
    void dma_2_hw_wait();
    void mipi_dma_2_hw_wait();

private:
    static VIO_Resource *m_instance;
    AXI_Bus      bus[VIO_NUM_BUS];

    // Common Module
    
    DMA_t        m_dma[NUM_DMA];
    DMA_MSG_t    dma_msg;
    DMA_EXT_DESC_t dma_desc;
    VIO_Mem      *vio_mem;
    dma_hdl_t    m_unique_seq;
    
private:
    void dma_init();
    void rd_dma_init();
    void mipi_dma_init();
};
