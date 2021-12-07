#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <malloc.h>
#include <string.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "caf_insts.h" /* caf-related instructions */
#include "caf.h"

#define CAF_MAX 64
#define MMAP_ZONE_QMD 0x000000002E400000

#if defined(__GNUC__)
#define CACHE_ALIGNED __attribute__((aligned(64))) /* clang and GCC */
#elif defined(_MSC_VER)
#define CACHE_ALIGNED __declspec(align(64))        /* MSVC */
#endif

#define CAS(a_ptr, a_oldVal, a_newVal) \
  __sync_bool_compare_and_swap(a_ptr, a_oldVal, a_newVal)

#define STATIC_ASSERT(COND,MSG) typedef char static_assert_##MSG[(COND)?1:-1]

static void *qmd_base = NULL;

/* Create an endpoint of CAF queue */
int open_caf(int fd, cafendpt_t *endptr)
{
    endptr->fd = fd;
    if (NULL == qmd_base) {
        /* Memory map QMD */
        int devmem_fd = open("/dev/mem", O_RDWR);
        if (0 > devmem_fd) {
            printf("\033[91mFAILED to open /dev/mem: %d\033[0m\n", devmem_fd);
        }
        qmd_base = (void*) mmap(NULL, 64 * CAF_MAX,
                PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, MMAP_ZONE_QMD);
        if (MAP_FAILED == qmd_base) {
            perror("mmap() failed");
        }
    }
    endptr->pqmd = (volatile void*) ((uint64_t)qmd_base + (fd << 6));
    return 0;
}

/* Destroy an endpoint of CAF queue */
int close_caf(cafendpt_t endpt)
{
    endpt.fd = endpt.fd;
    return 0;
}

/* Enqueue */
static bool caf_push(cafendpt_t *endptr, uint64_t data, bool isblocking)
{
    uint64_t credit = 1;
    do {
        credit = 1;
        CAF_ENC(credit, endptr->pqmd);
        if (credit) {
            CAF_ENQ(data, endptr->pqmd);
        }
    } while (isblocking && (0 == credit));
    return (0 != credit);
}

/* specializations of caf_push() */
bool caf_push_non(cafendpt_t *endptr, uint64_t data)
{ return caf_push(endptr, data, false); }
void caf_push_strong(cafendpt_t *endptr, uint64_t data)
{ caf_push(endptr, data, true); }

size_t caf_push_bulk(cafendpt_t *endptr, uint64_t *pdata, size_t cnt)
{
    uint64_t i;
    uint64_t credit = (uint64_t)cnt;
    CAF_ENC(credit, endptr->pqmd);
    for (i = 0; i < credit; ++i) {
        CAF_ENQ(pdata[i], endptr->pqmd);
    };
    return (size_t)credit;
}

/* Dequeue */
static bool caf_pop(cafendpt_t *endptr, uint64_t *pdata, bool isblocking)
{
    uint64_t credit = 1;
    do {
        credit = 1;
        CAF_DEC(credit, endptr->pqmd);
        if (credit) {
            CAF_DEQ(*pdata, endptr->pqmd);
        }
    } while (isblocking && (0 == credit));
    return (0 != credit);
}

/* specializations of caf_pop() */
bool caf_pop_non(cafendpt_t *endptr, uint64_t *pdata)
{ return caf_pop(endptr, pdata, false); }
void caf_pop_strong(cafendpt_t *endptr, uint64_t *pdata)
{
    bool isvalid;
    do {
        isvalid = caf_pop(endptr, pdata, true);
    } while(!isvalid);
}

size_t caf_pop_bulk(cafendpt_t *endptr, uint64_t *pdata, size_t cnt)
{
    uint64_t i;
    uint64_t credit = (uint64_t)cnt;
    CAF_DEC(credit, endptr->pqmd);
    for (i = 0; i < credit; ++i) {
        CAF_DEQ(pdata[i], endptr->pqmd);
    }
    return (size_t)credit;
}

/* prepush data to PoC */
void caf_prepush(void *pdata, size_t size)
{
    CAF_PREPUSH((size + 64) / 64, pdata);
}
