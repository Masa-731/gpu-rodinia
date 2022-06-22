#makefile

openacc: ex_particle_OPENMP_seq.c
	nvc -fast -acc -Minfo=accel ex_particle_OPENMP_seq.c -o particle_filter -lm 

clean:
	rm particle_filter
