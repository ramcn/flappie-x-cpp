/*
 * vio_drv.c
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>
#include "vio_drv.h"

#define SUCCESS  0
#define FAILURE  -1
#define MAJOR_NUM    51
#define MINOR_NUM    0
#define DEVICE_NAME  "vio_drv"
#define PRINTK_D(...) //printk("[%s]: ", DEVICE_NAME); printk(__VA_ARGS__)
#define PRINTK(...)   printk("[%s]: ", DEVICE_NAME); printk(__VA_ARGS__)

#define RESV_ON_INSERT 1 // INSERT or OPEN
#define ORDER_4MB_BUFFERS 10
#define LOG_PAGE_SIZE     14
//#define NUM_BUFS 1
#define USE_COHERENT_MEM 1
static int s_busy = 0;
unsigned long  phys_page = 0;
unsigned long  phys_addr = 0;
dma_addr_t     dma_addr  = 0;
void *virt_buf = NULL;
static int major;
static struct class *dev_class = NULL;
static struct device *dev = NULL;
#define CLASS_NAME "vio_class"

#define CHECK_STATUS(x) \
    status = x;  \
    if(status) { \
        PRINTK("Failed in calling %s, status %d\n", #x, status);   \
        return status; \
    }

#define RET_ON_ASSERT(cond, ...) \
    if(!(cond)) {                \
        PRINTK(__VA_ARGS__);     \
        return FAILURE;          \
    }


static int reserve_mem(void)
{
#if USE_COHERENT_MEM
    virt_buf = dma_alloc_coherent(dev, 1 << ORDER_4MB_BUFFERS, &dma_addr, GFP_USER | GFP_DMA);
    RET_ON_ASSERT(virt_buf != NULL, "failed to get large page\n");
    phys_addr = virt_to_phys(virt_buf);
#else
    phys_page = __get_free_pages(GFP_USER | GFP_DMA, ORDER_4MB_BUFFERS);
    RET_ON_ASSERT(phys_page != 0, "failed to get large page\n");
    dma_addr = phys_addr = virt_to_phys((void *)phys_page);
#endif
        
    PRINTK(" reserved phys page at 0x%08lx, dma addr 0x%08x\n", phys_addr, dma_addr);
    return SUCCESS;
}

static void unres_mem(void)
{
    PRINTK( "freeing phys page for 0x%08lx, dma addr 0x%08x\n", phys_addr, dma_addr);
#if USE_COHERENT_MEM
    dma_free_coherent(dev, 1 << ORDER_4MB_BUFFERS, virt_buf, dma_addr);
#else
    free_pages(phys_page, ORDER_4MB_BUFFERS);
#endif
}

static int device_open(struct inode *inode, struct file *file)
{
#if (!RESV_ON_INSERT)
    int  status;
#endif
    PRINTK_D( "device open, file %p\n", file);

    if(s_busy > 0) {
        printk("device busy, file %p\n", file);
        return -EBUSY;
    }
    s_busy = 1;

#if (!RESV_ON_INSERT)
    CHECK_STATUS(reserve_mem());
#endif
    
    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
#if (!RESV_ON_INSERT)
    unres_mem();
#endif
    PRINTK_D( "device release, file %p\n", file);
    s_busy = 0;
    module_put(THIS_MODULE);
    return SUCCESS;
}

static ssize_t device_read(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    printk("device_read() not supported yet\n");
    return 0;
}

static ssize_t device_write(struct file *file,
                            const char __user *buffer,
                            size_t length, loff_t *offset)
{
    printk("device_write() not supported yet\n");
    return 0;
}

//extern void v7_dma_inv_range(const void *, const void *);
//extern void v7_dma_clean_range(const void *, const void *);
extern void v7_flush_dcache_all(void);
static long device_ioctl(struct file *file,
                         unsigned int ioctl_num,
                         unsigned long ioctl_param)
{
    long status = FAILURE;
    switch(ioctl_num) {
    case IOCTL_SET_MEM_SIZE:
    {
        unsigned long size = ioctl_param;
        if(size > (1 << (ORDER_4MB_BUFFERS + LOG_PAGE_SIZE))) {
            PRINTK("Size (0x%08lx) requested is too large\n", size);
            status = FAILURE;
        } else {
            status = SUCCESS;
        }
        break;
    }
    case IOCTL_GET_RSVD_MEM:
    {
        int ret_val; void *ptr;
        ptr = (void *) ioctl_param;
        ret_val = copy_to_user(ptr, &phys_addr, sizeof(phys_addr));
        status  = SUCCESS;
        break;
    }
    case IOCTL_GET_DMA_ADDR:
    {
        int ret_val; void *ptr; unsigned long val;
        ptr = (void *) ioctl_param;
        val = (unsigned long)dma_addr;
        ret_val = copy_to_user(ptr, &val, sizeof(val));
        status  = SUCCESS;
        break;
    }
    case IOCTL_MM_CACHE:
    {
#if (!USE_COHERENT_MEM)
        int ret_val; MM_Cache u; void *ptr; int dir; void *usr_addr;
        ptr = (void *)ioctl_param; 
        ret_val = copy_from_user(&u, ptr, sizeof(u));
        dir = u.flags.dma_dir == TO_DEVICE ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
        usr_addr = u.usr_addr; //(void *)(phys_page + u.addr_off);
        //__cpuc_flush_user_all();
        //__cpuc_flush_kern_all();
        status = SUCCESS;
        
        switch(u.flags.ctrl) {
        case MAP_AREA:
            u.dma_addr = dma_map_single(dev, usr_addr, u.size, dir);
            if(dma_mapping_error(dev, u.dma_addr)) {
                return FAILURE;
            } 
            dma_sync_single_for_device(dev, u.dma_addr, u.size, dir);
            /*
            dma_sync_single_for_cpu(dev, u.dma_addr, u.size, dir);
            */
            if(dir == DMA_TO_DEVICE) {
                v7_dma_flush_range(usr_addr, ((char*)usr_addr) + u.size);
            }
            
            v7_dma_map_area(usr_addr, u.size, dir);
            //v7_flush_dcache_all();
            ret_val = copy_to_user(ptr, &u, sizeof(u));
            //v7_dma_map_area(usr_addr, u.size, dir);
            break;
        case UNMAP_AREA: 
            //dma_sync_single_for_cpu(dev, u.dma_addr, u.size, dir);
            /*
            dma_sync_single_for_device(dev, u.dma_addr, u.size, dir);
            */
            //dma_unmap_single(dev, u.dma_addr, u.size, dir);
            //v7_dma_unmap_area(usr_addr, u.size, dir);
            //__cpuc_unmap_area(usr_addr, u.size, dir);
            break;
        default:
            status = FAILURE;
            break;
        }
#else
        status = FAILURE;        
#endif
        //dmac_flush_range(virt_buf, ((char*)virt_buf) + 1024);
        //__cpuc_flush_user_range(virt_buf, 1024, vm_flags); 
        break;
    }
    default:
        PRINTK("Not supported ioctl call, num %d\n", ioctl_num);
        break;
    }
    PRINTK_D("ioctl exit, status %ld\n", status);
    return status;
}

struct file_operations fops = {
    .read  = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .open  = device_open,
    .release = device_release,
};

static int device_init(void)
{
#if RESV_ON_INSERT
    int status;
    CHECK_STATUS(reserve_mem());
#endif
    
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if(major < 0) {
        printk("Failed registering character device, err %d\n", major);
        return major;
    }
    dev_class = class_create(THIS_MODULE, CLASS_NAME);
    if(dev_class == NULL) {
        unregister_chrdev(major, DEVICE_NAME);
        printk("failed class_creat");
        return 1;
    }
    dev = device_create(dev_class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if(dev == NULL) {
        class_destroy(dev_class);
        unregister_chrdev(major, DEVICE_NAME);
        printk("failed class_creat");
        return 1; 
    }
    return 0;
}

static void device_exit(void)
{
#if RESV_ON_INSERT
    unres_mem();
#endif
    device_destroy(dev_class, MKDEV(major, 0));
    class_unregister(dev_class);
    class_destroy(dev_class);
    unregister_chrdev(major, DEVICE_NAME);
}


module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Om J Omer <om.j.omer@intel.com>");
MODULE_DESCRIPTION("VIO HWA Driver");
MODULE_VERSION("1.1");
