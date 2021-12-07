#include <stdio.h>
#include <vl.h>

int main(int argc, char* argv[]) {
  int fd = mkvl();
  if (0 > fd || 1 < argc || !argv) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  } else {
    printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
    return 0;
  }
}
