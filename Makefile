include common/make.config

RODINIA_BASE_DIR := $(shell pwd)

CUDA_BIN_DIR := $(RODINIA_BASE_DIR)/bin/linux/cuda
OMP_BIN_DIR := $(RODINIA_BASE_DIR)/bin/linux/omp
OPENCL_BIN_DIR := $(RODINIA_BASE_DIR)/bin/linux/opencl

CUDA_DIRS := backprop bfs cfd gaussian heartwall hotspot kmeans lavaMD leukocyte lud nn	nw srad streamcluster particlefilter pathfinder mummergpu hybridsort dwt2d
OMP_DIRS  := backprop bfs cfd		   heartwall hotspot kmeans lavaMD leukocyte lud nn nw srad streamcluster particlefilter pathfinder mummergpu
OCL_DIRS  := backprop bfs cfd gaussian heartwall hotspot kmeans lavaMD leukocyte lud nn	nw srad streamcluster particlefilter pathfinder

all: CUDA OMP

CUDA: 
	cd cuda/backprop;		make;	mv backprop $(CUDA_BIN_DIR)
	cd cuda/bfs;			make;	mv bfs $(CUDA_BIN_DIR)
	cd cuda/cfd;			make;	mv euler3d euler3d_double pre_euler3d pre_euler3d_double $(CUDA_BIN_DIR)
	cd cuda/gaussian;		make;	mv gaussian $(CUDA_BIN_DIR)
	cd cuda/heartwall;		make;	mv heartwall $(CUDA_BIN_DIR)
	cd cuda/hotspot;		make;	mv hotspot $(CUDA_BIN_DIR)
	cd cuda/kmeans;			make;	mv kmeans $(CUDA_BIN_DIR)
	cd cuda/lavaMD;			make;	mv lavaMD $(CUDA_BIN_DIR)
	cd cuda/leukocyte;		make;	mv CUDA/leukocyte $(CUDA_BIN_DIR)
	cd cuda/lud;			make;	mv cuda/lud_cuda $(CUDA_BIN_DIR)
	cd cuda/nn;				make;	mv nn $(CUDA_BIN_DIR)
	cd cuda/nw;			make;	mv needle $(CUDA_BIN_DIR)
	cd cuda/srad/srad_v1;		make;	mv srad $(CUDA_BIN_DIR)/srad_v1
	cd cuda/srad/srad_v2;		make;   mv srad $(CUDA_BIN_DIR)/srad_v2
	cd cuda/streamcluster;		make;	mv sc_gpu $(CUDA_BIN_DIR)
	cd cuda/particlefilter;		make;	mv particlefilter_naive particlefilter_float $(CUDA_BIN_DIR)       
	cd cuda/pathfinder;		make;	mv pathfinder $(CUDA_BIN_DIR)
	cd cuda/mummergpu;		make;	mv bin/mummergpu $(CUDA_BIN_DIR)
	cd cuda/hybridsort;              make;   mv hybridsort $(CUDA_BIN_DIR)
	cd cuda/dwt2d;                   make;   mv dwt2d  $(CUDA_BIN_DIR)
	
	
OMP:
	cd openmp/backprop;				make;	mv backprop $(OMP_BIN_DIR)
	cd openmp/bfs;					make;	mv bfs $(OMP_BIN_DIR)
	cd openmp/cfd;					make;	mv euler3d_cpu euler3d_cpu_double pre_euler3d_cpu pre_euler3d_cpu_double $(OMP_BIN_DIR)
	cd openmp/heartwall;				make;	mv heartwall $(OMP_BIN_DIR)
	cd openmp/hotspot;				make;	mv hotspot $(OMP_BIN_DIR)
	cd openmp/kmeans/kmeans_openmp;			make;	mv kmeans $(OMP_BIN_DIR)
	cd openmp/lavaMD;				make;	mv lavaMD $(OMP_BIN_DIR)
	cd openmp/leukocyte;				make;	mv OpenMP/leukocyte $(OMP_BIN_DIR)
	cd openmp/lud;					make;	mv omp/lud_omp $(OMP_BIN_DIR)
	cd openmp/nn;					make;	mv nn $(OMP_BIN_DIR)
	cd openmp/nw;					make;	mv needle $(OMP_BIN_DIR)
	cd openmp/srad/srad_v1;				make;	mv srad $(OMP_BIN_DIR)/srad_v1
	cd openmp/srad/srad_v2;				make;   mv srad $(OMP_BIN_DIR)/srad_v2
	cd openmp/streamcluster;			make;	mv sc_omp $(OMP_BIN_DIR)
	cd openmp/particlefilter;			make;	mv particle_filter $(OMP_BIN_DIR)
	cd openmp/pathfinder;				make;	mv pathfinder $(OMP_BIN_DIR)
	cd openmp/mummergpu;				make;	mv bin/mummergpu $(OMP_BIN_DIR)

OPENCL:
	cd opencl/backprop;			make;	mv backprop     $(OPENCL_BIN_DIR)
	cd opencl/bfs;				make;	mv bfs		$(OPENCL_BIN_DIR)
	cd opencl/b+tree;			make;	mv b+tree	$(OPENCL_BIN_DIR)
	cd opencl/cfd;				make;	mv euler3d	$(OPENCL_BIN_DIR)
	cd opencl/hotspot;			make;	mv hotspot	$(OPENCL_BIN_DIR)
	cd opencl/kmeans;			make;	mv kmeans	$(OPENCL_BIN_DIR)
	cd opencl/lavaMD;			make;	mv lavaMD	$(OPENCL_BIN_DIR)
	cd opencl/leukocyte;			make;	mv OpenCL/leukocyte	$(OPENCL_BIN_DIR)
	cd opencl/lud/ocl;			make;	mv lud		$(OPENCL_BIN_DIR)
	cd opencl/myocyte;			make;	mv myocyte	$(OPENCL_BIN_DIR)
	cd opencl/nw;				make;	mv nw		$(OPENCL_BIN_DIR)
	cd opencl/srad;				make;	mv srad		$(OPENCL_BIN_DIR)
	cd opencl/streamcluster;		make;	mv streamcluster	$(OPENCL_BIN_DIR)
	cd opencl/pathfinder;			make;	mv pathfinder	$(OPENCL_BIN_DIR)
	cd opencl/particlefilter;		make;	mv OCL_particlefilter_naive OCL_particlefilter_single OCL_particlefilter_double $(OPENCL_BIN_DIR)
	cd opencl/gaussian;			make;	mv gaussian	$(OPENCL_BIN_DIR)
	cd opencl/nn;				make;	mv nn	$(OPENCL_BIN_DIR)
	cd opencl/heartwall;		        make;	mv heartwall	$(OPENCL_BIN_DIR)
	cd opencl/hybridsort;              	make;   mv hybridsort $(CUDA_BIN_DIR)
	cd opencl/dwt2d;                   	make;   mv dwt2d  $(CUDA_BIN_DIR)
	
clean: CUDA_clean OMP_clean OCL_clean

CUDA_clean:
	cd $(CUDA_BIN_DIR); rm -f *
	for dir in $(CUDA_DIRS) ; do cd cuda/$$dir ; make clean ; cd ../.. ; done
	
OMP_clean:
	cd $(OMP_BIN_DIR); rm -f *
	for dir in $(OMP_DIRS) ; do cd openmp/$$dir ; make clean ; cd ../.. ; done

OCL_clean:
	cd $(OPENCL_BIN_DIR); rm -f *
	for dir in $(OCL_DIRS) ; do cd opencl/$$dir ; make clean ; cd ../.. ; done
