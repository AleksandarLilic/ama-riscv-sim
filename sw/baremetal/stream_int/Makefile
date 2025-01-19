NTIMES := 500
TARGET := stream
GCC_OPTS += -O3 -flto
CFLAGS += -DSTREAM_TYPE=uint32_t -DSTREAM_ARRAY_SIZE=2048 -DNTIMES=$(NTIMES)

all: $(TARGET).elf

include ../Makefile.inc
