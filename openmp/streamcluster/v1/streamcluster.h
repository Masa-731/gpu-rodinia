#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <sys/resource.h>
#include <limits.h>
#include <openacc.h>

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#define MAXNAMESIZE 1024 // max filename length
#define SEED 1
/* increase this to reduce probability of random error */
/* increasing it also ups running time of "speedy" part of the code */
/* SP = 1 seems to be fine */
#define SP 1 // number of repetitions of speedy must be >=1

/* higher ITER --> more likely to get correct # of centers */
/* higher ITER also scales the running time almost linearly */
#define ITER 3 // iterate ITER* k log k times; ITER >= 1

//#define PRINTINFO //comment this out to disable output
#define PROFILE // comment this out to disable instrumentation code
//#define ENABLE_THREADS  // comment this out to disable threads
//#define INSERT_WASTE //uncomment this to insert waste computation into dist function

#define CACHE_LINE 512 // cache line in byte

typedef int bool;
#define true 1
#define false 0

/* this structure represents a point */
/* these will be passed around to avoid copying coordinates */
typedef struct {
  float weight;
  float *coord;
  long assign;  /* number of point where this one is assigned */
  float cost;  /* cost of that assignment, weight*distance */
} Point;

/* this is the array of points */
typedef struct {
  long num; /* number of points; may not be N if this is a sample */
  int dim;  /* dimensionality */
  Point *p; /* the array itself */
} Points;

typedef struct {
    float weight;
    long assign;
    float cost;
} Point_Struct;

typedef struct
{
  Points* points;
  long kmin;
  long kmax;
  long* kfinal;
  int pid;
} pkmedian_arg_t;

static bool* switch_membership; //whether to switch membership in pgain
static bool* is_center; //whether a point is a center
static int* center_table; //index table of centers
float* block;

//float *work_mem;
//float *coord;
//Point_Struct *p;

bool isCoordChanged;

static int iter = 0;

static int nproc; //# of threads
static int c, d;

// instrumentation code
#ifdef PROFILE
double time_local_search;
double time_speedy;
double time_select_feasible;
double time_gain;
double time_shuffle;
double time_gain_dist;
double time_gain_init;
#endif 
