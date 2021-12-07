#include <stdio.h>
#include <vl.h>

#define NUM_PUSHPOP 250

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
  bool checked[NUM_PUSHPOP];
  /* test byte push/pop _non */
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[0], 3))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    while (!byte_vl_push_non(&endpts[0], (uint8_t)cnt));
  }
  if ((errorcode = close_byte_vl_as_producer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[0], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP;) {
    byte_vl_pop_non(&endpts[0], &val8, &isvalid);
    if (isvalid) {
      if (checked[val8]) {
        printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)val8);
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      } else {
        checked[val8] = true;
        /* printf("\033[93CHECK:\033[0m %lx\n", (uint64_t)val8); */
        ++cnt;
      }
    }
  }
  if ((errorcode = close_byte_vl_as_consumer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test half push/pop _non */
  if ((errorcode = open_half_vl_as_producer(fd, &endpts[1], 6))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    while (!half_vl_push_non(&endpts[1], (uint16_t)(cnt + 0x101)));
  }
  if ((errorcode = close_half_vl_as_producer(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_half_vl_as_consumer(fd, &endpts[1], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP;) {
    half_vl_pop_non(&endpts[1], &val16, &isvalid);
    if (isvalid) {
      if (checked[val16 - 0x101]) {
        printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val16 - 0x101));
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      } else {
        checked[val16 - 0x101] = true;
        ++cnt;
      }
    }
  }
  if ((errorcode = close_half_vl_as_consumer(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test word push/pop _non */
  if ((errorcode = open_word_vl_as_producer(fd, &endpts[2], 12))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    while (!word_vl_push_non(&endpts[2], (uint32_t)(cnt + 0x10101)));
  }
  if ((errorcode = close_word_vl_as_producer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_word_vl_as_consumer(fd, &endpts[2], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP;) {
    word_vl_pop_non(&endpts[2], &val32, &isvalid);
    if (isvalid) {
      if (checked[val32 - 0x10101]) {
        printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val32 - 0x10101));
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      } else {
        checked[val32 - 0x10101] = true;
        ++cnt;
      }
    }
  }
  if ((errorcode = close_word_vl_as_consumer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test twin push/pop _non */
  if ((errorcode = open_twin_vl_as_producer(fd, &endpts[3], 24))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    while (!twin_vl_push_non(&endpts[3], (uint64_t)(cnt + 0x100010101)));
  }
  if ((errorcode = close_twin_vl_as_producer(endpts[3]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpts[3], 8))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP;) {
    twin_vl_pop_non(&endpts[3], &val64, &isvalid);
    if (isvalid) {
      if (checked[val64 - 0x100010101]) {
        printf("\033[91mCONFLICT:\033[0m %lx\n",
               (uint64_t)(val64 - 0x100010101));
        printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
        return 0;
      } else {
        checked[val64 - 0x100010101] = true;
        ++cnt;
      }
    }
  }
  if ((errorcode = close_twin_vl_as_consumer(endpts[3]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test byte push/pop _weak */
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[0], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[3], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    byte_vl_push_weak(&endpts[3], (uint8_t)cnt); /* no FIFO guarantee */
  }
  if ((errorcode = close_byte_vl_as_producer(endpts[3]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    byte_vl_pop_weak(&endpts[0], &val8);
    if (checked[val8]) {
      printf("\033[91mCONFLICT:\033[0m %d\n", (int)val8);
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val8] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    byte_vl_pop_non(&endpts[0], &val8, &isvalid);
    if (isvalid && checked[val8]) {
      printf("\033[91mCONFLICT:\033[0m %d\n", (int)val8);
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_byte_vl_as_consumer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test half push/pop _weak */
  if ((errorcode = open_half_vl_as_consumer(fd, &endpts[1], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_half_vl_as_producer(fd, &endpts[2], 5))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    half_vl_push_weak(&endpts[2], (uint16_t)(cnt + 0x7001));
  }
  if ((errorcode = close_half_vl_as_producer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    half_vl_pop_weak(&endpts[1], &val16);
    if (checked[val16 - 0x7001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val16 - 0x7001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val16 - 0x7001] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    half_vl_pop_non(&endpts[1], &val16, &isvalid);
    if (isvalid && checked[val16 - 0x7001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val16 - 0x7001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_half_vl_as_consumer(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test word push/pop _weak */
  if ((errorcode = open_word_vl_as_consumer(fd, &endpts[2], 3))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_word_vl_as_producer(fd, &endpts[0], 6))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    word_vl_push_weak(&endpts[0], (uint32_t)(cnt + 0x70000001));
  }
  if ((errorcode = close_word_vl_as_producer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    word_vl_pop_weak(&endpts[2], &val32);
    if (checked[val32 - 0x70000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val32 - 0x70000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val32 - 0x70000001] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    word_vl_pop_non(&endpts[2], &val32, &isvalid);
    if (isvalid && checked[val32 - 0x70000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val32 - 0x70000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_word_vl_as_consumer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test twin push/pop _weak */
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpts[3], 16))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_producer(fd, &endpts[1], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    twin_vl_push_weak(&endpts[1], (uint64_t)(cnt + 0x7000000000000001));
  }
  if ((errorcode = close_twin_vl_as_producer(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    twin_vl_pop_weak(&endpts[3], &val64);
    if (checked[val64 - 0x7000000000000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n",
             (uint64_t)(val64 - 0x7000000000000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val64 - 0x7000000000000001] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    twin_vl_pop_non(&endpts[3], &val64, &isvalid);
    if (isvalid && checked[val64 - 0x7000000000000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n",
             (uint64_t)(val64 - 0x7000000000000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_twin_vl_as_consumer(endpts[3]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test byte push/pop _strong */
  if ((errorcode = open_byte_vl_as_consumer(fd, &endpts[1], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_byte_vl_as_producer(fd, &endpts[2], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    byte_vl_push_strong(&endpts[2], (uint8_t)cnt);
  }
  if ((errorcode = close_byte_vl_as_producer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    byte_vl_pop_strong(&endpts[1], &val8);
    if (checked[val8]) {
      printf("\033[91mCONFLICT:\033[0m %d\n", (int)val8);
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val8] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    byte_vl_pop_non(&endpts[1], &val8, &isvalid);
    if (isvalid && checked[val8]) {
      printf("\033[91mCONFLICT:\033[0m %d\n", (int)val8);
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_byte_vl_as_consumer(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test half push/pop _strong */
  if ((errorcode = open_half_vl_as_consumer(fd, &endpts[2], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_half_vl_as_producer(fd, &endpts[0], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    half_vl_push_strong(&endpts[0], (uint16_t)(cnt + 0x7801));
  }
  if ((errorcode = close_half_vl_as_producer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    half_vl_pop_strong(&endpts[2], &val16);
    if (checked[val16 - 0x7801]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val16 - 0x7801));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val16 - 0x7801] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    half_vl_pop_non(&endpts[2], &val16, &isvalid);
    if (isvalid && checked[val16 - 0x7801]) {
      printf("\033[91mCONFLICT:\033[0m %d\n", (int)val8);
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_half_vl_as_consumer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test word push/pop _strong */
  if ((errorcode = open_word_vl_as_consumer(fd, &endpts[3], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_word_vl_as_producer(fd, &endpts[2], 2))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    word_vl_push_strong(&endpts[2], (uint32_t)(cnt + 0x78000001));
  }
  if ((errorcode = close_word_vl_as_producer(endpts[2]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    word_vl_pop_strong(&endpts[3], &val32);
    if (checked[val32 - 0x78000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val32 - 0x78000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val32 - 0x78000001] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    word_vl_pop_non(&endpts[3], &val32, &isvalid);
    if (isvalid && checked[val32 - 0x78000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n", (uint64_t)(val32 - 0x78000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_word_vl_as_consumer(endpts[3]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  /* test twin push/pop _strong */
  if ((errorcode = open_twin_vl_as_consumer(fd, &endpts[0], 1))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  if ((errorcode = open_twin_vl_as_producer(fd, &endpts[1], 4))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) {
    twin_vl_push_strong(&endpts[1], (uint64_t)(cnt + 0x7800000000000001));
  }
  if ((errorcode = close_twin_vl_as_producer(endpts[1]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  for (cnt = 0; cnt < NUM_PUSHPOP; ++cnt) { /* reset checked */
    checked[cnt] = false;
  }
  for (cnt = 0; cnt < (NUM_PUSHPOP - 1); ++cnt) {
    twin_vl_pop_strong(&endpts[0], &val64);
    if (checked[val64 - 0x7800000000000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n",
             (uint64_t)(val64 - 0x7800000000000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    } else {
      checked[val64 - 0x7800000000000001] = true;
    }
  }
  do { /* has to use pop_non for the last value in order to avoid blocking */
    twin_vl_pop_non(&endpts[0], &val64, &isvalid);
    if (isvalid && checked[val64 - 0x7800000000000001]) {
      printf("\033[91mCONFLICT:\033[0m %lx\n",
             (uint64_t)(val64 - 0x7800000000000001));
      printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
      return 0;
    }
  } while (!isvalid);
  if ((errorcode = close_twin_vl_as_consumer(endpts[0]))) {
    printf("\033[91mFAILED:\033[0m %s\n", argv[0]);
    return 0;
  }
  printf("\033[92mPASSED:\033[0m %s\n", argv[0]);
  return 0;
}
