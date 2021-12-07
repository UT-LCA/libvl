#include <stdio.h>
#include <vl.h>
#include "threading.h"

char *testname;
int fd0, fd1;

void echo() {
  int errorcode;
  size_t idx;
  vlendpt_t prod, cons;
  char buf[64];

  if ((errorcode = open_byte_vl_as_consumer(fd0, &cons, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }
  if ((errorcode = open_byte_vl_as_producer(fd1, &prod, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }

  line_vl_pop_weak(&cons, (uint8_t*)buf, &idx);
  for (idx = 0; idx < 62; ++idx) {
    if ('\0' == buf[idx]) {
      break;
    }
    if ('a' <= buf[idx] && buf[idx] <= 'z') {
      buf[idx] = buf[idx] - 'a' + 'A';
    }
  }
  line_vl_push_strong(&prod, (uint8_t*)buf, idx);

  if ((errorcode = close_byte_vl_as_consumer(cons))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }
  if ((errorcode = close_byte_vl_as_producer(prod))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }

  return;
}

void send(char *msg) {
  int errorcode;
  size_t idx;
  vlendpt_t endpt;

  if ((errorcode = open_byte_vl_as_producer(fd0, &endpt, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }
  for (idx = 0; idx < 62; ++idx) {
    if ('\0' == msg[idx]) {
      break;
    }
  }
  msg[idx] = '\0';
  line_vl_push_strong(&endpt, (uint8_t*)msg, idx);
  if ((errorcode = close_byte_vl_as_producer(endpt))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }
}

void recv(char *msg) {
  int errorcode;
  size_t idx;
  char buf[64];
  vlendpt_t endpt;

  if ((errorcode = open_byte_vl_as_consumer(fd1, &endpt, 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }
  line_vl_pop_weak(&endpt, (uint8_t*)buf, &idx);
  for (idx = 0; idx < 62; ++idx) {
    if ('\0' == buf[idx]) {
      break;
    }
    if (msg[idx] != buf[idx]) {
      if ('a' <= msg[idx] && msg[idx] <= 'z' &&
          (msg[idx] - 'a' + 'A') == buf[idx]) {
        continue;
      }
      printf("\033[91mFAILED:\033[0m %s\n", testname);
      break;
    }
  }
  if ((errorcode = close_byte_vl_as_consumer(endpt))) {
    printf("\033[91mFAILED:\033[0m %s\n", testname);
    return;
  }
}

int main(int argc, char* argv[]) {
  testname = argv[0];
  fd0 = mkvl();
  fd1 = mkvl();
  if (2 > argc) { /* server process started w/o argument */
    echo();
  } else { /* client process executed with the message to send */
    send(argv[1]);
    recv(argv[1]);
  }

  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
