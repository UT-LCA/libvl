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
  uint8_t val8;
  uint16_t val16;
  uint32_t val32;
  uint64_t val64;
  bool isvalid;
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[0], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    byte_vl_pop_non(&endpts[0], &val8, &isvalid);
  }
  if ((errorcode = open_half_vl_as_consumer(fd, &endpts[1], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    half_vl_pop_non(&endpts[1], &val16, &isvalid);
  }
  if ((errorcode = open_word_vl_as_consumer(fd, &endpts[2], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    word_vl_pop_non(&endpts[2], &val32, &isvalid);
  }
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpts[3], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    twin_vl_pop_non(&endpts[3], &val64, &isvalid);
  }
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[4], 16))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    byte_vl_pop_non(&endpts[4], &val8, &isvalid);
  }
  if ((errorcode = open_half_vl_as_consumer(fd, &endpts[5], 8))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    half_vl_pop_non(&endpts[5], &val16, &isvalid);
  }
  if ((errorcode = open_word_vl_as_consumer(fd, &endpts[6], 4))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    word_vl_pop_non(&endpts[6], &val32, &isvalid);
  }
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpts[7], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    twin_vl_pop_non(&endpts[7], &val64, &isvalid);
  }
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
