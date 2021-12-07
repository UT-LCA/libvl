#include <iostream>
#include <iomanip>
#include <queue>
#include <sys/time.h>
#include <vl.h>

int main()
{
    int fd = mkvl();
    if (0 > fd) {
        std::cerr << "mkvl() returned " << fd << std::endl;
        return fd;
    }

    int errcode;
    vlendpt_t bytevl;

    errcode = open_byte_vl_as_producer(fd, &bytevl, 1);
    if (0 != errcode) {
        std::cerr << "open_byte_vl_as_producer() returned " <<
            errcode << std::endl;
        return errcode;
    } else {
        std::cout << "allocated byte queue cacheline addr: 0x";
        std::cout << std::hex << std::setfill('0') << std::setw(16) <<
            (uint64_t) bytevl.pcacheline << std::setfill(' ') <<
            std::dec << std::endl;
        std::cout << "allocated byte queue control region: 0x";
        std::cout << std::hex << std::setfill('0') << std::setw(2) <<
            (uint64_t) ((uint8_t*)bytevl.pcacheline)[63] << std::setw(2) <<
            (uint64_t) ((uint8_t*)bytevl.pcacheline)[62] <<
            std::setfill(' ') << std::dec << std::endl;
    }
    // test enqueue
    uint8_t val8bit;
    struct timeval now;

    if (0 != gettimeofday(&now, NULL)) {
        std::cerr << "gettimeofday() failed\n";
        exit(-1);
    }
    val8bit = now.tv_usec;
    for (int i = 0; i < 70; ++i) {
        if (0 != gettimeofday(&now, NULL)) {
            std::cerr << "gettimeofday() failed\n";
            exit(-1);
        }
        val8bit = now.tv_usec - val8bit;
        // blocking
        byte_vl_push_weak(&bytevl, val8bit);
        val8bit = now.tv_usec;
        // inspect the cacheline
        std::cout << std::hex << std::setfill('0');
        for (int j = 63; 0 <= j; --j) {
            std::cout << std::setw(2) <<
                (uint64_t)((uint8_t*)bytevl.pcacheline)[j];
            if (0 == j % 16) {
                std::cout << std::endl;
            } else {
                std::cout << " ";
            }
        }
        std::cout << std::setfill(' ') << std::dec << std::endl;
    }

    errcode = close_byte_vl_as_producer(bytevl);
    if (0 != errcode) {
        std::cerr << "close_byte_vl_as_producer() returned " <<
            errcode << std::endl;
        return errcode;
    }

    std::queue<uint32_t> record_queue;
    vlendpt_t wordvl;
    errcode = open_word_vl_as_producer(fd, &wordvl, 3);
    if (0 != errcode) {
        std::cerr << "open_word_vl_as_producer() returned " <<
            errcode << std::endl;
        return errcode;
    } else {
        std::cout << "allocated word queue cacheline addr: 0x";
        std::cout << std::hex << std::setfill('0') << std::setw(16) <<
            (uint64_t) wordvl.pcacheline << std::setfill(' ') <<
            std::dec << std::endl;
        std::cout << "allocated word queue control region: 0x";
        std::cout << std::hex << std::setfill('0') << std::setw(2) <<
            (uint64_t) ((uint8_t*)wordvl.pcacheline)[63] << std::setw(2) <<
            (uint64_t) ((uint8_t*)wordvl.pcacheline)[62] <<
            std::setfill(' ') << std::dec << std::endl;
    }
    // test enqueue multi-cacheline
    uint32_t val32bit;
    if (0 != gettimeofday(&now, NULL)) {
        std::cerr << "gettimeofday() failed\n";
        exit(-1);
    }
    val32bit = now.tv_usec;
    for (int i = 0; i < 70; ++i) {
        if (0 != gettimeofday(&now, NULL)) {
            std::cerr << "gettimeofday() failed\n";
            exit(-1);
        }
        val32bit = (now.tv_usec - val32bit) << (i % 32);
        // non-blocking
        word_vl_push_non(&wordvl, val32bit);
        record_queue.push(val32bit);
        val32bit = now.tv_usec;
        // inspect the cacheline
        std::cout << std::hex << std::setfill('0');
        for (int j = 63; 0 <= j; --j) {
            std::cout << std::setw(2) <<
                (uint64_t)((uint8_t*)wordvl.pcacheline)[j];
            if (0 == j % 32) {
                std::cout << std::endl;
            } else if (0 == j % 4) {
                std::cout << " ";
            }
        }
        std::cout << std::setfill(' ') << std::dec << std::endl;
    }

    vlendpt_t wordvl2;
    errcode = open_word_vl_as_consumer(fd, &wordvl2, 3);
    if (0 != errcode) {
        std::cerr << "open_word_vl_as_consumer() returned " <<
            errcode << std::endl;
        return errcode;
    } else {
        std::cout << "allocated word queue cacheline addr: 0x";
        std::cout << std::hex << std::setfill('0') << std::setw(16) <<
            (uint64_t) wordvl2.pcacheline << std::endl;
        std::cout << "allocated word queue control region: 0x";
        std::cout << std::setw(2) <<
            (uint64_t) ((uint8_t*)wordvl2.pcacheline)[63] << std::setw(2) <<
            (uint64_t) ((uint8_t*)wordvl2.pcacheline)[62] <<
            std::setfill(' ') << std::dec << std::endl;
        // TODO: find an alternative way to fake the data movement
        for (int i = 0; 2 >= i; ++i) {
            uint8_t *pprodB = (uint8_t*)(
                    ((uint64_t)wordvl.pcacheline & 0xFFFFFFFFFFFFF000)
                    + (i + 1) * 64);
            uint8_t *pconsB = (uint8_t*)(
                    ((uint64_t)wordvl2.pcacheline & 0xFFFFFFFFFFFFF000)
                    + i * 64);
            for (int j = 61; 0 <= j; --j) {
                pconsB[j] = pprodB[j];
            }
        }
    }
    for (int i = 0; i < 35; ++i) {
        // nonblocking
        bool isvalid;
        word_vl_pop_non(&wordvl2, &val32bit, &isvalid);
        if (!isvalid) {
            std::cout << "No valid data popped\n";
            continue;
        }
        std::cout << std::hex << std::setfill('0');
        std::cout << "Got " << std::setw(8) <<
            (uint64_t)val32bit << std::endl;
        if (record_queue.front() != val32bit) {
            std::cerr << "\033[91mdiff from record[" << i << "]=" <<
                std::hex << std::setfill('0') << std::setw(8) <<
                (uint64_t)record_queue.front() << std::setfill(' ') <<
                std::dec << "\033[0m\n";
        }
        record_queue.pop();
        // inspect the cacheline
        for (int j = 63; 0 <= j; --j) {
            std::cout << std::setw(2) <<
                (uint64_t)((uint8_t*)wordvl2.pcacheline)[j];
            if (0 == j % 32) {
                std::cout << std::endl;
            } else if (0 == j % 4) {
                std::cout << " ";
            }
        }
        std::cout << std::setfill(' ') << std::dec << std::endl;
    }
    for (int i = 35; i < 70; ++i) {
        // blocking
        word_vl_pop_weak(&wordvl2, &val32bit);
        std::cout << std::hex << std::setfill('0');
        std::cout << "Got " << std::setw(8) <<
            (uint64_t)val32bit << std::endl;
        if (record_queue.front() != val32bit) {
            std::cerr << "\033[91mdiff from record[" << i << "]=" <<
                std::hex << std::setfill('0') << std::setw(8) <<
                (uint64_t)record_queue.front() << std::setfill(' ') <<
                std::dec << "\033[0m\n";
        }
        record_queue.pop();
        // inspect the cacheline
        for (int j = 63; 0 <= j; --j) {
            std::cout << std::setw(2) <<
                (uint64_t)((uint8_t*)wordvl2.pcacheline)[j];
            if (0 == j % 32) {
                std::cout << std::endl;
            } else if (0 == j % 4) {
                std::cout << " ";
            }
        }
        std::cout << std::setfill(' ') << std::dec << std::endl;
    }

    return 0;
}

