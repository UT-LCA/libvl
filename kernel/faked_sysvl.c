#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "faked_sysvl.h"

#define MMAP_ZONE_PROD 0x000000002E200000
#define MMAP_ZONE_CONS 0x000000002E300000
#define MMAP_CTRL_ADDRTAB_LINK 0x000000002E0C0000

/* faked syscall and variables should be moved to linux kernel */
void *prod_page_pool = NULL;
void *cons_page_pool = NULL;
volatile void *prod_devmem_base = NULL;
volatile void *cons_devmem_base = NULL;
uint8_t *ctrl_addrtab_link = NULL;

int faked_sysvl_init() {
    static bool initialized = false;
    if (!initialized) {
        prod_page_pool = memalign(4096, 4096 * VL_MAX);
        cons_page_pool = memalign(4096, 4096 * VL_MAX);
#ifdef NOGEM5 /* no guarantee on reserved device memory, avoid segfault */
        prod_devmem_base = (volatile void*) memalign(4096, 4096 * VL_MAX);
        cons_devmem_base = (volatile void*) memalign(4096, 4096 * VL_MAX);
        ctrl_addrtab_link = (uint8_t*) memalign(4096, 2 * VL_MAX);
#else
        /* memory map to get the virtual address of device memory */
        int fd = open("/dev/mem", O_RDWR);
        if (0 > fd) {
            printf("\033[91mFAILED to open /dev/mem: %d\033[0m\n", fd);
        }
        prod_devmem_base = (volatile void*) mmap(NULL, 4096 * VL_MAX,
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMAP_ZONE_PROD);
        cons_devmem_base = (volatile void*) mmap(NULL, 4096 * VL_MAX,
                PROT_READ | PROT_WRITE, MAP_SHARED, fd, MMAP_ZONE_CONS);
        ctrl_addrtab_link = (uint8_t*) mmap(NULL, 2 * VL_MAX,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                fd, MMAP_CTRL_ADDRTAB_LINK);
#endif
        int cnt;
        for (cnt = 0; 256 * 2 > cnt; ++cnt) {
            ctrl_addrtab_link[cnt] = 0; /* invalidate addrTab rows */
        }
        initialized = true;
    }
    return 0;
}
int faked_sys_mkvl(void **prod_head_page_base,
                   void **cons_head_page_base,
                   volatile void **prod_head_devmem_base,
                   volatile void **cons_head_devmem_base)
{
    static uint64_t cnt = 1; /* count of invocation of this function */
    *prod_head_page_base = prod_page_pool + ((cnt & 0x00FF) << 12);
    *cons_head_page_base = cons_page_pool + ((cnt & 0x00FF) << 12);
    *prod_head_devmem_base = prod_devmem_base + ((cnt & 0x00FF) << 12);
    *cons_head_devmem_base = cons_devmem_base + ((cnt & 0x00FF) << 12);
    ctrl_addrtab_link[cnt] = cnt; /* for producer devmem */
    ctrl_addrtab_link[cnt + 256] = cnt; /* for consumer devmem */
    return (cnt++); /* return file descriptor */
}
