#ifndef _THREADING_H__
#define _THREADING_H__  1

#ifndef __USE_GNU
#define __USE_GNU
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>

#define BUFFER_LENGTH 1000

#ifdef __cplusplus
extern "C" {
#endif

extern void setAffinity(const int desired_core);

#ifdef __cplusplus
}
#endif

#endif /* END _THREADING_H__ */
