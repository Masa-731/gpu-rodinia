# C compiler
CC = nvc++
CC_FLAGS = -g -fast -acc -Minfo=accel -gpu=cc86

bfs: 
	$(CC) $(CC_FLAGS) srad.cpp -o srad 

clean:
	rm -f srad
