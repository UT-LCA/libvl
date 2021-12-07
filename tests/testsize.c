#include <stdio.h>
#include <vl.h>

int main(int argc, char* argv[]) {
  int fd = mkvl();
  if (0 > fd || 1 < argc || !argv) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return fd;
  }
  vlendpt_t endpt;
  uint64_t res = 0;
  uint64_t errorcode, cnt, val64;
  bool isvalid;
  if ((errorcode = open_twin_vl_as_producer(fd, &endpt, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; 700 > cnt; ++cnt) {
    twin_vl_push_strong(&endpt, res);
    res = vl_size(&endpt);
    if (res != ((cnt + 1) / 7)) {
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  }
  if ((errorcode = close_twin_vl_as_producer(endpt))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpt, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 700; cnt != 0;) {
    twin_vl_pop_non(&endpt, (uint64_t*)&val64, &isvalid);
    if (isvalid) {
      cnt--;
      res = vl_size(&endpt);
      if (res != (cnt / 7)) {
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      }
    }
  }
  if ((errorcode = close_twin_vl_as_consumer(endpt))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
