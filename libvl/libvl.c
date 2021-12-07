#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <malloc.h>
#include <string.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "../vl/vl.h"
#include "vl_insts.h" /* vl-related instructions */
#ifdef NOSYSVL
#include <faked_sysvl.h> /* faked vl-related syscalls and structures */
#endif

#if defined(__GNUC__)
#define CACHE_ALIGNED __attribute__((aligned(64))) /* clang and GCC */
#elif defined(_MSC_VER)
#define CACHE_ALIGNED __declspec(align(64))        /* MSVC */
#endif

#define BYTE_CTRL_INIT 0x3D
#define HALF_CTRL_INIT 0x7C
#define WORD_CTRL_INIT 0xB8
#define TWIN_CTRL_INIT 0xF0
#define BYTE_CTRL_UV 0x3F
#define HALF_CTRL_UV 0x7E
#define WORD_CTRL_UV 0xBC
#define TWIN_CTRL_UV 0xF8

#define Addr2Next(addr) ((((uint64_t)addr) >> 6) & 0x3F)
#define Next2Addr(next, addr) (((uint64_t)addr & 0xFFFFFFFFFFFFF000) | \
                               ((uint64_t)next & 0x000000000000003F) << 6)
#define CachelineBaseOf(addr) (((uint64_t)addr) & 0xFFFFFFFFFFFFFF30)
#define PageBaseOf(addr) (((uint64_t)addr) & 0xFFFFFFFFFFFFF000)
#define DevMemSlotNextTo(addr) (PageBaseOf(addr) | \
                                (((uint64_t)addr - 0x0000000000000040) & \
                                 0x0000000000000FFF))

#define ValidProd(ptr, size, max) \
  ((0 > ((max) - (ptr) - (size))) ? (max) : ((max) - (ptr) - (size)))
#define ValidCons(ptr, size, max) \
  (((max) < ((ptr) + (size))) ? 0 : ((ptr) + (size)))

#define CAS(a_ptr, a_oldVal, a_newVal) \
  __sync_bool_compare_and_swap(a_ptr, a_oldVal, a_newVal)

#define STATIC_ASSERT(COND,MSG) typedef char static_assert_##MSG[(COND)?1:-1]

#ifdef NOGEM5
#ifdef PTHREAD_MUTEX
#include <pthread.h>
pthread_mutex_t vlink_lock[256];
#else
volatile uint8_t vlink_lock[256];
#endif
volatile uint64_t prod_head[256];
volatile uint64_t prod_tail[256];
volatile uint64_t cons_head[256];
volatile uint64_t cons_tail[256];
volatile uint8_t prod_buf[256][64 * 256];
volatile uint8_t *cons_buf[256][256];
volatile uint8_t cons_blk_tag[256][256];

static void sw_lock_vlink(int fd) {
#ifdef PTHREAD_MUTEX
    pthread_mutex_lock(&vlink_lock[fd]);
#else
    while (!CAS(&vlink_lock[fd], 1, 0)) {
        /* sched_yield(); */
        __asm__ volatile ("nop" : : :);
    }
#endif
}

static void sw_unlock_vlink(int fd) {
#ifdef PTHREAD_MUTEX
    pthread_mutex_unlock(&vlink_lock[fd]);
#else
    while (!CAS(&vlink_lock[fd], 0, 1)) {
        /* sched_yield(); */
        __asm__ volatile ("nop" : : :);
    }
#endif
}

static void sw_init_vl(int fd) {
    int i;
#ifdef PTHREAD_MUTEX
    pthread_mutex_init(&vlink_lock[fd], NULL);
#else
    vlink_lock[fd] = 1;
#endif
    prod_head[fd] = prod_tail[fd] = 0;
    cons_head[fd] = cons_tail[fd] = 0;
    for (i = 0; 256 > i; ++i) {
        cons_blk_tag[fd][i] = 0;
    }
}

static void try_mapping(int fd) {
    int i;
    while (cons_tail[fd] > cons_head[fd] && prod_tail[fd] > prod_head[fd]) {
        if (cons_blk_tag[fd][cons_head[fd] % 256]) {
            uint8_t sz_ptr = cons_buf[fd][cons_head[fd] % 256][62];
            if (TWIN_CTRL_UV == sz_ptr || WORD_CTRL_UV == sz_ptr ||
                HALF_CTRL_UV == sz_ptr || BYTE_CTRL_UV == sz_ptr) {
                for (i = 0; 63 > i; ++i) {
                    cons_buf[fd][cons_head[fd] % 256][i] =
                        prod_buf[fd][((prod_head[fd] % 256) << 6) + i];
                }
                prod_head[fd]++;
            }
        }
        cons_blk_tag[fd][cons_head[fd] % 256] = 0;
        cons_head[fd]++;
    }
}

static uint64_t sw_vl_push(volatile void *addr, int fd) {
    uint64_t i, ret;
    uint8_t *pbyte = (uint8_t*) addr;
    const uint8_t ctrl_size = pbyte[62] >> 6;
    const uint8_t ctrl_ptr = pbyte[62] & 0x3F;
    const int8_t ptr_size = (0x01 << ctrl_size); /* in byte */
    const int8_t max_valid = 62 - (62 % ptr_size);
    int8_t num_valid = ValidProd(ctrl_ptr, ptr_size, max_valid);

    sw_lock_vlink(fd);
    for (i = cons_head[fd]; cons_tail[fd] > i; ++i) {
        if (cons_buf[fd][i % 256] == addr) {
            cons_blk_tag[fd][i % 256] = 0;
            break;
        }
    }
    if (0 == num_valid) {
        ret = prod_tail[fd] - prod_head[fd];
        sw_unlock_vlink(fd);
        return ret;
    }
    uint64_t tail_tmp = prod_tail[fd];
    uint64_t head_tmp = prod_head[fd];
    uint64_t buf_taken = tail_tmp - head_tmp;
    if (256 <= buf_taken) {
        ret = (0x8000000000000000 | buf_taken);
    } else {
        uint64_t buf_base = (tail_tmp % 256) << 6;
        int8_t right_shift = max_valid - num_valid;
        for (i = 0; (uint64_t) num_valid > i; ++i) {
            prod_buf[fd][buf_base + i] = pbyte[right_shift + i];
        }
        prod_buf[fd][buf_base + 62] =
            (ctrl_size << 6) | ((num_valid - ptr_size) & 0x3F);
        switch (pbyte[62] & 0xC0) {
            case 0x00: pbyte[62] = BYTE_CTRL_INIT; break;
            case 0x40: pbyte[62] = HALF_CTRL_INIT; break;
            case 0x80: pbyte[62] = WORD_CTRL_INIT; break;
            case 0xC0: pbyte[62] = TWIN_CTRL_INIT; break;
        }
        prod_tail[fd]++;
        try_mapping(fd);
        ret = prod_tail[fd] - prod_head[fd];
    }
    sw_unlock_vlink(fd);

    return ret;
}

static uint64_t sw_vl_pop(volatile void *addr, int fd) {
    uint64_t i, ret;
    sw_lock_vlink(fd);
    uint64_t tail_tmp = cons_tail[fd];
    uint64_t head_tmp = cons_head[fd];
    uint64_t buf_taken = tail_tmp - head_tmp;
    /* check pushable flag of cacheline at addr */
    for (i = head_tmp; tail_tmp > i; ++i) {
        if (cons_buf[fd][i % 256] == addr) {
            if(0 == cons_blk_tag[fd][i % 256]) {
                cons_blk_tag[fd][i % 256] = 1;
                try_mapping(fd);
            }
            sw_unlock_vlink(fd);
            return buf_taken;
        }
    }
    if (256 <= buf_taken) {
        ret = (0x8000000000000000 | buf_taken);
    } else {
        cons_buf[fd][tail_tmp % 256] = (void*) addr;
        cons_blk_tag[fd][tail_tmp % 256] = 1;
        cons_tail[fd]++;
        try_mapping(fd);
        ret = cons_tail[fd] - cons_head[fd];
    }
    sw_unlock_vlink(fd);
    return ret;
}
#endif /* end of NOGEM5 */

enum SizeType {
    BYTE = 0,
    HALF = 1,
    WORD = 2,
    TWIN = 3
};

struct StructPageNode_s { /* free cachelines what are continuous */
    void *base;
    uint64_t num_inactive; /* to quickly tell #cachelines free to allocate */
    struct StructPageNode_s *next;
};

typedef struct StructPageNode_s StructPageNode;
typedef struct StructPageNode_s *StructPageNodePtr;

STATIC_ASSERT(0 == (sizeof(StructPageNode) % 2), StructPageNodeSize);

struct VirtualLink {
    StructPageNodePtr prod_head_node;
    StructPageNodePtr cons_head_node;
    StructPageNodePtr free_upon_rmvl; /* to avoid ABA */
    volatile void *prod_devmem; /* points to current non-cachable array */
    volatile void *cons_devmem; /* points to current non-cachable array */
};

static struct VirtualLink links_g[VL_MAX];

/* Wrapper function of sys_mkvl() */
int mkvl()
{
    void *prod_pg_base;
    void *cons_pg_base;
    volatile void *prod_devmem;
    volatile void *cons_devmem;
    /* TODO: implement and use the real syscall */
    /* long int ret = syscall(SYS_mkvl, prod_head_node, cons_head_node); */
#ifdef NOSYSVL
    faked_sysvl_init();
    int fd = faked_sys_mkvl(&prod_pg_base, &cons_pg_base,
                            &prod_devmem, &cons_devmem);
#else
    int fd = sys_mkvl(&prod_pg_base, &cons_pg_base,
                      &prod_devmem, &cons_devmem);
#endif
#ifdef NOGEM5
    sw_init_vl(fd);
#endif

    links_g[fd].prod_head_node =
      (StructPageNodePtr) memalign(2, sizeof(StructPageNode));
    links_g[fd].cons_head_node =
      (StructPageNodePtr) memalign(2, sizeof(StructPageNode));
    links_g[fd].free_upon_rmvl =
      (StructPageNodePtr) memalign(2, sizeof(StructPageNode));

    links_g[fd].prod_head_node->base = 0; /* this is dummy head node */
    links_g[fd].prod_head_node->num_inactive = 0;
    links_g[fd].prod_head_node->next = NULL;
    links_g[fd].cons_head_node->base = 0; /* this is dummy head node */
    links_g[fd].cons_head_node->num_inactive = 0;
    links_g[fd].cons_head_node->next = NULL;
    links_g[fd].free_upon_rmvl->next = (StructPageNodePtr) 0x01;

    links_g[fd].prod_devmem = prod_devmem;
    links_g[fd].cons_devmem = cons_devmem;

    return fd;
}

static bool append_to_free(StructPageNodePtr pdelete, StructPageNodePtr prmvl)
{
    StructPageNodePtr ptail = prmvl;
    StructPageNodePtr newtail = (StructPageNodePtr) 0x01;
    StructPageNodePtr ptailnext =
        (StructPageNodePtr) ((uint64_t) ptail->next & ~0x01);
    while (NULL != ptailnext) { /* until tail: NULL or 0x01 */
        ptail = ptailnext;
        ptailnext = (StructPageNodePtr) ((uint64_t) ptail->next & ~0x01);
        /* Note: only append to free_upon_rmvl, never delete until rmvl(),
         * so this is safe that never would go to a branch */
    }
    pdelete->next = (StructPageNodePtr) 0x01;
    pdelete = (StructPageNodePtr) ((uint64_t) pdelete | 0x01);
    while (!atomic_compare_exchange_strong(&ptail->next, &newtail, pdelete)) {
        ptail = (StructPageNodePtr) ((uint64_t) newtail & ~0x01);
        newtail = (StructPageNodePtr) 0x01;
    }
    return true;
}

static bool logical_delete(StructPageNodePtr pdelete)
{
    uint64_t node_next = (uint64_t) pdelete->next;
    StructPageNodePtr expected = (StructPageNodePtr) (node_next & ~0x01);
    StructPageNodePtr desired = (StructPageNodePtr) (node_next | 0x01);
    return atomic_compare_exchange_strong(&pdelete->next, &expected, desired);
}

static bool physical_delete(StructPageNodePtr pdelete, StructPageNodePtr pprev,
                            StructPageNodePtr phead, StructPageNodePtr prmvl)
{
    uint64_t node_next = (uint64_t) pdelete->next;
    if (!(node_next & 0x01)) { /* not even logically deleted yet */
        return false;
    }
    /* bridge pprev to pdelete->next */
    uint64_t node_delete = (uint64_t) pdelete;
    while ((uint64_t) pprev & ~0x01) { /* until tail: NULL or 0x01 (deleted) */
        uint64_t next_tmp = (uint64_t) pprev->next;
        if ((next_tmp & ~0x01) == node_delete) {
            StructPageNodePtr expected = pdelete;
            StructPageNodePtr desired =
              (StructPageNodePtr) (node_next & ~0x01);
            if (!atomic_compare_exchange_strong(&pprev->next, &expected,
                        desired)) {
                /* pprev has been changed: */
                if ((uint64_t)expected == (node_delete | 0x01)) {
                    /* 1. pprev is deleted */
                    pprev = phead; /* search from head again */
                } else { /* 2. other helped bridging */
                    pprev = NULL;
                    break;
                }
            } else {
                break;
            }
        }
        pprev = (StructPageNodePtr) ((uint64_t) pprev->next & ~0x01);
        /* if reach the end not found any node points to pnode,
         * it is because other has helped removing pnode */
    }

    if (!((uint64_t) pprev & ~0x01)) { /* other thread has bridged */
        return true;
    }

    /* the thread succeed on bridge pprev to next,
     * it is then its responsibility to append pdelete to prmvl */
    append_to_free(pdelete, prmvl);

    return true;
}

static bool insert_structpage(StructPageNodePtr pinsert,
                              StructPageNodePtr phead, StructPageNodePtr prmvl)
{
    while (true) {
        uint64_t pinsert_base = (uint64_t) pinsert->base;
        uint64_t next_base = 0;
        StructPageNodePtr pprev = phead;
        /* after the following while loop,
         * pprev points to where preturn should insert */
        StructPageNodePtr pnext =
            (StructPageNodePtr) ((uint64_t) pprev->next & ~0x01);
        while ((NULL != pnext) && (next_base < pinsert_base)) {
            next_base = (uint64_t) pnext->base;
            uint64_t next_num_inactive = pnext->num_inactive;
            if (next_base > pinsert_base) { /* stop by the 1st bigger one */
                if (pinsert_base + (pinsert->num_inactive << 6) == next_base) {
                    /* take pnext out and merge those two */
                    if (logical_delete(pnext)) {
                        /* successfully marked pnext as deleted */
                        /* update pnew */
                        pinsert->num_inactive += pnext->num_inactive;
                        physical_delete(pnext, pprev, phead, prmvl);
                    } else { /* pnext has been changed */
                        /* TODO: if another thread logically deleted pnext
                         * but died, this thread might try merging pnext
                         * but fail all the time, no more progress */
                        pprev = phead; /* search over from phead */
                        next_base = 0;
                    }
                }
            } else if (next_base + (next_num_inactive << 6) == pinsert_base) {
                /* take pnext and merge those two */
                if (logical_delete(pnext)) {
                    /* successfully marked pnext as deleted */
                    /* update pinsert */
                    pinsert->base = pnext->base;
                    pinsert->num_inactive += next_num_inactive;
                    physical_delete(pnext, pprev, phead, prmvl);
                } else { /* pnext has been changed */
                    /* TODO: if another thread logically deleted pnext
                     * but died, this thread might try merging pnext
                     * but fail all the time, no more progress */
                    pprev = phead; /* search over from phead */
                    next_base = 0;
                }
            } else {
                /* not the place to insert yet, move to next */
                pprev = pnext;
            }
            pnext = (StructPageNodePtr) ((uint64_t) pprev->next & ~0x01);
        }
        pinsert->next = (StructPageNodePtr) ((uint64_t) pprev->next & ~0x01);
        if (atomic_compare_exchange_strong(&(pprev->next), &(pinsert->next),
                    pinsert)) {
            break;
        } /* else pprev has been changed, start over again */
    }
    return true;
}

/* Create an endpoint of the virtual link (identified by fd) */
static int open_vl(int fd, vlendpt_t *endptr, uint64_t num_cachelines,
                   bool isproducer, enum SizeType size)
{
    StructPageNodePtr phead;
    uint8_t ctrl_init;
    if (isproducer) {
        phead = links_g[fd].prod_head_node;
        switch (size) { /* producer cacheline begins with empty (Ptr max) */
            case BYTE: ctrl_init = BYTE_CTRL_INIT; break;
            case HALF: ctrl_init = HALF_CTRL_INIT; break;
            case WORD: ctrl_init = WORD_CTRL_INIT; break;
            case TWIN: ctrl_init = TWIN_CTRL_INIT; break;
            default: ctrl_init = BYTE_CTRL_INIT;
            /* TODO: better handle illegal size */
        }
    } else {
        phead = links_g[fd].cons_head_node;
        switch (size) { /* consumer cacheline begins with empty (Ptr UV) */
            case BYTE: ctrl_init = BYTE_CTRL_UV; break;
            case HALF: ctrl_init = HALF_CTRL_UV; break;
            case WORD: ctrl_init = WORD_CTRL_UV; break;
            case TWIN: ctrl_init = TWIN_CTRL_UV; break;
            default: ctrl_init = BYTE_CTRL_UV;
            /* TODO: better handle illegal size */
        }
    }

    /* get a StructPageNode that provides enough cachelines */
    StructPageNodePtr pprev = NULL;
    StructPageNodePtr pnode = phead;
    /* after while loop, pnode holds the node to allocate cachelines */
    while (NULL != pnode) {
        uint64_t node_next = (uint64_t) pnode->next;
        uint64_t num_inactive = pnode->num_inactive;
        if (node_next & 0x1) { /* node is logically deleted */
            /* help other, trying to remove the deleted node */
            physical_delete(pnode, pprev, phead, links_g[fd].free_upon_rmvl);
            /* Note: it should be safe to access pprev here,
             * because only when pnode == phead, pprev is NULL,
             * while phead is never logically deleted. */
        } else if (num_inactive < num_cachelines) { /* no enough cachelines */
            /* move to next StructPageNode */
            pprev = pnode;
            pnode = (StructPageNodePtr) (node_next & ~0x01);
            if (NULL == pnode) { /* reaches the end of the linked list */
                /* allocate a new struct page */
                pnode = (StructPageNodePtr) memalign(2,
                        sizeof(StructPageNode));
                pnode->num_inactive = ((num_cachelines + 63) >> 6) << 6;
                pnode->base = memalign(4096, (pnode->num_inactive) << 6);
                pnode->next = NULL;
                break;
            }
            continue; /* skip the start over code at the end of while loop */
        } else { /* there are enough free cacheline in this struct page */
            /* take this struct page immediately by removing it from list */
            if (logical_delete(pnode)) {
                /* pnode has marked deleted, need to remove pnode from list */
                physical_delete(pnode, pprev, phead,
                        links_g[fd].free_upon_rmvl);
                break; /* has successfully taken pnode */
            } /* else failed to take this struct page node, start over */
        }
        /* start over from head */
        pprev = NULL;
        pnode = phead;
    }

    if (NULL == pnode) {
        printf("\033[91mERROR: pnode should never exit with NULL\033[0m\n");
        return -1;
    }

    endptr->fd = fd;
    endptr->pcacheline = (volatile void*) pnode->base;
    /* initialize cachelines one by one */
    uint64_t cl_idx;
    for (cl_idx = 0; cl_idx < num_cachelines; ++cl_idx) {
        uint64_t address = (uint64_t) pnode->base + (cl_idx << 6);
        /* address of the cacheline to initialize */
        volatile uint8_t *pbyte = (volatile uint8_t*) address;
        /* to initialize cacheline control region */
        uint64_t cl_idx_next = cl_idx + 1; /* default is the next cacheline */
        if (63 == (cl_idx % 64)) { /* every 64th $line reserved for backward */
            if (64 > cl_idx) { /* 1st cacheline is in reach of 64 */
                cl_idx_next = 0;
            } else {
                cl_idx_next = cl_idx - 64;
            }
        } else if ((num_cachelines - 1) == cl_idx) { /* last cacheline */
            if (64 > cl_idx) { /* 1st cacheline is in reach of 64 */
                cl_idx_next = 0;
            } else {
                cl_idx_next = cl_idx - (cl_idx % 64);
            }
        } else if ((num_cachelines - 2) == cl_idx) { /* last 2nd cacheline */
            cl_idx_next = cl_idx + 1;
        } else if (62 == (cl_idx % 64)) { /* cacheline needs to jump */
            cl_idx_next = cl_idx + 2;
        }
        pbyte[63] = (uint8_t) ((cl_idx_next - cl_idx) & 0x00FF);
        pbyte[62] = ctrl_init;

        if (!isproducer) { /* try to prefill consumer cachelines */
#ifdef M5VL
            m5_vl_pop((uint64_t) address, endptr->fd);
#elif NOGEM5
            sw_vl_pop((void*) address, endptr->fd);
#else
            VL_DCSVAC(address);
            VL_DCFSVAC(links_g[fd].cons_devmem);
#endif  /* end of M5VL/NOGEM5 */
        }
    }

    if (pnode->num_inactive > num_cachelines) { /* free cacheline left */
        /* make another struct page node and return it back to linked list */
        StructPageNodePtr pnew;
        if (!((uint64_t) pnode->next & 0x01)) { /* pnode is newly-allocated */
            pnew = pnode;
        } else {
            pnew = (StructPageNodePtr) memalign(2, sizeof(StructPageNode));
        }
        pnew->base = (void*)((uint64_t) pnode->base + (num_cachelines << 6));
        pnew->num_inactive = pnode->num_inactive - num_cachelines;
        insert_structpage(pnew, phead, links_g[fd].free_upon_rmvl);
    }

    return 0;
}

/* specializations of open_vl() */
int open_byte_vl_as_producer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, true, BYTE); }
int open_half_vl_as_producer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, true, HALF); }
int open_word_vl_as_producer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, true, WORD); }
int open_twin_vl_as_producer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, true, TWIN); }
int open_byte_vl_as_consumer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, false, BYTE); }
int open_half_vl_as_consumer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, false, HALF); }
int open_word_vl_as_consumer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, false, WORD); }
int open_twin_vl_as_consumer(int fd, vlendpt_t *endptr, int num_cachelines)
{ return open_vl(fd, endptr, num_cachelines, false, TWIN); }

/* Destroy an endpoint of the virtual link (identified by fd in endpt) */
static int close_vl(vlendpt_t endpt, bool isproducer)
{
    StructPageNodePtr phead;
    if (isproducer) {
        phead = links_g[endpt.fd].prod_head_node;
    } else {
        phead = links_g[endpt.fd].cons_head_node;
    }

    uint64_t endpt_pagebase = (uint64_t) endpt.pcacheline;
    uint64_t num_cachelines = 0;
    const volatile void *first_cacheline = endpt.pcacheline; /* sentinel */

    do {
        uint64_t res = 0;
        volatile uint8_t *pbyte = (volatile uint8_t*)endpt.pcacheline;
        const uint8_t ctrl_size = pbyte[62] >> 6;
        const uint8_t ctrl_ptr = pbyte[62] & 0x3F;
        const int8_t ptr_size = (0x01 << ctrl_size); /* in byte */
        const int8_t max_valid = 62 - (62 % ptr_size);
        int8_t num_valid; /* in byte */
        if (!isproducer) {
            /* shift the cacheline to be producer-like */
            uint8_t left_shift; /* in byte */
            num_valid = ValidCons(ctrl_ptr, ptr_size, max_valid);
            pbyte[62] = (ctrl_size << 6) |
                ((max_valid - num_valid - ptr_size) & 0x3F);
            left_shift = max_valid - num_valid;
            if (left_shift) { /* full cacheline needs no shift */
                int8_t tmp;
                /* shift data region */
                for (tmp = num_valid; 0 < tmp; --tmp) {
                    pbyte[tmp + left_shift] = pbyte[tmp];
                }
            }
        } else {
            num_valid = ValidProd(ctrl_ptr, ptr_size, max_valid);
        }
        if (num_valid) { /* copy over if has valid data in the cacheline */
            do {
#ifdef M5VL
                res = m5_vl_push((uint64_t) endpt.pcacheline, endpt.fd);
#elif NOGEM5
                res = sw_vl_push(endpt.pcacheline, endpt.fd);
#else
                VL_DCSVAC(endpt.pcacheline);
                VL_DCPSVAC(res, links_g[endpt.fd].prod_devmem);
#endif  /* end of M5VL/NOGEM5 */
            } while (res & 0x8000000000000000);
        } else if (!isproducer) {
            /* TODO: a better way, invalidate the cacheline or clear its
             * pushable flag so that the pull requests on fly will not
             * get valid data stuck in a discarded cacheline */
            /* With cache check Ctrl to avoid overwritten, BYTE_CTRL_INIT
             * would refuse further data delivered to this cacheline */
            pbyte[62] = BYTE_CTRL_INIT; /* empty at all granularity */
#ifdef M5VL
            res = m5_vl_push((uint64_t) endpt.pcacheline, endpt.fd);
#elif NOGEM5
            res = sw_vl_push(endpt.pcacheline, endpt.fd);
#else
            VL_DCSVAC(endpt.pcacheline);
            VL_DCPSVAC(res, links_g[endpt.fd].prod_devmem);
#endif  /* end of M5VL */
        }

        VL_Next(&endpt.pcacheline);
        num_cachelines++;
        if (endpt_pagebase > (uint64_t) endpt.pcacheline) {
            endpt_pagebase = (uint64_t) endpt.pcacheline;
        }
    } while (first_cacheline != endpt.pcacheline);

    /* construct a new struct page node to insert into linked list */
    StructPageNodePtr preturn = (StructPageNodePtr) memalign(2,
            sizeof(StructPageNode));
    preturn->base = (StructPageNodePtr) endpt_pagebase;
    preturn->num_inactive = num_cachelines;
    insert_structpage(preturn, phead, links_g[endpt.fd].free_upon_rmvl);

    return 0;
}

/* specializations of close_vl() */
int close_byte_vl_as_producer(vlendpt_t endpt)
{ return close_vl(endpt, true); }
int close_half_vl_as_producer(vlendpt_t endpt)
{ return close_vl(endpt, true); }
int close_word_vl_as_producer(vlendpt_t endpt)
{ return close_vl(endpt, true); }
int close_twin_vl_as_producer(vlendpt_t endpt)
{ return close_vl(endpt, true); }
int close_byte_vl_as_consumer(vlendpt_t endpt)
{ return close_vl(endpt, false); }
int close_half_vl_as_consumer(vlendpt_t endpt)
{ return close_vl(endpt, false); }
int close_word_vl_as_consumer(vlendpt_t endpt)
{ return close_vl(endpt, false); }
int close_twin_vl_as_consumer(vlendpt_t endpt)
{ return close_vl(endpt, false); }

/* Wrapper of ENQ */
static bool vl_push(vlendpt_t *endptr, uint64_t data,
                    bool isblocking, bool isstrong, enum SizeType size)
{
    uint64_t res = 0;
    volatile void const *first_cacheline = endptr->pcacheline;
    bool tried_all = false;
    do {
        switch (size) {
            case BYTE: VL_ENQB(&res, data, endptr->pcacheline); break;
            case HALF: VL_ENQH(&res, data, endptr->pcacheline); break;
            case WORD: VL_ENQ32(&res, data, endptr->pcacheline); break;
            case TWIN: VL_ENQ64(&res, data, endptr->pcacheline); break;
        }
        bool full = res & 0x0000000000000001;
        bool fail = res & 0x0000000000000002;
        if (full) {
            /* first try to copy over then try to get an empty cacheline */
            bool isempty = false;
            do {
#ifdef M5VL
                res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
                res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
                VL_DCSVAC(endptr->pcacheline);
                VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif  /* end of M5VL/NOGEM5 */
                if (!(res & 0x8000000000000000)) { /* copy over succeed */
                    /* reinitialize current cacheline control region */
                    volatile uint8_t *pbyte =
                        (volatile uint8_t*)(endptr->pcacheline);
                    switch (size) {
                        case BYTE: pbyte[62] = BYTE_CTRL_INIT; break;
                        case HALF: pbyte[62] = HALF_CTRL_INIT; break;
                        case WORD: pbyte[62] = WORD_CTRL_INIT; break;
                        case TWIN: pbyte[62] = TWIN_CTRL_INIT; break;
                    }
                }
                if (isstrong) {
                    isempty = !(res & 0x8000000000000000);
                } else { /* _weak always goes to next cacheline */
                    VL_Next(&endptr->pcacheline);
                    volatile uint8_t *pbyte =
                      (volatile uint8_t*)(endptr->pcacheline);
                    switch (size) {
                        case BYTE:
                            isempty = (BYTE_CTRL_INIT == pbyte[62]); break;
                        case HALF:
                            isempty = (HALF_CTRL_INIT == pbyte[62]); break;
                        case WORD:
                            isempty = (WORD_CTRL_INIT == pbyte[62]); break;
                        case TWIN:
                            isempty = (TWIN_CTRL_INIT == pbyte[62]); break;
                    }
                    tried_all = first_cacheline == endptr->pcacheline;
                }
            } while (!(isempty || tried_all));
            if (!fail || (!isblocking && tried_all)) {
                return !fail;
            } /* else must because fail, needs to loop back and retry ENQ */
        } else { /* not full means must have succeeded */
            return true;
        }
    } while (true);

    return false; /* should never reach here */
}

/* specializations of vl_push() */
/* non-blocking only guarantees that data is committed,
 * while a filled-up cacheline would not trigger copy over,
 * so may not get prepared for next push */
bool byte_vl_push_non(vlendpt_t *endptr, uint8_t data)
{ return vl_push(endptr, data, false, false, BYTE); }
bool half_vl_push_non(vlendpt_t *endptr, uint16_t data)
{ return vl_push(endptr, data, false, false, HALF); }
bool word_vl_push_non(vlendpt_t *endptr, uint32_t data)
{ return vl_push(endptr, data, false, false, WORD); }
bool twin_vl_push_non(vlendpt_t *endptr, uint64_t data)
{ return vl_push(endptr, data, false, false, TWIN); }
/* weak-blocking makes sure cacheline is copied over if full,
 * and no matter the copy over of this cacheline succeed or not,
 * always move to next empty cacheline in the endpoint,
 * so it could make use of multi-cacheline, while less FIFO guarantee */
void byte_vl_push_weak(vlendpt_t *endptr, uint8_t data)
{ vl_push(endptr, data, true, false, BYTE); }
void half_vl_push_weak(vlendpt_t *endptr, uint16_t data)
{ vl_push(endptr, data, true, false, HALF); }
void word_vl_push_weak(vlendpt_t *endptr, uint32_t data)
{ vl_push(endptr, data, true, false, WORD); }
void twin_vl_push_weak(vlendpt_t *endptr, uint64_t data)
{ vl_push(endptr, data, true, false, TWIN); }
/* strong-blocking spins on copying over current cacheline if full,
 * so it should not use with multi-cacheline, but guarantees stronger FIFO */
void byte_vl_push_strong(vlendpt_t *endptr, uint8_t data)
{ vl_push(endptr, data, true, true, BYTE); }
void half_vl_push_strong(vlendpt_t *endptr, uint16_t data)
{ vl_push(endptr, data, true, true, HALF); }
void word_vl_push_strong(vlendpt_t *endptr, uint32_t data)
{ vl_push(endptr, data, true, true, WORD); }
void twin_vl_push_strong(vlendpt_t *endptr, uint64_t data)
{ vl_push(endptr, data, true, true, TWIN); }

bool line_vl_push_non(vlendpt_t *endptr, uint8_t *pdata, size_t cnt)
{
    /* assert(cnt < 62) */
    uint64_t res = 0;
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    volatile void const *first_cacheline = endptr->pcacheline;
    while (BYTE_CTRL_INIT != pbyte[62]) { /* not empty */
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif
        if (!(res & 0x8000000000000000)) { /* push succeed */
            pbyte[62] = BYTE_CTRL_INIT;
        }
        VL_Next(&endptr->pcacheline);
        pbyte = (volatile uint8_t*)(endptr->pcacheline);
        if (first_cacheline == pbyte) { /* tried all cachelines */
            break;
        }
    }
    if (BYTE_CTRL_INIT == pbyte[62]) { /* empty */
        size_t idx = 0;
        for (; idx < cnt; ++idx) {
            pbyte[61 - idx] = pdata[idx];
        }
        pbyte[62] = (61 - cnt) & 0x3F;
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif
        if (!(res & 0x8000000000000000)) { /* push succeed */
            pbyte[62] = BYTE_CTRL_INIT;
        }
        VL_Next(&endptr->pcacheline);
        return true;
    }
    return false;
}

void line_vl_push_weak(vlendpt_t *endptr, uint8_t *pdata, size_t cnt)
{
    /* assert(cnt < 62) */
    uint64_t res = 0;
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    while (BYTE_CTRL_INIT != pbyte[62]) { /* not empty */
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif
        if (!(res & 0x8000000000000000)) { /* push succeed */
            pbyte[62] = BYTE_CTRL_INIT;
        }
        VL_Next(&endptr->pcacheline);
        pbyte = (volatile uint8_t*)(endptr->pcacheline);
    }
    size_t idx = 0;
    for (; idx < cnt; ++idx) {
        pbyte[61 - idx] = pdata[idx];
    }
    pbyte[62] = (61 - cnt) & 0x3F;
    do {
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif
        if (!(res & 0x8000000000000000)) { /* push succeed */
            pbyte[62] = BYTE_CTRL_INIT;
        }
        VL_Next(&endptr->pcacheline);
        pbyte = (volatile uint8_t*)(endptr->pcacheline);
    } while (BYTE_CTRL_INIT != pbyte[62]); /* not empty */
}

void line_vl_push_strong(vlendpt_t *endptr, uint8_t *pdata, size_t cnt)
{
    /* assert(cnt < 62) */
    uint64_t res = 0;
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    while (BYTE_CTRL_INIT != pbyte[62]) { /* not empty */
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif
        if (!(res & 0x8000000000000000)) { /* push succeed */
            pbyte[62] = BYTE_CTRL_INIT;
        }
    }
    size_t idx = 0;
    for (; idx < cnt; ++idx) {
        pbyte[61 - idx] = pdata[idx];
    }
    pbyte[62] = (61 - cnt) & 0x3F;
    do {
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif
        if (!(res & 0x8000000000000000)) { /* push succeed */
            pbyte[62] = BYTE_CTRL_INIT;
        }
    } while (BYTE_CTRL_INIT != pbyte[62]); /* not empty */
}

/* Wrapper of DEQ */
static void vl_pop(vlendpt_t *endptr, void *pdata, bool *isvalid,
                   bool isblocking, bool isstrong, enum SizeType size)
{
    uint64_t res = 0;
    /* TODO: make FSVAC use MSHR, so HW prevents cacheline fill hazard */
    do {
        switch (size) {
            case BYTE:
                VL_DEQB(&res, (uint8_t*)pdata, endptr->pcacheline); break;
            case HALF:
                VL_DEQH(&res, (uint16_t*)pdata, endptr->pcacheline); break;
            case WORD:
                VL_DEQ32(&res, (uint32_t*)pdata, endptr->pcacheline); break;
            case TWIN:
                VL_DEQ64(&res, (uint64_t*)pdata, endptr->pcacheline); break;
        }
        bool fail = res & 0x0000000000000002;
        if (fail) {
            volatile uint8_t *pbyte;
            pbyte = (volatile uint8_t*)(endptr->pcacheline);
            /* read Nx (Byte63) and write back, force cacheline writable */
            __asm__ volatile (
                "ldrb  w8,     [%[cva], #63]        \n\r"
                "strb  w8,     [%[cva], #63]        \n\r"
                :
                : [cva]"r" (pbyte)
                : "x8", "memory"
                );
            /* TODO: what if context switch happens right after strb */
#ifdef M5VL
            m5_vl_pop((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
            sw_vl_pop(endptr->pcacheline, endptr->fd);
#else
            VL_DCSVAC(endptr->pcacheline);
            VL_DCFSVAC(links_g[endptr->fd].cons_devmem);
#endif  /* end of M5VL/NOGEM5 */
            if (!isstrong) { /* _weak always goes to next cacheline */
                VL_Next(&endptr->pcacheline);
            }
            if (!isblocking) { /* must because fail */
                *isvalid = false;
                break;
            }
        } else { /* get valid data */
            *isvalid = true;
            break;
        }
    } while (true);
}

/* specializations of vl_pop() */
/* nonblocking does not guarantee that data is valid,
 * but return an indicator to let user application know whether data valid */
void byte_vl_pop_non(vlendpt_t *endptr, uint8_t *pdata, bool *pvalid)
{ vl_pop(endptr, (void*)pdata, pvalid, false, false, BYTE); }
void half_vl_pop_non(vlendpt_t *endptr, uint16_t *pdata, bool *pvalid)
{ vl_pop(endptr, (void*)pdata, pvalid, false, false, HALF); }
void word_vl_pop_non(vlendpt_t *endptr, uint32_t *pdata, bool *pvalid)
{ vl_pop(endptr, (void*)pdata, pvalid, false, false, WORD); }
void twin_vl_pop_non(vlendpt_t *endptr, uint64_t *pdata, bool *pvalid)
{ vl_pop(endptr, (void*)pdata, pvalid, false, false, TWIN); }
/* weak-blocking refills empty cacheline when no valid data pop,
 * and no matter the refilling of the current cacheline succeed or not,
 * move to next cacheline until get a valid data (could stuck forever),
 * so it could make use of multi-cacheline but less guarantee on FIFO */
void byte_vl_pop_weak(vlendpt_t *endptr, uint8_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, false, BYTE);
    } while(!isvalid);
}
void half_vl_pop_weak(vlendpt_t *endptr, uint16_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, false, HALF);
    } while(!isvalid);
}
void word_vl_pop_weak(vlendpt_t *endptr, uint32_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, false, WORD);
    } while(!isvalid);
}
void twin_vl_pop_weak(vlendpt_t *endptr, uint64_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, false, TWIN);
    } while(!isvalid);
}
/* strong-blocking spinning on refilling current empty cacheline,
 * until get a valid data (could struck forever),
 * so it should not use with multi-cacheline but guarantees stronger FIFO */
void byte_vl_pop_strong(vlendpt_t *endptr, uint8_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, true, BYTE);
    } while(!isvalid);
}
void half_vl_pop_strong(vlendpt_t *endptr, uint16_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, true, HALF);
    } while(!isvalid);
}
void word_vl_pop_strong(vlendpt_t *endptr, uint32_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, true, WORD);
    } while(!isvalid);
}
void twin_vl_pop_strong(vlendpt_t *endptr, uint64_t *pdata)
{
    bool isvalid;
    do {
        vl_pop(endptr, (void*)pdata, &isvalid, true, true, TWIN);
    } while(!isvalid);
}

void line_vl_pop_non(vlendpt_t *endptr, uint8_t *pdata, size_t *pcnt)
{
    volatile uint8_t* pbyte = endptr->pcacheline;
    if (pbyte[62] == BYTE_CTRL_UV) { /* empty */
        /* read Nx (Byte63) and write back, force cacheline writable */
        __asm__ volatile (
            "ldrb  w8,     [%[cva], #63]        \n\r"
            "strb  w8,     [%[cva], #63]        \n\r"
            :
            : [cva]"r" (pbyte)
            : "x8", "memory"
            );
#ifdef M5VL
        m5_vl_pop((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        sw_vl_pop(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCFSVAC(links_g[endptr->fd].cons_devmem);
#endif
        *pcnt = 0;
    } else {
        const size_t byte_valid = (pbyte[62] + 1) & 0x3f;
        *pcnt = (*pcnt < byte_valid) ? *pcnt : byte_valid;
        size_t idx = *pcnt;
        const size_t idx_max = byte_valid - 1;
        for (; 0 < idx--;) {
            pdata[idx] = pbyte[idx_max - idx];
        }
        pbyte[62] = (pbyte[62] - *pcnt) & 0x3f;
    }
    if (pbyte[62] == BYTE_CTRL_UV) {
        VL_Next(&endptr->pcacheline);
    }
}

void line_vl_pop_weak(vlendpt_t *endptr, uint8_t *pdata, size_t *pcnt)
{
    volatile uint8_t* pbyte = endptr->pcacheline;
    while (pbyte[62] == BYTE_CTRL_UV) { /* empty */
        /* read Nx (Byte63) and write back, force cacheline writable */
        __asm__ volatile (
            "ldrb  w8,     [%[cva], #63]        \n\r"
            "strb  w8,     [%[cva], #63]        \n\r"
            :
            : [cva]"r" (pbyte)
            : "x8", "memory"
            );
#ifdef M5VL
        m5_vl_pop((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        sw_vl_pop(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCFSVAC(links_g[endptr->fd].cons_devmem);
#endif
        VL_Next(&endptr->pcacheline);
        pbyte = (volatile uint8_t*) endptr->pcacheline;
    }
    const size_t byte_valid = (pbyte[62] + 1) & 0x3f;
    size_t idx = 0;
    for (; idx < byte_valid; ++idx) {
        pdata[byte_valid - idx - 1] = pbyte[idx];
    }
    pbyte[62] = BYTE_CTRL_UV;
    *pcnt = idx;
}

void line_vl_pop_strong(vlendpt_t *endptr, uint8_t *pdata, size_t *pcnt)
{
    volatile uint8_t* pbyte = endptr->pcacheline;
    while (pbyte[62] == BYTE_CTRL_UV) { /* empty */
        /* read Nx (Byte63) and write back, force cacheline writable */
        __asm__ volatile (
            "ldrb  w8,     [%[cva], #63]        \n\r"
            "strb  w8,     [%[cva], #63]        \n\r"
            :
            : [cva]"r" (pbyte)
            : "x8", "memory"
            );
#ifdef M5VL
        m5_vl_pop((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        sw_vl_pop(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCFSVAC(links_g[endptr->fd].cons_devmem);
#endif
    }
    const size_t byte_valid = (pbyte[62] + 1) & 0x3f;
    size_t idx = 0;
    for (; idx < byte_valid; ++idx) {
        pdata[byte_valid - idx - 1] = pbyte[idx];
    }
    pbyte[62] = BYTE_CTRL_UV;
    *pcnt = idx;
}

/* Wrapper of VL_DCPSVAC */
static void vl_flush(vlendpt_t *endptr, enum SizeType size)
{
    uint64_t res = 0;
    do {
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif  /* end of M5VL/NOGEM5 */
        if (!(res & 0x8000000000000000)) { /* copy over succeed */
            /* reinitialize current cacheline control region */
            volatile uint8_t *pbyte =
                (volatile uint8_t*)(endptr->pcacheline);
            switch (size) {
                case BYTE: pbyte[62] = BYTE_CTRL_INIT; break;
                case HALF: pbyte[62] = HALF_CTRL_INIT; break;
                case WORD: pbyte[62] = WORD_CTRL_INIT; break;
                case TWIN: pbyte[62] = TWIN_CTRL_INIT; break;
            }
        }
    } while ((res & 0x8000000000000000));
}
/* specializations of vl_flush() */
void byte_vl_flush(vlendpt_t *endptr)
{ vl_flush(endptr, BYTE); }
void half_vl_flush(vlendpt_t *endptr)
{ vl_flush(endptr, HALF); }
void word_vl_flush(vlendpt_t *endptr)
{ vl_flush(endptr, WORD); }
void twin_vl_flush(vlendpt_t *endptr)
{ vl_flush(endptr, TWIN); }

/* Query how many producer cachelines are buffered in routing device */
uint64_t vl_size(const vlendpt_t *endptr)
{
    static __thread uint8_t empty_line[64] CACHE_ALIGNED;
    uint64_t res;
    empty_line[62] = BYTE_CTRL_INIT;
#ifdef M5VL
    res = m5_vl_push((uint64_t) empty_line, endptr->fd);
#elif NOGEM5
    res = sw_vl_push(empty_line, endptr->fd);
#else
    VL_DCSVAC(empty_line);
    VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif  /* end of M5VL */
    return res & 0x7FFFFFFFFFFFFFFF;
}

/* Allocate the slot to store a data */
void* vl_allocate(vlendpt_t *endptr, size_t cnt)
{
    uint64_t res = 0;
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    volatile void const *first_cacheline = endptr->pcacheline;
    while ((BYTE_CTRL_INIT < pbyte[62]) || (pbyte[62] < (cnt - 1))) {
#ifdef M5VL
        res = m5_vl_push((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        res = sw_vl_push(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCPSVAC(res, links_g[endptr->fd].prod_devmem);
#endif
        if (!(res & 0x8000000000000000)) { /* push succeed */
            pbyte[62] = BYTE_CTRL_INIT - cnt;
            return (void*)(&pbyte[(pbyte[62] + 1) & 0x0FF]);
        }
        VL_Next(&endptr->pcacheline);
        pbyte = (volatile uint8_t*)(endptr->pcacheline);
        if (first_cacheline == pbyte) { /* tried all cachelines */
            return NULL;
        }
    }
    pbyte[62] -= cnt;
    return ((void*)(endptr->pcacheline + pbyte[62] + 1));
}

/* Reverse the last allocation */
void vl_deallocate(vlendpt_t *endptr, size_t cnt)
{
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    pbyte[62] += cnt;
}

/* Send out a producer cacheline if it is filled up */
void vl_send(vlendpt_t *endptr, size_t cnt)
{
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    if ((uint8_t)cnt <= (BYTE_CTRL_INIT - pbyte[62]) ||
        BYTE_CTRL_UV == pbyte[62]) {
        byte_vl_flush(endptr);
    }
}

/* Return the slot at the front of the current consumer cacheline */
void* vl_peek(vlendpt_t *endptr, size_t cnt, bool isblocking)
{
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    while (pbyte[62] == BYTE_CTRL_UV) { /* empty */
        /* read Nx (Byte63) and write back, force cacheline writable */
        __asm__ volatile (
            "ldrb  w8,     [%[cva], #63]        \n\r"
            "strb  w8,     [%[cva], #63]        \n\r"
            :
            : [cva]"r" (pbyte)
            : "x8", "memory"
            );
#ifdef M5VL
        m5_vl_pop((uint64_t) endptr->pcacheline, endptr->fd);
#elif NOGEM5
        sw_vl_pop(endptr->pcacheline, endptr->fd);
#else
        VL_DCSVAC(endptr->pcacheline);
        VL_DCFSVAC(links_g[endptr->fd].cons_devmem);
#endif
        if (!isblocking) {
            return NULL;
        }
    }
    return ((void*)(endptr->pcacheline + pbyte[62] - cnt + 1));
}

/* Free the front slot of the current consumer cacheline */
void vl_recycle(vlendpt_t *endptr, size_t cnt)
{
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    const uint8_t ctrl_ptr = pbyte[62] & 0x3F;
    pbyte[62] = ((uint8_t)cnt > ctrl_ptr) ? BYTE_CTRL_UV : ctrl_ptr - cnt;
}

/* Return the number of data elements in the current producer cacheline */
size_t vl_producer_size(vlendpt_t *endptr, size_t cnt)
{
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    if (0 == cnt) { /* let the function figure out by looking at Ctrl region */
        const uint8_t ctrl_size = pbyte[62] >> 6;
        const uint8_t ctrl_ptr = pbyte[62] & 0x3F;
        const int8_t ptr_size = (0x01 << ctrl_size); /* in byte */
        const int8_t max_valid = 62 - (62 % ptr_size);
        return ValidProd(ctrl_ptr, ptr_size, max_valid);
    } else {
        return ((size_t)((61 - pbyte[62]) / cnt));
    }
}

/* Return the number of data elements in the current consumer cacheline */
size_t vl_consumer_size(vlendpt_t *endptr, size_t cnt)
{
    volatile uint8_t *pbyte = (volatile uint8_t*)(endptr->pcacheline);
    if (0 == cnt) { /* let the function figure out by looking at Ctrl region */
        const uint8_t ctrl_size = pbyte[62] >> 6;
        const uint8_t ctrl_ptr = pbyte[62] & 0x3F;
        const int8_t ptr_size = (0x01 << ctrl_size); /* in byte */
        const int8_t max_valid = 62 - (62 % ptr_size);
        return ValidCons(ctrl_ptr, ptr_size, max_valid);
    } else {
        return (BYTE_CTRL_UV == pbyte[62]) ? \
            0 : ((size_t)((pbyte[62] + 1) / cnt));
    }
}
