#include <stdio.h>
#include <caf.h>

#define NUM_PUSHPOP 250
#define CAF_QID 1

int main(int argc, char* argv[]) {
  cafendpt_t endpts[2];
  int errorcode, cnt;
  uint64_t val64;
  bool checked[NUM_PUSHPOP];
  argc = argc;
  /* test push/pop _non */
  if ((errorcode = open_caf(CAF_QID, &endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    while (!caf_push_non(&endpts[0], (uint64_t)cnt));
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP;) {
    if (caf_pop_non(&endpts[0], &val64)) {
      if (checked[val64]) {
        printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)val64);
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      } else {
        checked[val64] = true;
        ++cnt;
      }
    }
  }
  if ((errorcode = close_caf(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test push/pop _strong */
  if ((errorcode = open_caf(CAF_QID, &endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_caf(CAF_QID, &endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    caf_push_strong(&endpts[0], (uint64_t)cnt);
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP;) {
    caf_pop_strong(&endpts[0], &val64);
    if (checked[val64]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)val64);
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val64] = true;
      ++cnt;
    }
  }
  if ((errorcode = close_caf(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = close_caf(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
