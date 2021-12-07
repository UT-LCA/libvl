#ifndef	_LIBVL_H
#define	_LIBVL_H	1
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Endpoint_s {
    volatile void *pcacheline; /* points to current cachline of the endpoint */
    int fd; /* file descriptor of the virtual link this endpoint belongs to */
} vlendpt_t;

/* Create a new Virtual Link.  */
extern int mkvl (void) __THROW;

/* Create endpoints */
extern int open_byte_vl_as_producer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;
extern int open_half_vl_as_producer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;
extern int open_word_vl_as_producer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;
extern int open_twin_vl_as_producer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;
extern int open_byte_vl_as_consumer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;
extern int open_half_vl_as_consumer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;
extern int open_word_vl_as_consumer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;
extern int open_twin_vl_as_consumer(int fd, vlendpt_t *endptr,
        int num_cachelines) __THROW;

/* Destroy endpoint */
extern int close_byte_vl_as_producer(vlendpt_t endpt) __THROW;
extern int close_half_vl_as_producer(vlendpt_t endpt) __THROW;
extern int close_word_vl_as_producer(vlendpt_t endpt) __THROW;
extern int close_twin_vl_as_producer(vlendpt_t endpt) __THROW;
extern int close_byte_vl_as_consumer(vlendpt_t endpt) __THROW;
extern int close_half_vl_as_consumer(vlendpt_t endpt) __THROW;
extern int close_word_vl_as_consumer(vlendpt_t endpt) __THROW;
extern int close_twin_vl_as_consumer(vlendpt_t endpt) __THROW;

/* Push data into an endpoint */
/* non-blocking will try to commit the data but may return false if failed,
 * a filled-up cacheline would trigger copy over */
/* weak-blocking guarantees that data is committed,
 * triggers cacheline copy over if full,
 * and no matter the copy over of this cacheline succeed or not,
 * always try moving to the next empty cacheline in the endpoint,
 * so it could make use of multi-cacheline, while less FIFO guarantee */
/* strong-blocking spins on copying over current cacheline if full,
 * so it should not use with multi-cacheline, but guarantees stronger FIFO */
extern bool byte_vl_push_non(vlendpt_t *endptr, uint8_t data) __THROW;
extern bool half_vl_push_non(vlendpt_t *endptr, uint16_t data) __THROW;
extern bool word_vl_push_non(vlendpt_t *endptr, uint32_t data) __THROW;
extern bool twin_vl_push_non(vlendpt_t *endptr, uint64_t data) __THROW;
extern bool line_vl_push_non(vlendpt_t *endptr, uint8_t *pdata,
        size_t cnt) __THROW;
extern void byte_vl_push_weak(vlendpt_t *endptr, uint8_t data) __THROW;
extern void half_vl_push_weak(vlendpt_t *endptr, uint16_t data) __THROW;
extern void word_vl_push_weak(vlendpt_t *endptr, uint32_t data) __THROW;
extern void twin_vl_push_weak(vlendpt_t *endptr, uint64_t data) __THROW;
extern void line_vl_push_weak(vlendpt_t *endptr, uint8_t *pdata,
        size_t cnt) __THROW;
extern void byte_vl_push_strong(vlendpt_t *endptr, uint8_t data) __THROW;
extern void half_vl_push_strong(vlendpt_t *endptr, uint16_t data) __THROW;
extern void word_vl_push_strong(vlendpt_t *endptr, uint32_t data) __THROW;
extern void twin_vl_push_strong(vlendpt_t *endptr, uint64_t data) __THROW;
extern void line_vl_push_strong(vlendpt_t *endptr, uint8_t *pdata,
        size_t cnt) __THROW;

/* Pop data from an endpoint */
/* nonblocking does not guarantee that data is valid,
 * but return an indicator to let user application know whether data valid */
/* weak-blocking refills empty cacheline when no valid data pop,
 * and no matter the refilling of the current cacheline succeed or not,
 * move to next cacheline until get a valid data (could stuck forever),
 * so it could make use of multi-cacheline but less guarantee on FIFO */
/* strong-blocking spinning on refilling current empty cacheline,
 * until get a valid data (could struck forever),
 * so it should not use with multi-cacheline but guarantees stronger FIFO */
extern void byte_vl_pop_non(vlendpt_t *endptr, uint8_t *pdata,
        bool *pvalid) __THROW;
extern void half_vl_pop_non(vlendpt_t *endptr, uint16_t *pdata,
        bool *pvalid) __THROW;
extern void word_vl_pop_non(vlendpt_t *endptr, uint32_t *pdata,
        bool *pvalid) __THROW;
extern void twin_vl_pop_non(vlendpt_t *endptr, uint64_t *pdata,
        bool *pvalid) __THROW;
extern void line_vl_pop_non(vlendpt_t *endptr, uint8_t *pdata,
        size_t *pcnt) __THROW;
extern void byte_vl_pop_weak(vlendpt_t *endptr, uint8_t *pdata) __THROW;
extern void half_vl_pop_weak(vlendpt_t *endptr, uint16_t *pdata) __THROW;
extern void word_vl_pop_weak(vlendpt_t *endptr, uint32_t *pdata) __THROW;
extern void twin_vl_pop_weak(vlendpt_t *endptr, uint64_t *pdata) __THROW;
extern void line_vl_pop_weak(vlendpt_t *endptr, uint8_t *pdata,
        size_t *pcnt) __THROW;
extern void byte_vl_pop_strong(vlendpt_t *endptr, uint8_t *pdata) __THROW;
extern void half_vl_pop_strong(vlendpt_t *endptr, uint16_t *pdata) __THROW;
extern void word_vl_pop_strong(vlendpt_t *endptr, uint32_t *pdata) __THROW;
extern void twin_vl_pop_strong(vlendpt_t *endptr, uint64_t *pdata) __THROW;
extern void line_vl_pop_strong(vlendpt_t *endptr, uint8_t *pdata,
        size_t *pcnt) __THROW;

/* Flush local cacheline, force writeback */
extern void byte_vl_flush(vlendpt_t *endptr) __THROW;
extern void half_vl_flush(vlendpt_t *endptr) __THROW;
extern void word_vl_flush(vlendpt_t *endptr) __THROW;
extern void twin_vl_flush(vlendpt_t *endptr) __THROW;

/* Query how many producer cachelines are buffered in routing device */
extern uint64_t vl_size(const vlendpt_t *endptr) __THROW;

/* Endpoint cacheline operations exposed for Raftlib */
extern void* vl_allocate(vlendpt_t *endptr, size_t cnt) __THROW;
extern void vl_deallocate(vlendpt_t *endptr, size_t cnt) __THROW;
extern void vl_send(vlendpt_t *endptr, size_t cnt) __THROW;
extern void* vl_peek(vlendpt_t *endptr, size_t cnt, bool isblocking) __THROW;
extern void vl_recycle(vlendpt_t *endptr, size_t cnt) __THROW;
extern size_t vl_producer_size(vlendpt_t *endptr, size_t cnt) __THROW;
extern size_t vl_consumer_size(vlendpt_t *endptr, size_t cnt) __THROW;

#ifdef __cplusplus
}
#endif

#endif /* _LIBVL_H */
