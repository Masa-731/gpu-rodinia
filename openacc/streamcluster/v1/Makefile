TARGET_G = sc_gpu

all: gpu 

gpu:
	nvc -acc -gpu=cc86 -Minfo=accel -std=c99 -fast -o $(TARGET_G) streamcluster.c -lm
#gcc -std=c99 -O3 -o $(TARGET_G) streamcluster.c -lm

clean:
	rm -f *.o *~ *.txt sc_gpu *.hmf* *.log *.out
	
