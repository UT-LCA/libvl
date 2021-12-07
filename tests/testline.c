#include <stdio.h>
#include <vl.h>

#define NUM_COUNTERS 100

struct Msg {
  uint64_t* arr;
  uint64_t len;
  uint64_t beg;
  uint64_t end;
};

int main(int argc, char* argv[]) {
  int fd = mkvl();
  if (0 > fd || 1 < argc || !argv) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return fd;
  }
  vlendpt_t endpts[2];
  uint64_t errorcode, cnt;
  struct Msg msg;
  uint64_t counters[NUM_COUNTERS] = {0};
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[0], 3))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[1], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = NUM_COUNTERS; 0 < cnt; --cnt) {
    uint64_t burst = 0;
    uint64_t n = NUM_COUNTERS / cnt;
    uint64_t r = NUM_COUNTERS % cnt;
    uint64_t num_valid;
    uint64_t idx;
    msg.arr = counters;
    msg.len = NUM_COUNTERS;
    /* send cnt messages to cover all counters */
    for (burst = 0; cnt > burst; ++burst) {
      msg.beg = n * burst + (burst >= r ? r : burst);
      msg.end = msg.beg + (burst >= r ? n : n + 1);
      line_vl_push_strong(&endpts[0], (uint8_t*) &msg, sizeof(msg));
    }
    /* receive cnt messages and increase all counters */
    for (burst = 0; cnt > burst; ++burst) {
      line_vl_pop_strong(&endpts[1], (uint8_t*) &msg, &num_valid);
      if (sizeof(msg) != num_valid) {
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      }
      for (idx = msg.beg; msg.end > idx; ++idx) {
        msg.arr[idx]++;
      }
    }
    /* verify that every counter is increased by 1 */
    for (idx = 0; NUM_COUNTERS > idx; ++idx) {
      if (counters[idx] != (NUM_COUNTERS - cnt + 1)) {
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      }
    }
  }
  /* reset counters to test _weak */
  for (cnt = 0; NUM_COUNTERS > cnt; ++cnt) {
    counters[cnt] = 0;
  }
  for (cnt = NUM_COUNTERS; 0 < cnt; --cnt) {
    uint64_t burst = 0;
    uint64_t n = NUM_COUNTERS / cnt;
    uint64_t r = NUM_COUNTERS % cnt;
    uint64_t num_valid;
    uint64_t idx;
    msg.arr = counters;
    msg.len = NUM_COUNTERS;
    /* send cnt messages to cover all counters */
    for (burst = 0; cnt > burst; ++burst) {
      msg.beg = n * burst + (burst >= r ? r : burst);
      msg.end = msg.beg + (burst >= r ? n : n + 1);
      line_vl_push_weak(&endpts[0], (uint8_t*) &msg, sizeof(msg));
    }
    /* receive cnt messages and increase all counters */
    for (burst = 0; cnt > burst; ++burst) {
      line_vl_pop_weak(&endpts[1], (uint8_t*) &msg, &num_valid);
      if (sizeof(msg) != num_valid) {
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      }
      for (idx = msg.beg; msg.end > idx; ++idx) {
        msg.arr[idx]++;
      }
    }
    /* verify that every counter is increased by 1 */
    for (idx = 0; NUM_COUNTERS > idx; ++idx) {
      if (counters[idx] != (NUM_COUNTERS - cnt + 1)) {
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      }
    }
  }
  /* reset counters to test _weak */
  for (cnt = 0; NUM_COUNTERS > cnt; ++cnt) {
    counters[cnt] = 0;
  }
  /* send cnt messages to cover all counters */
  for (cnt = 0; NUM_COUNTERS > cnt; ++cnt) {
    msg.beg = cnt;
    msg.end = msg.beg + 1;
    while(!line_vl_push_non(&endpts[0], (uint8_t*) &msg, sizeof(msg)));
  }
  if ((errorcode = close_byte_vl_as_producer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* receive cnt messages and increase all counters */
  for (cnt = 0; NUM_COUNTERS > cnt;) {
    uint64_t num_valid = sizeof(msg);
    line_vl_pop_non(&endpts[1], (uint8_t*) &msg, &num_valid);
    if (sizeof(msg) == num_valid) {
      uint64_t idx;
      for (idx = msg.beg; msg.end > idx; ++idx) {
        msg.arr[idx]++;
      }
      ++cnt;
    } else if (0 != num_valid) {
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  }

  /* verify that every counter is increased by 1 */
  for (cnt = 0; NUM_COUNTERS > cnt; ++cnt) {
    if (counters[cnt] != 1) {
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  }
  
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
