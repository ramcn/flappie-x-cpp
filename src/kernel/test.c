#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "vio_drv_lib.h"

VIO_Mem* VIO_Mem::m_instance = NULL;
int main (int argc, char *argv[])
{
    int off = atoi(argv[1]);
    VIO_Mem *vio =VIO_Mem::get_instance();
    unsigned int *ptr = (unsigned int *)vio->get_mem(off);
    printf("Value at off 0x%08llx = 0x%08x\n", vio->get_phys_addr(ptr, sizeof(int)), *ptr);
    VIO_Mem::delete_instance();
    return 0;
}
