#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include "vio_drv.h"

#define VIO_MEM_PRINTF(...)     printf("[vio_mem] "); printf(__VA_ARGS__)

#define VIO_MEM_ASSERT(cond, ...)                   \
    if(!(cond)) {                                   \
    printf("[vio_mem] %s:%d ", __FILE__, __LINE__); \
    printf(__VA_ARGS__);                            \
    exit(1);                                        \
    }
#define VIO_DRV_DEBUG(x) std::cout << x

class Mem_Pool
{
public:
    Mem_Pool()
        : m_num_bufs(0), p_is_free(NULL), p_num_chained_bufs(NULL),
          p_prev_buf_id(NULL)
    {
        return;
    }
    ~Mem_Pool()
    {
        VIO_MEM_ASSERT((p_is_free[0]) && (p_num_chained_bufs[0] == m_num_bufs),
                "All allocations are not freed");

        if(p_prev_buf_id) {
            delete [] p_prev_buf_id;
        }
        if(p_num_chained_bufs) {
            delete [] p_num_chained_bufs;
        }
        if(p_is_free) {
            delete [] p_is_free;
        }
    }


    void configure(uint32_t buf_size, uint64_t start_addr, uint64_t end_addr)
    {
        VIO_MEM_ASSERT(start_addr < end_addr, "End Address is less then Start Address");

        uint64_t pool_size = end_addr - start_addr;
        m_buf_size   = buf_size;
        m_num_bufs   = (pool_size / m_buf_size);
        m_start_addr = start_addr;
        m_end_addr   = m_start_addr + (m_buf_size * m_num_bufs);

        if(p_is_free){delete [] p_is_free;}
        p_is_free = new bool[m_num_bufs];
        VIO_MEM_ASSERT(p_is_free, "Allocation failed");

        if(p_num_chained_bufs){delete [] p_num_chained_bufs;}
        p_num_chained_bufs = new uint32_t[m_num_bufs];
        VIO_MEM_ASSERT(p_num_chained_bufs, "Allocation failed");

        if(p_prev_buf_id){delete [] p_prev_buf_id;}
        p_prev_buf_id = new uint32_t[m_num_bufs];
        VIO_MEM_ASSERT(p_prev_buf_id, "Allocation failed");

        p_is_free[0] = true;
        p_num_chained_bufs[0] = m_num_bufs;
        p_prev_buf_id[0] = 0;

        
        VIO_MEM_PRINTF("Pool Created at 0x%llx  with %6d  buffers, %6lld  bytes each\n",
                       m_start_addr, m_num_bufs, m_buf_size);
        
    }

    uint64_t allocate(uint32_t size = 0xffffffff)
    {
        VIO_MEM_ASSERT(size != 0, "Zero Size Allocation is not allowed");

        if(size == 0xffffffff) {
            size = m_buf_size;
        }

        uint32_t numChainedBufs = (size + m_buf_size - 1) / m_buf_size;
        uint32_t id;

        for(id = 0; id < m_num_bufs; ) {
            if(p_is_free[id] && (numChainedBufs <= p_num_chained_bufs[id])) {
                break;
            }
            id += p_num_chained_bufs[id];
        }
        VIO_MEM_ASSERT(id + numChainedBufs <= m_num_bufs, "No more memory left");

        if(numChainedBufs != p_num_chained_bufs[id]) {
            uint32_t nextFreeId = id + numChainedBufs;
            p_is_free[nextFreeId] = true;
            p_num_chained_bufs[nextFreeId] = p_num_chained_bufs[id] - numChainedBufs;
            p_prev_buf_id[nextFreeId] = id;

            uint32_t secondNextId = nextFreeId + p_num_chained_bufs[nextFreeId];
            if(secondNextId < m_num_bufs) {
                p_prev_buf_id[secondNextId] = nextFreeId;
            }
        }
        p_is_free[id] = false;
        p_num_chained_bufs[id] = numChainedBufs;

        uint64_t addr = m_start_addr;
        addr += id * m_buf_size;
        return addr;
    }

    void release(uint64_t addr)
    {
        uint32_t id = (addr - m_start_addr) / m_buf_size;
        VIO_MEM_ASSERT((addr - m_start_addr) == m_buf_size * id, "Invalid Address");

        uint32_t numChainedBufs = p_num_chained_bufs[id];
        VIO_MEM_ASSERT(numChainedBufs != 0, "Invalid Address");
        VIO_MEM_ASSERT(!p_is_free[id], "Address is already deallocated");

        uint32_t nextId = id + p_num_chained_bufs[id];
        if(p_is_free[nextId]) {
            numChainedBufs += p_num_chained_bufs[nextId];
        }
        if(0 < id) {
            uint32_t prevId = p_prev_buf_id[id];
            if(p_is_free[prevId]) {
                id = prevId;
                numChainedBufs += p_num_chained_bufs[prevId];
            }
        }
        VIO_MEM_ASSERT(id + numChainedBufs <= m_num_bufs, "Invalid Address");

        p_is_free[id] = true;
        p_num_chained_bufs[id] = numChainedBufs;
        if(id+numChainedBufs < m_num_bufs) {
            p_prev_buf_id[id+numChainedBufs] = id;
        }
    }

    unsigned int get_size(uint64_t addr) const
    {
        uint32_t id = (addr - m_start_addr) / m_buf_size;
        VIO_MEM_ASSERT((addr - m_start_addr) == (id * m_buf_size), "Invalid Address");
        uint32_t numChainedBufs = p_num_chained_bufs[id];
        return (numChainedBufs * m_buf_size);
    }

    bool is_valid_address(uint64_t addr) const
    {
        if((m_start_addr <= addr) && (addr < m_end_addr)) {
            return true;
        }
        return false;
    }
    uint32_t get_buf_size() const {return m_buf_size;}
    
    
private:
    bool         *p_is_free;
    uint32_t     *p_num_chained_bufs;
    uint32_t     *p_prev_buf_id;
    uint32_t      m_num_bufs;
    uint64_t      m_start_addr;
    uint64_t      m_end_addr;
    uint64_t      m_buf_size;
    std::string name() const {return "virt_mem_mngr";}
    uint64_t get_start_addr() const {return m_start_addr;}
};


class VIO_Mem {
public:
    static VIO_Mem* m_instance;
    static VIO_Mem* get_instance() {
        if(!m_instance) {
            m_instance = new VIO_Mem ;
        }
        return m_instance;
    }
    static void delete_instance() {
        if(m_instance) {
            delete m_instance;
            m_instance = NULL;
        }
    }
    ~VIO_Mem()
    {
        if(m_ptr)         munmap(m_ptr, phys_size);
        if(m_mem_fd > -1) close(m_mem_fd);
        if(m_dev_fd > -1) close(m_dev_fd);
    }
    void* get_mem(uint64_t size) {
        uint64_t addr = m_pool.allocate(size);
        //VIO_MEM_PRINTF("alloc %p\n", (m_ptr + addr));
        return (m_ptr + addr);
    }
    uint64_t get_phys_addr(const void *ptr, size_t size = 0) {
        //VIO_MEM_PRINTF("get_phys_addr %p\n", ptr);
        uint64_t ptr_addr   = (uint64_t)ptr;
        uint64_t start_addr = (uint64_t)m_ptr;
        if((ptr_addr < start_addr) || (ptr_addr + size > start_addr + phys_size)) {
            return 0;
        }
        size_t off = ptr_addr - start_addr;
        return (phys_addr + off);
    }
    void free_mem(void *ptr) {
        uint64_t addr = get_phys_addr(ptr);
        VIO_MEM_ASSERT(addr != 0, "Invalid mem free, %p\n", ptr);
        m_pool.release(addr - phys_addr);
    }
    void map_buf(void *ptr, size_t size, bool to_dev, bool map, uint32_t &cookie) {
        MM_Cache u; int ret_val;
        uint64_t pa = get_phys_addr(ptr, size);
        VIO_MEM_ASSERT(pa !=0 , "Ptr(%p) not mapped to physical address", ptr);
        if(map) {
            u.usr_addr = ptr; //pa - phys_addr;
        } else {
            u.dma_addr = cookie;
        }
        u.size     = size;
        u.flags.dma_dir = to_dev ? TO_DEVICE : FROM_DEVICE;
        u.flags.ctrl    = map ? MAP_AREA : UNMAP_AREA;
        
        ret_val = ioctl(m_dev_fd, IOCTL_MM_CACHE, &u);
        VIO_MEM_ASSERT(ret_val == 0, "ioctl(IOCTL_MM_CACHE) failed\n");
        if(map) {
            cookie = u.dma_addr;
        }
        /*
        VIO_MEM_ASSERT(!map || u.dma_addr == pa, "DMA address(0x%lx) is not same as phys addr(0x%llx)\n",
                       u.dma_addr, pa);
        */
    }
    /*
      uint64_t get_dma_addr(const void *ptr) {
      return (ptr == m_ptr ? dma_addr : 0);
      }
    */

private:
    int       m_dev_fd;
    int       m_mem_fd;
    std::string    dev_name;
    unsigned long  phys_addr;
    unsigned long  dma_addr;
    unsigned long  phys_size;
    char           *m_ptr;
    Mem_Pool        m_pool;
    
private:
    VIO_Mem()
        : dev_name(DEVICE_PATH),
          m_dev_fd(-1),
          m_mem_fd(-1),
          phys_addr(0),
          phys_size(4*1024*1024) // 4MB
    {
        m_dev_fd = open(dev_name.c_str(), 0);
        VIO_MEM_ASSERT(m_dev_fd > -1, "open(%s) failed, fd %d, errno %s\n",
                       dev_name.c_str(), m_dev_fd, strerror(errno));

        int ret_val;
        ret_val = ioctl(m_dev_fd, IOCTL_SET_MEM_SIZE, phys_size);
        VIO_MEM_ASSERT(ret_val == 0, "ioctl(IOCTL_SET_MEM_SIZE) failed\n");

        ret_val = ioctl(m_dev_fd, IOCTL_GET_RSVD_MEM, &phys_addr);
        VIO_MEM_ASSERT(ret_val == 0, "ioctl(IOCTL_GET_RSVD_MEM) failed\n"); 

        ret_val = ioctl(m_dev_fd, IOCTL_GET_DMA_ADDR, &dma_addr);
        VIO_MEM_ASSERT(ret_val == 0, "ioctl(IOCTL_GET_DMA_ADDR) failed\n"); 
        
        m_mem_fd = open("/dev/mem", O_RDWR);
        VIO_MEM_ASSERT(m_mem_fd > -1, "open(/dev/mem) failed: %s\n", strerror(errno));
        
        m_ptr = (char *)mmap(NULL, phys_size, PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_LOCKED,
                             m_mem_fd, phys_addr);
        VIO_MEM_ASSERT(m_ptr, "mmap(0x%lx) failed: %s\n", phys_addr, strerror(errno));
        
        VIO_MEM_PRINTF("resverved phys address is 0x%08lx, virt_buf %p\n",
                       phys_addr, m_ptr);
        m_pool.configure(16 * 1024, 0, phys_size);
    }
};

#undef VIO_MEM_ASSERT
#undef VIO_DEV_PRINTF
