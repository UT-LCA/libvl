prefix = /usr/local
includedir = ${prefix}/include
libdir = ${prefix}/include

AR = ar
CC = gcc
CCFLAGS = -O3 -g -Wall -Wextra -Werror -std=c11 -pthread
CXX = g++
CXXFLAGS = -O3 -g -Wall -Wextra -Werror -std=c++11 -pthread

LIB_TGTS = libvl/libvl.so libvl/libvl.a ext/libcaf/libcaf.so ext/libcaf/libcaf.a

TEST_TGTS = \
	tests/testmkvl \
	tests/testopenvl tests/testclosevl \
	tests/testvlpush tests/testvlpop \
	tests/testpushpop \
	tests/testline tests/testsize \
	tests/testcpp \
	tests/testmt \
    tests/testipc

# by default compile for gem5 simulation as well, unless NOGEM5
ifeq ($(NOGEM5), true)
LIBSO_NM = libvl.so.nogem5
LIBA_NM = libvl.a.nogem5
GEM5_INC =
GEM5_LIB =
else ifdef GEM5_ROOT # export GEM5_ROOT=<path to gem5 repo>
ifeq ($(M5VL), true)
LIBSO_NM = libvl.so.m5vl
LIBA_NM = libvl.a.m5vl
else
LIBSO_NM = libvl.so.gem5
LIBA_NM = libvl.a.gem5
endif # end of M5VL
GEM5_INC = -I$(GEM5_ROOT)/include
GEM5_LIB = -L$(GEM5_ROOT)/util/m5 -lm5
else ifneq ($(MAKECMDGOALS), clean) # no error on clean
$(error set GEM5_ROOT or NOGEM5=true)
endif

# if no syscall support for libvl, use faked ones
ifeq (${NOSYSVL}, true)
CCFLAGS += -Ikernel -DNOSYSVL=1
CXXFLAGS += -Ikernel -DNOSYSVL=1
SYSVL_NOGEM5 = kernel/faked_sysvl_nogem5.o
SYSVL_GEM5 = kernel/faked_sysvl_gem5.o
else
SYSVL_NOGEM5 =
SYSVL_GEM5 =
endif

# if use pthread mutex in libvl
ifeq (${PTHREAD_MUTEX}, true)
CCFLAGS += -DPTHREAD_MUTEX=1
CXXFLAGS += -DPTHREAD_MUTEX=1
endif

# whether need DBM depends on DC PSVAC, DC SVAC implementations in gem5
ifeq (${NEEDDMB}, true)
CCFLAGS += -DNEEDDMB=1
CXXFLAGS += -DNEEDDMB=1
endif

all: $(LIB_TGTS)
test: $(TEST_TGTS)
	@for test_tgt in `echo $(TEST_TGTS)`; do ./$${test_tgt}; done

kernel/faked_sysvl_nogem5.o: kernel/faked_sysvl.c
	$(CC) $(CCFLAGS) -DNOGEM5=1 -c $< -o $@

kernel/faked_sysvl_gem5.o: kernel/faked_sysvl.c
	$(CC) $(CCFLAGS) -c $< -o $@

libvl/libvl_nogem5.o: libvl/libvl.c libvl/vl_insts.h
	$(CC) $(CCFLAGS) -DNOGEM5=1 -c $< -o $@

libvl/libvl_gem5.o: libvl/libvl.c libvl/vl_insts.h
	$(CC) $(CCFLAGS) $(GEM5_INC) -c $< -o $@ $(GEM5_LIB)

libvl/libvl_m5vl.o: libvl/libvl.c libvl/vl_insts.h
	$(CC) $(CCFLAGS) $(GEM5_INC) -DM5VL=1 -c $< -o $@ $(GEM5_LIB)

libvl/libvl.so.nogem5: libvl/libvl.c $(SYSVL_NOGEM5)
	$(CC) $(CCFLAGS) -DNOGEM5=1 -fPIC -shared $< -o $@

libvl/libvl.so.gem5: libvl/libvl.c $(SYSVL_GEM5)
	$(CC) $(CCFLAGS) $(GEM5_INC) -fPIC -shared $< -o $@ $(GEM5_LIB)

libvl/libvl.so.m5vl: libvl/libvl.c $(SYSVL_GEM5)
	$(CC) $(CCFLAGS) $(GEM5_INC) -DM5VL=1 -fPIC -shared $< -o $@ $(GEM5_LIB)

libvl/libvl.a.nogem5: libvl/libvl_nogem5.o $(SYSVL_NOGEM5)
	$(AR) rcs $@ $^

libvl/libvl.a.gem5: libvl/libvl_gem5.o $(SYSVL_GEM5)
	$(AR) rcs $@ $^

libvl/libvl.a.m5vl: libvl/libvl_m5vl.o $(SYSVL_GEM5)
	$(AR) rcs $@ $^

ext/libcaf/libcaf.o: ext/libcaf/libcaf.c ext/libcaf/caf_insts.h
	$(CC) $(CCFLAGS) $(GEM5_INC) -c $< -o $@ $(GEM5_LIB)

.PHONY: libvl/libvl.so libvl/libvl.a ext/libcaf/libcaf.so ext/libcaf/libcaf.a

libvl/libvl.so: libvl/$(LIBSO_NM)
	if [ -L $@ ]; then unlink $@; fi
	ln -s $(LIBSO_NM) $@

libvl/libvl.a: libvl/$(LIBA_NM)
	if [ -L $@ ]; then unlink $@; fi
	ln -s $(LIBA_NM) $@

ext/libcaf/libcaf.so: ext/libcaf/libcaf.o
	$(CC) $(CCFLAGS) $(GEM5_INC) -fPIC -shared $< -o $@ $(GEM5_LIB)

ext/libcaf/libcaf.a: ext/libcaf/libcaf.o
	$(AR) rcs $@ $^


install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(includedir)
	cp -r vl $(DESTDIR)$(includedir)

install-libs: libvl/$(LIBSO_NM) libvl/$(LIBA_NM)
	mkdir -p $(DESTDIR)$(libdir)
	rm -f $(DESTDIR)$(libdir)/{$(soname), $(libaname)}
	cp libvl/{$(LIBSO_NM), $(LIBA_NM)} -t $(DESTDIR)$(libdir)
	ln -s $(LIBSO_NM) $(DESTDIR)$(libdir)/$(soname)
	ln -s $(LIBA_NM) $(DESTDIR)$(libdir)/$(libaname)

VL_INC = -Ivl
VL_LIB = -Llibvl -lvl $(GEM5_LIB)
CAF_INC = -Iext/libcaf
CAF_LIB = -Lext/libcaf -lcaf $(GEM5_LIB)

tests/%.o: tests/%.c
	$(CC) $(CCFLAGS) $(VL_INC) -static -c $< -o $@ $(VL_LIB)

tests/testmt: tests/testmt.c tests/threading.o libvl/libvl.a
	$(CC) $(CCFLAGS) $(VL_INC) -static $^ -o $@ $(VL_LIB)

tests/%: tests/%.c libvl/libvl.a
	$(CC) $(CCFLAGS) $(VL_INC) -static $< -o $@ $(VL_LIB)

tests/%: tests/%.cpp libvl/libvl.a
	$(CXX) $(CXXFLAGS) $(VL_INC) -static $< -o $@ $(VL_LIB)

ext/libcaf/testpushpop: ext/libcaf/testpushpop.c ext/libcaf/libcaf.a
	$(CXX) $(CXXFLAGS) $(CAF_INC) -static $< -o $@ $(CAF_LIB)

ext/libcaf/testmt: ext/libcaf/testmt.c ext/libcaf/libcaf.a tests/threading.o
	$(CXX) $(CXXFLAGS) $(CAF_INC) -static $^ -o $@ $(CAF_LIB)

.PHONY: clean

clean:
	rm -rf kernel/*.o libvl/*.o libvl/libvl.so* libvl/libvl.a* $(TEST_TGTS)
	rm -f ext/libcaf/*.o ext/libcaf/libcaf.so* ext/libcaf/libcaf.a* ext/libcaf/testpushpop ext/libcaf/testmt
