# These three variables are designed to be modifiable.
SST_VERSION=SST-11.0.0
GEM5_LIB=gem5_debug
ARCH=RISCV
OFLAG=0

LDFLAGS=-shared -fno-common ${shell pkg-config ${SST_VERSION} --libs} -L../../build/${ARCH}/ -Wl,-rpath ../../build/${ARCH}
CXXFLAGS=-std=c++17 -g -O${OFLAG} -fPIC ${shell pkg-config ${SST_VERSION} --cflags} ${shell python3-config --includes} -I../../build/${ARCH}/ -I../../ext/pybind11/include/ -I../../build/softfloat/
CPPFLAGS+=-MMD -MP
SRC=$(wildcard *.cc)

.PHONY: clean all

all: libgem5.so

libgem5.so: $(SRC:%.cc=%.o)
	${CXX} ${CPPFLAGS} ${LDFLAGS} $? -o $@ -l${GEM5_LIB} -lcrypto -lssl -lz

-include $(SRC:%.cc=%.d)

clean:
	${RM} *.[do] libgem5.so
