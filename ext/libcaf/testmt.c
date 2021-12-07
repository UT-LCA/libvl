#include <stdio.h>
#include <caf.h>
#include "../../tests/threading.h"

#define NUM_PINGPONG 512
#define PING_QID 1
#define PONG_QID 2

char *testname;

void *echo(void *arg) {
  int *desired_core = (int*)arg;
  setAffinity(*desired_core);

  int errorcode, cnt;
  cafendpt_t recv, send;
  uint64_t val64;

  if ((errorcode = open_caf(PING_QID, &recv))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }
  if ((errorcode = open_caf(PONG_QID, &send))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }

  for (cnt = 0; NUM_PINGPONG > cnt; ++cnt) {
    caf_pop_strong(&recv, &val64);
    caf_push_strong(&send, val64);
    caf_pop_strong(&recv, &val64);
    caf_push_strong(&send, val64);
    caf_pop_strong(&recv, &val64);
    caf_push_strong(&send, val64);
    caf_pop_strong(&recv, &val64);
    caf_push_strong(&send, val64);
    caf_pop_strong(&recv, &val64);
    caf_push_strong(&send, val64);
    caf_pop_strong(&recv, &val64);
    caf_push_strong(&send, val64);
    caf_pop_strong(&recv, &val64);
    caf_push_strong(&send, val64);
  }

  if ((errorcode = close_caf(recv))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }
  if ((errorcode = close_caf(send))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return NULL;
  }

  return NULL;
}

int main(int argc, char* argv[]) {
  setAffinity(0);
  testname = argv[0];
  if (1 > argc) { /* just to get rid of unused warning */
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }
  cafendpt_t send, recv;
  int errorcode, cnt;
  uint64_t val64 = 0;
  pthread_t threads[2];
  int core[2] = {1, 2};

  if ((errorcode = open_caf(PING_QID, &send))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }
  if ((errorcode = open_caf(PONG_QID, &recv))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }

  if ((errorcode = pthread_create(&threads[0], NULL, echo, (void*)&core[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return 0;
  }

  for (cnt = 0; cnt < NUM_PINGPONG; ++cnt) {
    caf_push_strong(&send, (uint64_t)(cnt * 7));
    caf_push_strong(&send, (uint64_t)(cnt * 7 + 1));
    caf_push_strong(&send, (uint64_t)(cnt * 7 + 2));
    caf_push_strong(&send, (uint64_t)(cnt * 7 + 3));
    caf_push_strong(&send, (uint64_t)(cnt * 7 + 4));
    caf_push_strong(&send, (uint64_t)(cnt * 7 + 5));
    caf_push_strong(&send, (uint64_t)(cnt * 7 + 6));
    caf_pop_strong(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    caf_pop_strong(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 1)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    caf_pop_strong(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 2)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    caf_pop_strong(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 3)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    caf_pop_strong(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 4)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    caf_pop_strong(&recv, &val64);
    if (val64 != (uint64_t)(cnt * 7 + 5)) {
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      return 0;
    }
    caf_pop_strong(&recv, &val64);
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
      caf_push_strong(&send, (uint64_t)cnt);
    }
    for (i = 0; i < 14; ++i) {
      caf_pop_strong(&recv, &val64);
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
