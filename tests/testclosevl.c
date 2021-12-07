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
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[0], 34))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_byte_vl_as_producer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_half_vl_as_producer(fd, &endpts[1], 33))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_half_vl_as_producer(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_word_vl_as_producer(fd, &endpts[2], 32))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_word_vl_as_producer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_producer(fd, &endpts[3], 31))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_twin_vl_as_producer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[4], 46))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_byte_vl_as_consumer(endpts[4]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_half_vl_as_consumer(fd, &endpts[5], 38))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_half_vl_as_consumer(endpts[5]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_word_vl_as_consumer(fd, &endpts[6], 24))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_word_vl_as_consumer(endpts[6]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpts[7], 12))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else if ((errorcode = close_twin_vl_as_consumer(endpts[7]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
