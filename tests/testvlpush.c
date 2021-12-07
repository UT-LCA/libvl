#include <stdio.h>
#include <vl.h>

int main(int argc, char* argv[]) {
  int fd = mkvl();
  if (0 > fd || 1 < argc || !argv) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return fd;
  }
  vlendpt_t endpts[8];
  int errorcode, cnt;
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[0], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!byte_vl_push_non(&endpts[0], (uint8_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    byte_vl_push_weak(&endpts[0], (uint8_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    byte_vl_push_strong(&endpts[0], (uint8_t)cnt);
  }
  if ((errorcode = open_half_vl_as_producer(fd, &endpts[1], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!half_vl_push_non(&endpts[1], (uint16_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    half_vl_push_weak(&endpts[1], (uint16_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    half_vl_push_strong(&endpts[1], (uint16_t)cnt);
  }
  if ((errorcode = open_word_vl_as_producer(fd, &endpts[2], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!word_vl_push_non(&endpts[2], (uint32_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    word_vl_push_weak(&endpts[2], (uint32_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    word_vl_push_strong(&endpts[2], (uint32_t)cnt);
  }
  if ((errorcode = open_twin_vl_as_producer(fd, &endpts[3], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!twin_vl_push_non(&endpts[3], (uint64_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    twin_vl_push_weak(&endpts[3], (uint64_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    twin_vl_push_strong(&endpts[3], (uint64_t)cnt);
  }
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[4], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!byte_vl_push_non(&endpts[4], (uint8_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    byte_vl_push_weak(&endpts[4], (uint8_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    byte_vl_push_strong(&endpts[4], (uint8_t)cnt);
  }
  if ((errorcode = open_half_vl_as_producer(fd, &endpts[5], 3))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!half_vl_push_non(&endpts[5], (uint16_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    half_vl_push_weak(&endpts[5], (uint16_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    half_vl_push_strong(&endpts[5], (uint16_t)cnt);
  }
  if ((errorcode = open_word_vl_as_producer(fd, &endpts[6], 4))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!word_vl_push_non(&endpts[6], (uint32_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    word_vl_push_weak(&endpts[6], (uint32_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    word_vl_push_strong(&endpts[6], (uint32_t)cnt);
  }
  if ((errorcode = open_twin_vl_as_producer(fd, &endpts[7], 5))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!twin_vl_push_non(&endpts[7], (uint64_t)cnt));
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    twin_vl_push_weak(&endpts[7], (uint64_t)cnt);
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    twin_vl_push_strong(&endpts[7], (uint64_t)cnt);
  }
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
