#pragma once

#define DEVICE_PATH "/dev/vio_drv"
enum {
    IOCTL_GET_RSVD_MEM = 5,
    IOCTL_SET_MEM_SIZE,
    IOCTL_GET_DMA_ADDR,
    IOCTL_MM_CACHE,
};


#define TO_DEVICE   0
#define FROM_DEVICE 1

typedef enum {
    MAP_AREA = 0,
    UNMAP_AREA = 1,
} ctrl_t;

typedef union {
    struct {
        uint32_t dma_dir  : 1;
        uint32_t ctrl     : 5;
        uint32_t resvd    :26;
    };
    uint32_t val;
} flag_t;

typedef struct MM_Cache {
    union {
        void *usr_addr;
        unsigned long dma_addr;
    };
    size_t size;
    flag_t flags;
    void *info;
} MM_Cache;
