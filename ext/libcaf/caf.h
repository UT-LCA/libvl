#ifndef	_LIBCAF_H
#define	_LIBCAF_H	1
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CAFEndpoint_s {
    volatile void *pqmd; /* points to QMD */
    int fd; /* file descriptor (id) of the CAF queue */
} cafendpt_t;

/* Create endpoints */
extern int open_caf(int fd, cafendpt_t *endptr) __THROW;

/* Destroy endpoint */
extern int close_caf(cafendpt_t endpt) __THROW;

/* Push data into an endpoint */
extern bool caf_push_non(cafendpt_t *endptr, uint64_t data) __THROW;
extern void caf_push_strong(cafendpt_t *endptr, uint64_t data) __THROW;
extern size_t caf_push_bulk(cafendpt_t *endptr, uint64_t *pdata, size_t cnt)
    __THROW;

/* Pop data from an endpoint */
extern bool caf_pop_non(cafendpt_t *endptr, uint64_t *pdata) __THROW;
extern void caf_pop_strong(cafendpt_t *endptr, uint64_t *pdata) __THROW;
extern size_t caf_pop_bulk(cafendpt_t *endptr, uint64_t *pdata, size_t cnt)
    __THROW;

/* Prepush cachelines */
extern void caf_prepush(void *pdata, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _LIBCAF_H */
