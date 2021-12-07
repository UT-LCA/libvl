#include <iostream>
#include <vl.h>

int main(int argc, char* argv[]) {
  int fd = mkvl();
  if (0 > fd || 1 < argc || !argv) {
    std::cout << "\033[91mFAILED:\033[0m " << argv[0] << std::endl;
    return 0;
  }
  vlendpt_t endpt;
  int errorcode, cnt;
  if ((errorcode = open_byte_vl_as_producer(fd, &endpt, 2))) {
    std::cout << "\033[91mFAILED:\033[0m " << argv[0] << std::endl;
    return 0;
  }
  for (cnt = 0; cnt < 128; ++cnt) {
    while (!byte_vl_push_non(&endpt, (uint8_t)cnt));
  }
  if ((errorcode = close_byte_vl_as_producer(endpt))) {
    std::cout << "\033[91mFAILED:\033[0m " << argv[0] << std::endl;
    return 0;
  }
  std::cout << "\033[92mPASSED:\033[0m " << argv[0] << std::endl;
  return 0;
}
