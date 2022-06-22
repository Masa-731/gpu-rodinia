LOCAL_CC = gcc -g -O3 -Wall
CC = nvc++
CFLAGS = -Minfo=acc -fast -acc


all : nn

clean :
	rm -rf *.o nn

nn : nn_openmp.cpp
	$(CC) -o $@ $< $(LDFLAGS) $(CFLAGS) 

hurricane_gen : hurricane_gen.c
	$(LOCAL_CC) -o $@ $< -lm

