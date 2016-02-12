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

VIO_Mem* VIO_Mem::m_instance = NULL;
VIO_Resource* VIO_Resource::m_instance = NULL;
VIO_Resource g_vio_resource;
enum {
    VIO_CONFIG_BASE     = 0xC0000000,
    VIO_SRAM_BASE       = 0xC0400000,
    DMA_CONFIG_BASE     = 0xC0008000, // RD_DMA_CONFIG_BASE  = 0xC0008020,
    DMA_MSG_CONFIG_BASE = 0xC0009000,
    MIPI_DMA_CONFIG_BASE= 0xC0840000,
    ISRC_CONFIG_BASE    = 0xC1000000,
};


//extern "C" {
void *vio_malloc(size_t size) {
    //fprintf(stderr, "[vio_malloc] size: 0x%08x\n", size);
    VIO_Mem* mem = g_vio_resource.get_mem();
    return mem->get_mem(size);
}
void vio_free(void *ptr)
{
    VIO_Mem* mem = g_vio_resource.get_mem();
    mem->free_mem(ptr);
}
//}

AXI_Bus::AXI_Bus()
    : ptr(NULL),
      fd(-1),
      m_size(0),
      base(0)
{}

void
AXI_Bus::configure(off_t offset, size_t _size, string _name)
{
    m_size = _size;
    name = _name;
    base = offset;
    
    int fd = open("/dev/mem", O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "open(/dev/mem) failed: %s\n", strerror(errno));
        exit(1);
    }
    ptr = (uint8_t *)mmap(NULL, m_size,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_LOCKED,
                       fd, offset);
    if(ptr == NULL) {
        fprintf(stderr, "mmap(0x%lx) failed: %s\n", offset, strerror(errno));
        exit(1);
    }
    fprintf(stdout, "%s mapped to 0x%08lx (CPU:%p) size 0x%08x\n",
            name.c_str(), offset,  ptr, m_size);
}
AXI_Bus::~AXI_Bus()
{
    if(ptr != NULL) {
        fprintf(stdout, "munmap(%p) Bus(%s)\n", ptr, name.c_str());
        munmap(ptr, m_size);
    }
    if(fd != -1) {
        //fprintf(stdout, "close(%d) Bus(%s)\n", fd, name.c_str());
        close(fd);
    }
}



VIO_Resource::VIO_Resource()
    : m_unique_seq(0)
{
    bus[VIO_CONFIG  ].configure(VIO_CONFIG_BASE, VIO_CONFIG_SIZE, "cfg");
    bus[VIO_SRAM    ].configure(VIO_SRAM_BASE,   VIO_SRAM_SIZE,   "sram");
    bus[VIO_DMA     ].configure(DMA_CONFIG_BASE, DMA_CONFIG_SIZE*2, "dma_cfg");
    bus[VIO_DMA_MSG ].configure(DMA_MSG_CONFIG_BASE, DMA_MSG_CONFIG_SIZE*2, "dma_msg_cfg");
    bus[VIO_MIPI_DMA].configure(MIPI_DMA_CONFIG_BASE, DMA_CONFIG_SIZE, "mipi_dma_cfg");
    bus[VIO_ISRC_CONFIG].configure(ISRC_CONFIG_BASE, ISRC_CONFIG_SIZE, "isrc_cfg");

    for(uint32_t i = 0; i < NUM_DMA; i++) {
        memset(&m_dma[i], 0x0, sizeof(DMA_t));
    }
    vio_mem = VIO_Mem::get_instance();

    // initialization
    dma_init(); 
    rd_dma_init();
    mipi_dma_init();
    m_instance = this;
}

VIO_Resource::~VIO_Resource()
{
    VIO_Mem::delete_instance();
}

void
VIO_Resource::dma_init()
{

    // DMA Reset
    DMA_CONTROL_T &dma_control = get_dma(WR_DMA).DMA_CONTROL;
    dma_control.Byte = 0;
    dma_control.HW = 0;
    dma_control.Word = 0;
    dma_control.Go = 0;
    dma_control.I_En = 0;
    dma_control.ReEn = 0;
    dma_control.WrEn = 0;
    dma_control.LenEn = 0;
    dma_control.RCon = 0;
    dma_control.WCon = 0;
    dma_control.WCon = 0;
    dma_control.DoubleWord = 0;
    dma_control.QuadWord   = 0;
    dma_control.SoftwareReset = 1;
    DMA_WRITE_REG(CONTROL, dma_control.val);
    DMA_WRITE_REG(CONTROL, dma_control.val);
    dma_control.SoftwareReset = 0;
    DMA_WRITE_REG(CONTROL, dma_control.val);

    // DMA read address
    DMA_READ_ADDRESS_T &dma_rdaddr = get_dma(WR_DMA).DMA_READ_ADDRESS;
    dma_rdaddr.Address = 0;
    DMA_WRITE_REG(READ_ADDRESS, dma_rdaddr.val);

    // DMA write address
    DMA_WRITE_ADDRESS_T &dma_wraddr = get_dma(WR_DMA).DMA_WRITE_ADDRESS;
    dma_wraddr.Address = 0;
    DMA_WRITE_REG(WRITE_ADDRESS, dma_wraddr.val);

    // DMA trasaction length
    DMA_LENGTH_T &dma_len = get_dma(WR_DMA).DMA_LENGTH;
    dma_len.Length = 0;
    DMA_WRITE_REG(LENGTH, dma_len.val);
}

void
VIO_Resource::mipi_dma_init()
{

    // DMA Reset
    DMA_CONTROL_T &dma_control = get_dma(MIPI_DMA).DMA_CONTROL;
    dma_control.Byte = 0;
    dma_control.HW = 0;
    dma_control.Word = 0;
    dma_control.Go = 0;
    dma_control.I_En = 0;
    dma_control.ReEn = 0;
    dma_control.WrEn = 0;
    dma_control.LenEn = 0;
    dma_control.RCon = 0;
    dma_control.WCon = 0;
    dma_control.WCon = 0;
    dma_control.DoubleWord = 0;
    dma_control.QuadWord   = 0;
    dma_control.SoftwareReset = 1;
    MIPI_DMA_WRITE_REG(CONTROL, dma_control.val);
    MIPI_DMA_WRITE_REG(CONTROL, dma_control.val);
    dma_control.SoftwareReset = 0;
    MIPI_DMA_WRITE_REG(CONTROL, dma_control.val);

    // DMA read address
    DMA_READ_ADDRESS_T &dma_rdaddr = get_dma(MIPI_DMA).DMA_READ_ADDRESS;
    dma_rdaddr.Address = 0;
    MIPI_DMA_WRITE_REG(READ_ADDRESS, dma_rdaddr.val);

    // DMA write address
    DMA_WRITE_ADDRESS_T &dma_wraddr = get_dma(MIPI_DMA).DMA_WRITE_ADDRESS;
    dma_wraddr.Address = 0;
    MIPI_DMA_WRITE_REG(WRITE_ADDRESS, dma_wraddr.val);

    // DMA trasaction length
    DMA_LENGTH_T &dma_len = get_dma(MIPI_DMA).DMA_LENGTH;
    dma_len.Length = 0;
    MIPI_DMA_WRITE_REG(LENGTH, dma_len.val);
}

void
VIO_Resource::rd_dma_init()
{

    // DMA Reset
    DMA_CONTROL_T &dma_control = get_dma(RD_DMA).DMA_CONTROL;
    dma_control.Byte = 0;
    dma_control.HW = 0;
    dma_control.Word = 0;
    dma_control.Go = 0;
    dma_control.I_En = 0;
    dma_control.ReEn = 0;
    dma_control.WrEn = 0;
    dma_control.LenEn = 0;
    dma_control.RCon = 0;
    dma_control.WCon = 0;
    dma_control.WCon = 0;
    dma_control.DoubleWord = 0;
    dma_control.QuadWord   = 0;
    dma_control.SoftwareReset = 1;
    RD_DMA_WRITE_REG(CONTROL, dma_control.val);
    RD_DMA_WRITE_REG(CONTROL, dma_control.val);
    dma_control.SoftwareReset = 0;
    RD_DMA_WRITE_REG(CONTROL, dma_control.val);

    // DMA read address
    DMA_READ_ADDRESS_T &dma_rdaddr = get_dma(RD_DMA).DMA_READ_ADDRESS;
    dma_rdaddr.Address = 0;
    RD_DMA_WRITE_REG(READ_ADDRESS, dma_rdaddr.val);

    // DMA write address
    DMA_WRITE_ADDRESS_T &dma_wraddr = get_dma(RD_DMA).DMA_WRITE_ADDRESS;
    dma_wraddr.Address = 0;
    RD_DMA_WRITE_REG(WRITE_ADDRESS, dma_wraddr.val);

    // DMA trasaction length
    DMA_LENGTH_T &dma_len = get_dma(RD_DMA).DMA_LENGTH;
    dma_len.Length = 0;
    RD_DMA_WRITE_REG(LENGTH, dma_len.val);
}

void
VIO_Resource::write_register(Bus_Type type, const char *str, uint32_t reg, uint32_t val)
{
    AXI_Bus &c = get_bus(type);
    volatile uint32_t *base = (uint32_t *)c.ptr;
    base = base + (reg / 4);
    PRINTF("CfgW %28s reg:0x%08x (%p), val:0x%08x  ... ", str,
           c.base + reg, base, val);
    *base = val;
    PRINTF(" Done!!\n");
}

uint32_t
VIO_Resource::read_register(Bus_Type type, const char *str, uint32_t reg)
{
    AXI_Bus &c = get_bus(type);
    volatile uint32_t *base = (uint32_t *)c.ptr;
    base = base + (reg / 4);
    if(str) PRINTF("CfgR %28s reg:0x%08x (%p), ", str, c.base + reg, base);
    volatile uint32_t val = *base;
    if(str) PRINTF("val:0x%08x\n", val);
    return val;
}

uint32_t
VIO_Resource::read_register(Bus_Type type, const char *str, uint32_t reg, uint32_t init)
{
    AXI_Bus &c = get_bus(type);
    volatile uint32_t *base = (uint32_t *)c.ptr;
    base = base + (reg / 4);
    volatile uint32_t val = *base;
    if(init != val) {
        PRINTF("CfgR %28s reg:0x%08x (%p), ", str, c.base + reg, base);
        PRINTF("val:0x%08x\n", val);
    }
    return val;
}

inline void
VIO_Resource::dma_data_2_hw(uint64_t src_pa, uint64_t dst_pa, size_t size, bool nb)
{
    PRINTF("dma_data_2_hw(): src:0x%08llx, dst:0x%08llx, size:0x%08x\n",
           src_pa, dst_pa, size);
    size = (size + 0xf) & (~0xf); // Quad-Word transfer assert( ! (size & 0x3));
    assert( ! (dst_pa & 0xf));
    assert( ! (src_pa >> 32));
    assert( ! (dst_pa >> 32));

    DMA_t &dma = get_dma(WR_DMA);
    
    // DMA read address
    DMA_READ_ADDRESS_T &dma_rdaddr = dma.DMA_READ_ADDRESS;
    dma_rdaddr.Address =  src_pa;
    DMA_WRITE_REG(READ_ADDRESS, dma_rdaddr.val);

    // DMA write address
    DMA_WRITE_ADDRESS_T &dma_wraddr = dma.DMA_WRITE_ADDRESS;
    dma_wraddr.Address = dst_pa;
    DMA_WRITE_REG(WRITE_ADDRESS, dma_wraddr.val);

    // DMA trasaction length
    DMA_LENGTH_T &dma_len = dma.DMA_LENGTH;
    dma_len.Length = size;
    DMA_WRITE_REG(LENGTH, dma_len.val);

    // DMA control
    DMA_CONTROL_T &dma_control = dma.DMA_CONTROL;
    dma_control.Word = 0;
    dma_control.LenEn = 1;
    dma_control.QuadWord = 1;
    dma_control.Go = 0;
    DMA_WRITE_REG(CONTROL, dma_control.val);

    // DMA status
    DMA_STATUS_T &dma_status = dma.DMA_STATUS;
    dma_status.Done = 0;
    DMA_WRITE_REG(STATUS, dma_status.val);

    // Trigger DMA
    dma_control.Go = 1;
    DMA_WRITE_REG(CONTROL, dma_control.val);

    // POLL DMA status
    if(!nb) {
        DMA_POLL_N_WAIT(STATUS, dma_status.val, dma_status.val != 0x11);
    } 
}

void
VIO_Resource::mipi_dma_2_hw(uint64_t src_pa, uint64_t dst_pa, size_t size, bool nb)
{
    PRINTF("mipi_dma_2_hw(): src:0x%08llx, dst:0x%08llx, size:0x%08x\n",
           src_pa, dst_pa, size);
    size = (size + 0xf) & (~0xf); // Quad-Word transfer assert( ! (size & 0x3));
    assert( ! (dst_pa & 0xf));
    assert( ! (src_pa >> 32));
    assert( ! (dst_pa >> 32));

    DMA_t &dma = get_dma(MIPI_DMA);
    
    // DMA read address
    DMA_READ_ADDRESS_T &dma_rdaddr = dma.DMA_READ_ADDRESS;
    dma_rdaddr.Address =  src_pa;
    MIPI_DMA_WRITE_REG(READ_ADDRESS, dma_rdaddr.val);

    // DMA write address
    DMA_WRITE_ADDRESS_T &dma_wraddr = dma.DMA_WRITE_ADDRESS;
    dma_wraddr.Address = dst_pa;
    MIPI_DMA_WRITE_REG(WRITE_ADDRESS, dma_wraddr.val);

    // DMA trasaction length
    DMA_LENGTH_T &dma_len = dma.DMA_LENGTH;
    dma_len.Length = size;
    MIPI_DMA_WRITE_REG(LENGTH, dma_len.val);

    // DMA control
    DMA_CONTROL_T &dma_control = dma.DMA_CONTROL;
    dma_control.Word = 0;
    dma_control.LenEn = 1;
    dma_control.QuadWord = 1;
    dma_control.Go = 0;
    MIPI_DMA_WRITE_REG(CONTROL, dma_control.val);

    // DMA status
    DMA_STATUS_T &dma_status = dma.DMA_STATUS;
    dma_status.Done = 0;
    MIPI_DMA_WRITE_REG(STATUS, dma_status.val);

    // Trigger DMA
    dma_control.Go = 1;
    MIPI_DMA_WRITE_REG(CONTROL, dma_control.val);

    // POLL DMA status
    if(!nb) {
        MIPI_DMA_POLL_N_WAIT(STATUS, dma_status.val, dma_status.val != 0x11);
    }
    PRINTF("mipi_dma_2_hw(): Done !!\n");
}

inline void
VIO_Resource::dma_data_2_cpu(uint64_t src_pa, uint64_t dst_pa, size_t size)
{
    PRINTF("dma_data_2_cpu(): src:0x%08llx, dst:0x%08llx, size:0x%08x\n",
           src_pa, dst_pa, size);
    assert( ! (size & 0x3));
    assert( ! (dst_pa & 0xf));
    assert( ! (src_pa >> 32));
    assert( ! (dst_pa >> 32));

    DMA_t &dma = get_dma(RD_DMA);
    
    DMA_CONTROL_T &dma_control = dma.DMA_CONTROL;
    dma_control.val = 0;
    dma_control.Go  = 0;
    RD_DMA_WRITE_REG(CONTROL, dma_control.val);

    // DMA read address
    DMA_READ_ADDRESS_T &dma_rdaddr = dma.DMA_READ_ADDRESS;
    dma_rdaddr.Address =  src_pa;
    RD_DMA_WRITE_REG(READ_ADDRESS, dma_rdaddr.val);

    // DMA write address
    DMA_WRITE_ADDRESS_T &dma_wraddr = dma.DMA_WRITE_ADDRESS;
    dma_wraddr.Address = dst_pa;
    RD_DMA_WRITE_REG(WRITE_ADDRESS, dma_wraddr.val);

    // DMA trasaction length
    DMA_LENGTH_T &dma_len = dma.DMA_LENGTH;
    dma_len.Length = size;
    RD_DMA_WRITE_REG(LENGTH, dma_len.val);

    // DMA control
    // DMA_CONTROL_T &dma_control = dma.DMA_CONTROL;
    dma_control.Word = 0;
    dma_control.LenEn = 1;
    dma_control.QuadWord = 1;
    //dma_control.Word = 1;
    RD_DMA_WRITE_REG(CONTROL, dma_control.val);

    // DMA status
    DMA_STATUS_T &dma_status = dma.DMA_STATUS;
    dma_status.Done = 0;
    RD_DMA_WRITE_REG(STATUS, dma_status.val);

    // Trigger DMA
    dma_control.Go = 1;
    RD_DMA_WRITE_REG(CONTROL, dma_control.val);

    // POLL DMA status
    RD_DMA_POLL_N_WAIT(STATUS, dma_status.val, dma_status.val != 0x11);

    //dma_control.Go = 0;
    //1DMA_WRITE_REG(CONTROL, dma_control.val); 
}

dma_hdl_t
VIO_Resource::sgdma_2_hw_add(uint64_t src_pa, uint64_t dst_pa, size_t size, bool first, bool last)
{    
    PRINTF("sgdma_2_hw_add()\n");
    DMA_MSG_STATUS_T &status   = dma_msg.DMA_STATUS;
    DMA_MSG_CONTROL_T &control = dma_msg.DMA_CONTROL;
    
    if(first) { // first descriptor
        status.val = DMA_MSG_READ_REG(STATUS);
        if(status.Busy || status.DescBufFull) {
            WARN(true, "MSG-DMA is busy or descriptor buffer is full");
            PRINTF("Reseting SGDMA ... \n");
            control.val            = 0;
            control.StopDispatcher = 1;
            DMA_MSG_WRITE_REG(CONTROL, control.val);
            DMA_MSG_POLL_N_WAIT(STATUS, status.val, status.Stopped == 0);

            control.val = 0;
            control.ResetDispatcher = 1;
            DMA_MSG_WRITE_REG(CONTROL, control.val);
            DMA_MSG_POLL_N_WAIT(STATUS, status.val, status.Resetting == 1);
            
            ASSERT(!(status.Busy || status.DescBufFull),
                   "MSG-DMA is busy or descriptor buffer is full");
        }
        WARN(status.Stopped || status.StoppedOnError || status.StoppedOnEarlyTerm,
             "MSG-DMA was stoped for previous transfer");
        WARN(status.IRQ, "Clearing interrupt for previous transfer");
        control.val             = 0;
        control.StopOnError     = 1;
        control.StopOnEarlyTerm = 1;
        DMA_DESC_WRITE_REG(CONTROL, control.val);
    }
    DMA_DESC_WRITE_REG(READ_ADDRESS, 0x30b8d000);
    DMA_DESC_WRITE_REG(WRITE_ADDRESS, 0x0);
    DMA_DESC_WRITE_REG(LENGTH, 0x40);
    DMA_DESC_WRITE_REG(BURST_N_ID, 0x0);
    DMA_DESC_WRITE_REG(STRIDE, 0x10001);
    DMA_DESC_WRITE_REG(READ_ADDRESS_HI, 0x0);
    DMA_DESC_WRITE_REG(WRITE_ADDRESS_HI, 0x0);
    DMA_DESC_WRITE_REG(CONTROL, 0x80000000);
    status.val = 0xfff0;
    DMA_MSG_POLL_N_WAIT(STATUS, status.val, status.Busy == 0x1);

    m_unique_seq++; if(!m_unique_seq) {m_unique_seq++;}
    dma_desc.READ_ADDRESS.Address     = LOW_32(src_pa);
    dma_desc.READ_ADDRESS_HI.Address  = HIGH_32(src_pa);
    dma_desc.WRITE_ADDRESS.Address    = LOW_32(dst_pa);
    dma_desc.WRITE_ADDRESS_HI.Address = HIGH_32(dst_pa);
    dma_desc.LENGTH.Length            = 0x1000;
    dma_desc.BURST_N_ID.ReadBurst     = 0;
    dma_desc.BURST_N_ID.WriteBurst    = 0;
    dma_desc.BURST_N_ID.SeqNum        = m_unique_seq = 0;
    dma_desc.STRIDE.ReadStride        = 1;
    dma_desc.STRIDE.WriteStride       = 1;
    dma_desc.CONTROL.EarlyDoneEnable  = (last ? 0 : 1);
    dma_desc.CONTROL.val              = 0;
    dma_desc.CONTROL.Go               = 1;

#define WRITE_TO_DMA(reg) DMA_DESC_WRITE_REG(reg, dma_desc.reg.val)
    WRITE_TO_DMA(READ_ADDRESS);
    WRITE_TO_DMA(WRITE_ADDRESS);
    WRITE_TO_DMA(LENGTH);
    WRITE_TO_DMA(BURST_N_ID);
    WRITE_TO_DMA(STRIDE);
    WRITE_TO_DMA(READ_ADDRESS_HI);
    WRITE_TO_DMA(WRITE_ADDRESS_HI);
    WRITE_TO_DMA(CONTROL);
    //dma_desc.CONTROL.Go = 1;
    //WRITE_TO_DMA(CONTROL);    
#undef WRITE_TO_DMA
    
    return m_unique_seq;
}

void
VIO_Resource::sgdma_2_hw_wait(dma_hdl_t &hdl)
{
#if 0
    usleep(1000);
#else
    DMA_MSG_STATUS_T &status = dma_msg.DMA_STATUS;
    status.val = 0xFFF0;
    DMA_MSG_POLL_N_WAIT(STATUS, status.val, status.Busy == 0x1);
    WARN(!status.DescBufEmpty, "Not all descriptors are completed");
#endif
    
    DMA_MSG_SEQ_STATUS_T &seq_status = dma_msg.DMA_SEQ_STATUS;
    seq_status.val = DMA_MSG_READ_REG(SEQ_STATUS);
    ASSERT(hdl == seq_status.WriteSeqNum, "Tx Incomplete, DMA SeqNum:%d, Exp SeqNum:%d",
           (int)seq_status.WriteSeqNum, (int)hdl);
}


void
VIO_Resource::dma_2_hw_wait()
{
    DMA_STATUS_T &dma_status = get_dma(WR_DMA).DMA_STATUS;
    dma_status.Done = 0;
    DMA_POLL_N_WAIT(STATUS, dma_status.val, dma_status.val != 0x11);
}

void
VIO_Resource::mipi_dma_2_hw_wait()
{
    DMA_STATUS_T &dma_status = get_dma(MIPI_DMA).DMA_STATUS;
    dma_status.Done = 0;
    MIPI_DMA_POLL_N_WAIT(STATUS, dma_status.val, dma_status.val != 0x11);
}

dma_hdl_t sgdma_2_hw_add(uint64_t src_pa, uint64_t dst_pa, size_t size, bool first, bool last)
{
    return g_vio_resource.sgdma_2_hw_add(src_pa, dst_pa, size, first, last);
}

void sgdma_2_hw_wait(dma_hdl_t &hdl)
{
    g_vio_resource.sgdma_2_hw_wait(hdl);
}

void  vio_write_register(Bus_Type bus_type, const char* str, uint32_t reg, uint32_t val)
{
    g_vio_resource.write_register(bus_type, str, reg, val);
}
uint32_t vio_read_register(Bus_Type type, const char*str, uint32_t reg)
{
    return g_vio_resource.read_register(type, str, reg);
}
uint32_t vio_read_register(Bus_Type type, const char*str, uint32_t reg, uint32_t init)
{
    return g_vio_resource.read_register(type, str, reg, init);
}
void dma_data_2_hw(uint64_t src_pa, uint64_t dst_pa, size_t size, bool nb)
{
    g_vio_resource.dma_data_2_hw(src_pa, dst_pa, size, nb);
}
void dma_data_2_cpu(uint64_t src_pa, uint64_t dst_pa, size_t size)
{
    g_vio_resource.dma_data_2_cpu(src_pa, dst_pa, size);
}
void dma_2_hw_wait()
{
    g_vio_resource.dma_2_hw_wait();
}
void mipi_dma_2_hw(const void *src, uint64_t dst_off, size_t size, bool nb)
{
    uint64_t src_pa = vio_phys_addr(src, size);
    ASSERT(src_pa, "Source address is not on contiguous region");

    size_t dma_size = ALIGN_16(size);
    if(dma_size) {
        g_vio_resource.mipi_dma_2_hw(src_pa, dst_off, size, nb);
    }
}
void wait_mipi_dma()
{
    g_vio_resource.mipi_dma_2_hw_wait();
}
uint64_t vio_phys_addr(const void *ptr, size_t size)
{
    return g_vio_resource.get_mem()->get_phys_addr(ptr, size);
}
uint64_t vio_sram_size()
{
    return g_vio_resource.get_bus(VIO_SRAM).size();
}
uint8_t* vio_sram_ptr()
{
    return g_vio_resource.get_bus(VIO_SRAM).ptr;
}
uint64_t vio_sram_base()
{
    return g_vio_resource.get_bus(VIO_SRAM).base;
}

void vio_map_buf(const void *ptr, size_t size, bool to_dev, bool map, uint32_t &cookie)
{
    //g_vio_resource.get_mem()->map_buf(const_cast<void*>(ptr), size, to_dev, map, cookie); 
}

static void vio_dma_2_hw(const void *src, uint64_t dst_off, size_t size)
{
    uint64_t src_pa = vio_phys_addr(src, size);
    ASSERT(src_pa, "Source address is not on contiguous region");
    
    size_t dma_size = ALIGN_16(size); //(size & (~0xf)); // Due VIO address decode 128-bit requirement
    if(dma_size) {
        uint32_t cookie;
        vio_map_buf(src, dma_size, true /*to_dev*/, true, cookie);
        dma_data_2_hw(src_pa, dst_off, dma_size, false /*nb*/);
        vio_map_buf(src, dma_size, true /*to_dev*/, false, cookie);
    }
}

void vio_dma_image(const void *src, size_t size)
{
    vio_dma_2_hw(src, VIO_IMG1_BUF, size);
}

void vio_mipi_dma_image(const void *src, size_t size)
{
    mipi_dma_2_hw( src, VIO_MIPI1_BUF, size, false/*nb*/);
}


__attribute__ ((visibility("default"))) extern uint32_t g_val;
uint32_t g_val = 0;
