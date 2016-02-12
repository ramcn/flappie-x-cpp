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
 * vio.h
 *
 *  Created on: Sep 20, 2016
 *      Author: Om J Omer
 */
#pragma once
#include <memory>

#define EXPORT_API __attribute__ ((visibility("default")))
extern "C" {
    EXPORT_API void* vio_malloc(size_t size);
    EXPORT_API void  vio_free(void *p);
}

template <class T>
class Fast_Alloc {
public:
    typedef  T value_type;
    static T* allocate(std::size_t n) {
        void *ptr = vio_malloc(n * sizeof(T));
        //printf("allocate(): type: %s, n: %d, %p\n", typeid(T).name(), n, ptr);
        return (T*)ptr;
    }
    static void deallocate(T *p, std::size_t n) {
        //printf("deallocate(): type: %s, n: %d, %p\n", typeid(T).name(), n, p);
        vio_free(p);
    }
};

//class VIO_Resource;
enum Bus_Type {
    VIO_CONFIG,
    VIO_DMA,
    VIO_SRAM,
    VIO_DMA_MSG,
    VIO_MIPI_DMA,
    VIO_ISRC_CONFIG,
    VIO_NUM_BUS // Should be the last one
};

typedef uint16_t dma_hdl_t;

class VIO_Module {
public:    
    VIO_Module();
    virtual ~VIO_Module();
    void write_register(Bus_Type type, const char* str, uint32_t reg, uint32_t val);
    uint32_t read_register(Bus_Type type, const char*str, uint32_t reg);
    uint32_t read_register(Bus_Type type, const char*str, uint32_t reg, uint32_t init);
    void move_dma_2_cpu(uint64_t src_off, void *dst, size_t size);
    void move_dma_2_hw(const void *src, uint64_t dst_off, size_t size, bool nb = false);
    void wait_dma_wr();
    void *get_sram_buf(size_t off);
    bool test_sram();

protected:
    void check_dma_2_hw(const void *src, uint64_t dst_off, size_t size);
    dma_hdl_t sgdma_2_hw(const void *src, uint64_t dst_off, size_t size,
                          bool first, bool last);    
//private:
    //VIO_Resource *resource;
};
#define ALIGN_4(x)   (((x) + 0x3) & (~0x3))

EXPORT_API void  vio_write_register(Bus_Type bus_type, const char* str, uint32_t reg, uint32_t val);
EXPORT_API uint32_t vio_read_register(Bus_Type type, const char*str, uint32_t reg);
EXPORT_API uint32_t vio_read_register(Bus_Type type, const char*str, uint32_t reg, uint32_t init);
EXPORT_API void dma_data_2_hw(uint64_t src_pa, uint64_t dst_pa, size_t size, bool nb);
EXPORT_API void dma_data_2_cpu(uint64_t src_pa, uint64_t dst_pa, size_t size);
EXPORT_API dma_hdl_t sgdma_2_hw_add(uint64_t src_pa, uint64_t dst_pa, size_t size, bool first, bool last);
EXPORT_API void sgdma_2_hw_wait(dma_hdl_t &hdl);
EXPORT_API void dma_2_hw_wait();
EXPORT_API uint64_t vio_phys_addr(const void *ptr, size_t size);
EXPORT_API uint64_t vio_sram_size();
EXPORT_API uint8_t* vio_sram_ptr();
EXPORT_API uint64_t vio_sram_base();
EXPORT_API void vio_map_buf(const void *ptr, size_t size, bool to_dev, bool map, uint32_t &cookie);
EXPORT_API void vio_dma_image(const void *src, size_t size);
EXPORT_API void mipi_dma_2_hw(const void *src, uint64_t dst_pa, size_t size, bool nb);
EXPORT_API void wait_mipi_dma();
EXPORT_API void vio_mipi_dma_image(const void *src, size_t size);

#define _WRITE_REG(cfg, str, reg, val)  vio_write_register(cfg, str, reg, val)
#define _READ_REG(cfg, str, reg)        vio_read_register(cfg, str, reg)
#define _POLL_N_WAIT(cfg, str, reg, v, c)          \
    do {                                           \
        v = vio_read_register(cfg, str, reg, v);   \
    } while (c);
    

#define WRITE_REG(reg, val)   _WRITE_REG(VIO_CONFIG, #reg, VIO_##reg##_OFFSET, val)
#define READ_REG(reg)         _READ_REG(VIO_CONFIG, #reg, VIO_##reg##_OFFSET)
#define POLL_N_WAIT(r, v, c)  _POLL_N_WAIT(VIO_CONFIG, #r, VIO_##r##_OFFSET, v, c)

#define ISRC_WRITE_REG(reg, val)   _WRITE_REG(VIO_ISRC_CONFIG, #reg, VIO_##reg##_OFFSET, val)
#define ISRC_READ_REG(reg)	       _READ_REG(VIO_ISRC_CONFIG, #reg, VIO_##reg##_OFFSET)
#define ISRC_POLL_N_WAIT(r, v, c)  _POLL_N_WAIT(VIO_ISRC_CONFIG, #r, VIO_##r##_OFFSET, v, c)

#define DMA_WRITE_REG(reg, val)   _WRITE_REG(  VIO_DMA, "DMA_"#reg, DMA_##reg##_OFFSET, val)
#define DMA_READ_REG(reg)	      _READ_REG(   VIO_DMA, "DMA_"#reg, DMA_##reg##_OFFSET)
#define DMA_POLL_N_WAIT(r, v, c)  _POLL_N_WAIT(VIO_DMA, "DMA_"#r,   DMA_##r##_OFFSET, v, c)

#define RD_DMA_WRITE_REG(reg, val)   _WRITE_REG(  VIO_DMA, "DMA_RD_"#reg, DMA_CONFIG_SIZE + DMA_##reg##_OFFSET, val)
#define RD_DMA_READ_REG(reg)	     _READ_REG(   VIO_DMA, "DMA_RD_"#reg, DMA_CONFIG_SIZE + DMA_##reg##_OFFSET)
#define RD_DMA_POLL_N_WAIT(r, v, c)  _POLL_N_WAIT(VIO_DMA, "DMA_RD_"#r,   DMA_CONFIG_SIZE + DMA_##r##_OFFSET, v, c)

#define DMA_MSG_WRITE_REG(reg, val)   _WRITE_REG(  VIO_DMA_MSG, "DMA_MSG_"#reg, DMA_MSG_##reg##_OFFSET, val)
#define DMA_MSG_READ_REG(reg)	      _READ_REG(   VIO_DMA_MSG, "DMA_MSG_"#reg, DMA_MSG_##reg##_OFFSET)
#define DMA_MSG_POLL_N_WAIT(r, v, c)  _POLL_N_WAIT(VIO_DMA_MSG, "DMA_MSG_"#r,   DMA_MSG_##r##_OFFSET, v, c)

#define DMA_DESC_WRITE_REG(reg, val)   _WRITE_REG(  VIO_DMA_MSG, "DESC_"#reg, DMA_DESC_##reg##_OFFSET, val)
#define DMA_DESC_READ_REG(reg)	       _READ_REG(   VIO_DMA_MSG, "DESC_"#reg, DMA_DESC_##reg##_OFFSET)
#define DMA_DESC_POLL_N_WAIT(r, v, c)  _POLL_N_WAIT(VIO_DMA_MSG, "DESC_"#r,   DMA_DESC_##r##_OFFSET, v, c)

#define MIPI_DMA_WRITE_REG(reg, val)   _WRITE_REG(  VIO_MIPI_DMA, "MIPI_DMA_"#reg, DMA_##reg##_OFFSET, val)
#define MIPI_DMA_READ_REG(reg)	       _READ_REG(   VIO_MIPI_DMA, "MIPI_DMA_"#reg, DMA_##reg##_OFFSET)
#define MIPI_DMA_POLL_N_WAIT(r, v, c)  _POLL_N_WAIT(VIO_MIPI_DMA, "MIPI_DMA_"#r,   DMA_##r##_OFFSET, v, c)

