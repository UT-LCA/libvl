#include <stdio.h>
#include <vl.h>

int main(int argc, char* argv[]) {
  int fd = mkvl();
  if (0 > fd || 1 < argc || !argv) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return fd;
  }
  vlendpt_t endpts[8];
  int errorcode;
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[0], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_half_vl_as_producer(fd, &endpts[1], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_word_vl_as_producer(fd, &endpts[2], 3))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_producer(fd, &endpts[3], 4))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[4], 16))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_half_vl_as_consumer(fd, &endpts[5], 8))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_word_vl_as_consumer(fd, &endpts[6], 4))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpts[7], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
