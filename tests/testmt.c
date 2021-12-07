#include <stdio.h>
#include <vl.h>
#include "threading.h"

#define NUM_PINGPONG 512

char *testname;
int fd0, fd1;

void *echo(void *arg) {
  int *desired_core = (int*)arg;
  setAffinity(*desired_core);

  int errorcode, cnt;
  vlendpt_t recv, send;
  uint64_t val64;

  if ((errorcode = open_twin_vl_as_consumer(fd0, &recv, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }
  if ((errorcode = open_twin_vl_as_producer(fd1, &send, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }

  for (cnt = 0; NUM_PINGPONG > cnt; ++cnt) {
    twin_vl_pop_weak(&recv, &val64);
    twin_vl_push_strong(&send, val64);
    twin_vl_pop_weak(&recv, &val64);
    twin_vl_push_strong(&send, val64);
    twin_vl_pop_weak(&recv, &val64);
    twin_vl_push_strong(&send, val64);
    twin_vl_pop_weak(&recv, &val64);
    twin_vl_push_strong(&send, val64);
    twin_vl_pop_weak(&recv, &val64);
    twin_vl_push_strong(&send, val64);
    twin_vl_pop_weak(&recv, &val64);
    twin_vl_push_strong(&send, val64);
    twin_vl_pop_weak(&recv, &val64);
    twin_vl_push_strong(&send, val64);
  }

  if ((errorcode = close_twin_vl_as_consumer(recv))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }
  if ((errorcode = close_twin_vl_as_producer(send))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }

  return NULL;
}

int main(int argc, char* argv[]) {
  setAffinity(0);
  testname = argv[0];
  fd0 = mkvl();
  if (1 > argc) { /* just to get rid of unused warning */
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }
  if (0 > fd0) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }
  fd1 = mkvl();
  if (0 > fd1) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }
  vlendpt_t send, recv;
  int errorcode, cnt;
  uint64_t val64 = 0;
  pthread_t threads[2];
  int core[2] = {1, 2};

  if ((errorcode = open_twin_vl_as_producer(fd0, &send, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_consumer(fd1, &recv, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }

  if ((errorcode = pthread_create(&threads[0], NULL, echo, (void*)&core[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }

  for (cnt = 0; cnt < NUM_PINGPONG; ++cnt) {
    twin_vl_push_strong(&send, (uint64_t)(cnt * 7));
    twin_vl_push_strong(&send, (uint64_t)(cnt * 7 + 1));
    twin_vl_push_strong(&send, (uint64_t)(cnt * 7 + 2));
    twin_vl_push_strong(&send, (uint64_t)(cnt * 7 + 3));
    twin_vl_push_strong(&send, (uint64_t)(cnt * 7 + 4));
    twin_vl_push_strong(&send, (uint64_t)(cnt * 7 + 5));
    twin_vl_push_strong(&send, (uint64_t)(cnt * 7 + 6));
    twin_vl_pop_weak(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    twin_vl_pop_weak(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 1)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    twin_vl_pop_weak(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 2)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    twin_vl_pop_weak(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 3)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    twin_vl_pop_weak(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 4)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    twin_vl_pop_weak(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 5)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    twin_vl_pop_weak(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 6)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
  }

  pthread_join(threads[0], NULL);

  if ((errorcode = pthread_create(&threads[0], NULL, echo, (void*)&core[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }
  if ((errorcode = pthread_create(&threads[1], NULL, echo, (void*)&core[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }

  for (cnt = 0; cnt < NUM_PINGPONG; ++cnt) {
    int i; /* burst offset */
    for (i = 0; i < 14; ++i) {
      twin_vl_push_strong(&send, (uint64_t)cnt);
    }
    for (i = 0; i < 14; ++i) {
      twin_vl_pop_weak(&recv, &val64);
      if (val64 != (uint64_t)cnt) {
        printf("\033[91mFAILED:\033[0m %s\n", testname);
        return 0;
      }
    }
  }

  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
