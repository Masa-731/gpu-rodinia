# C compiler
CC = nvc++
ICC = icc
CC_FLAGS = -g -fast -acc -Minfo=accel
OFFLOAD_CC_FLAGS = -offload-option,mic,compiler,"-no-opt-prefetch"

all: needle

needle: needle.cpp
	$(CC) $(CC_FLAGS) needle.cpp -o needle 

needle_offload:
	$(ICC) $(CC_FLAGS) $(OFFLOAD_CC_FLAGS) -DOMP_OFFLOAD needle.cpp -o needle_offload

clean:
	rm -f needle
