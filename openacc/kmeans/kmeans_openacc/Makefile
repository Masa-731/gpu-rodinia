# C compiler
CC = nvc #gcc
CC_FLAGS = -acc -Minfo=accel -gpu=cc86  -g #-fopenmp -O2 

kmeans: cluster.o getopt.o kmeans.o kmeans_clustering.o wtime.o
	$(CC) $(CC_FLAGS) cluster.o getopt.o kmeans.o kmeans_clustering.o wtime.o -o kmeans

%.o: %.[ch]
	$(CC) $(CC_FLAGS) $< -c

cluster.o: cluster.c 
	$(CC) $(CC_FLAGS) cluster.c -c
	
getopt.o: getopt.c 
	$(CC) $(CC_FLAGS) getopt.c -c
	
kmeans.o: kmeans.c 
	$(CC) $(CC_FLAGS) kmeans.c -c

kmeans_clustering.o: kmeans_clustering.c kmeans.h
	$(CC) $(CC_FLAGS) kmeans_clustering.c -c

wtime.o: wtime.c
	$(CC) $(CC_FLAGS) wtime.c -c

clean:
	rm -f *.o *~ kmeans 
