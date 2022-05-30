TARGET_G = sc_omp

all: omp

omp:
	nvc -mp -std=c99 -fast -o $(TARGET_G) streamcluster.c -lm
#gcc -std=c99 -O3 -o $(TARGET_G) streamcluster.c -lm

clean:
	rm -f *.o *~ *.txt sc_omp *.hmf* *.log *.out
	
