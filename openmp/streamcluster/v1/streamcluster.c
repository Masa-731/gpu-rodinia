/***********************************************
	streamcluster_omp.cpp
	: parallelized code of streamcluster using OpenMP
	
	- original code from PARSEC Benchmark Suite
	- parallelization with OpenMP API has been applied by
	
	Sang-Ha (a.k.a Shawn) Lee - sl4ge@virginia.edu
	University of Virginia
	Department of Electrical and Computer Engineering
	Department of Computer Science
	
***********************************************/

#include "streamcluster.h"

double gettime() {
  struct timeval t;
  gettimeofday(&t,NULL);
  return (double)t.tv_sec+t.tv_usec*1e-6;
}

int isIdentical(float *i, float *j, int D)
// tells whether two points of D dimensions are identical
{
  int a = 0;
  int equal = 1;

  while (equal && a < D) {
    if (i[a] != j[a]) equal = 0;
    else a++;
  }
  if (equal) return 1;
  else return 0;

}

/* comparator for floating point numbers */
static int floatcomp(const void *i, const void *j)
{
  float a, b;
  a = *(float *)(i);
  b = *(float *)(j);
  if (a > b) return (1);
  if (a < b) return (-1);
  return(0);
}

/* shuffle points into random order */
void shuffle(Points *points)
{
#ifdef PROFILE
  double t1 = gettime();
#endif
  long i, j;
  Point temp;
  for (i=0;i<points->num-1;i++) {
    j=(lrand48()%(points->num - i)) + i;
    temp = points->p[i];
    points->p[i] = points->p[j];
    points->p[j] = temp;
  }
#ifdef PROFILE
  double t2 = gettime();
  time_shuffle += t2-t1;
#endif
}

/* shuffle an array of integers */
void intshuffle(int *intarray, int length)
{
#ifdef PROFILE
  double t1 = gettime();
#endif
  long i, j;
  int temp;
  for (i=0;i<length;i++) {
    j=(lrand48()%(length - i))+i;
    temp = intarray[i];
    intarray[i]=intarray[j];
    intarray[j]=temp;
  }
#ifdef PROFILE
  double t2 = gettime();
  time_shuffle += t2-t1;
#endif
}

#ifdef INSERT_WASTE
double waste(double s )
{
  for( int i =0 ; i< 4; i++ ) {
    s += pow(s,0.78);
  }
  return s;
}
#endif

/* compute Euclidean distance squared between two points */
float dist(Point p1, Point p2, int dim)
{
  int i;
  float result=0.0;
  for (i=0;i<dim;i++)
    result += (p1.coord[i] - p2.coord[i])*(p1.coord[i] - p2.coord[i]);
#ifdef INSERT_WASTE
  double s = waste(result);
  result += s;
  result -= s;
#endif
  return(result);
}

float dist_acc(int p1, int p2, int num, int dim, float *coord)
{
    float retval = 0.0;
    for (int i = 0; i < dim; i++) {
        float tmp = coord[(i*num)+p1] - coord[(i*num)+p2];
        retval += tmp * tmp;
    }
    return retval;
}

/* run speedy on the points, return total cost of solution */
float pspeedy(Points *points, float z, long *kcenter, int pid)
{
#ifdef PROFILE
  double t1 = gettime();
#endif

  //my block
  long bsize = points->num/nproc;
  long k1 = bsize * pid;
  long k2 = k1 + bsize;
  if( pid == nproc-1 ) k2 = points->num;

  static double totalcost;

  static bool open = false;
  static double* costs; //cost for each thread. 
  static int i;

#ifdef PRINTINFO
  if( pid == 0 ){
    fprintf(stderr, "Speedy: facility cost %lf\n", z);
  }
#endif

  /* create center at first point, send it to itself */
  for( int k = k1; k < k2; k++ )    {
    float distance = dist(points->p[k],points->p[0],points->dim);
    points->p[k].cost = distance * points->p[k].weight;
    points->p[k].assign=0;
  }

  if( pid==0 )   {
    *kcenter = 1;
    costs = (double*)malloc(sizeof(double)*nproc);
  }
    
  if( pid != 0 ) { // we are not the master threads. we wait until a center is opened.
    while(1) {
      if( i >= points->num ) break;
      for( int k = k1; k < k2; k++ ) {
        float distance = dist(points->p[i],points->p[k],points->dim);
    	  if( distance*points->p[k].weight < points->p[k].cost ) {
          points->p[k].cost = distance * points->p[k].weight;
          points->p[k].assign=i;
        }
      }
    }
  }
  else { // I am the master thread. I decide whether to open a center and notify others if so. 
    for(i = 1; i < points->num; i++ ) {

      bool to_open = ((float)lrand48()/(float)INT_MAX)<(points->p[i].cost/z);

      if( to_open ) {

        (*kcenter)++;
        open = true;

        for( int k = k1; k < k2; k++ ) {
          float distance = dist(points->p[i],points->p[k],points->dim);

          if( distance*points->p[k].weight < points->p[k].cost ) {
      	    points->p[k].cost = distance * points->p[k].weight;
      	    points->p[k].assign=i;
          }
        }

        open = false;

      }
    }

    open = true;
  }

  open = false;
  double mytotal = 0;
  for( int k = k1; k < k2; k++ )  {
    mytotal += points->p[k].cost;
  }
  costs[pid] = mytotal;

  // aggregate costs from each thread
  if( pid == 0 ) {

    totalcost=z*(*kcenter);

    for( int i = 0; i < nproc; i++ ) {
      totalcost += costs[i];
    } 
    
    free(costs);
  }

#ifdef PRINTINFO
  if( pid == 0 )
    {
      fprintf(stderr, "Speedy opened %d facilities for total cost %lf\n", *kcenter, totalcost);
      fprintf(stderr, "Distance Cost %lf\n", totalcost - z*(*kcenter));
    }
#endif

#ifdef PROFILE
  double t2 = gettime();
  if( pid== 0 ) {
    time_speedy += t2 -t1;
  }
#endif
  return(totalcost);
}


/* For a given point x, find the cost of the following operation:
 * -- open a facility at x if there isn't already one there,
 * -- for points y such that the assignment distance of y exceeds dist(y, x),
 *    make y a member of x,
 * -- for facilities y such that reassigning y and all its members to x 
 *    would save cost, realize this closing and reassignment.
 * 
 * If the cost of this operation is negative (i.e., if this entire operation
 * saves cost), perform this operation and return the amount of cost saved;
 * otherwise, do nothing.
 */

/* numcenters will be updated to reflect the new number of centers */
/* z is the facility cost, x is the number of this point in the array 
   points */

double pgain(long x, Points *points, double z, long int *numcenters, float *coord)
{
  //  printf("pgain pthread %d begin\n",pid);
#ifdef PROFILE
  double t0 = gettime();
#endif	


  int stride  = *numcenters + 1;      // size of each work_mem segment
  int K   = *numcenters ;       // number of centers
  int num   =  points->num;       // number of points
  int dim   =  points->dim;       // number of dimension
  int nThread =  num;           // number of threads == number of data points

  int i, j;

  float *work_mem = (float*) malloc(stride * (nThread + 1) * sizeof(float));
  Point_Struct *p = (Point_Struct*)malloc(num * sizeof(Point_Struct));
  
  int count = 0;
  for(i = 0; i < num; i++ ) {
    if( is_center[i] ) {
      center_table[i] = count++;
    }   
  } 

  for (i = 0; i < num; i++) {
    p[i].weight = ((points->p)[i]).weight;
    p[i].assign = ((points->p)[i]).assign;
    p[i].cost = ((points->p)[i]).cost;
  }
  
  //now we finish building the table. clear the working memory.
  
  memset(switch_membership, 0, num * sizeof(bool));
  memset(work_mem, 0, stride * (nThread+1) * sizeof(float));
#ifdef PROFILE
  double t1 = gettime();
  time_gain_init += t1-t0;
#endif

#pragma omp parallel for private(j)
  for (i = 0; i < num; i++ ) {
      /*
    float coord_c[256];
    for (j = 0; j < dim; j++){
        coord_c[j] = coord[j*num + x];
    }
    */
    //double *lower = &work_mem[i*stride];
    //float x_cost = dist_acc(i, x, num, dim, coord) * p[i].weight;

    float x_cost = 0.0;
    for (j = 0; j < dim; j++) {
        float tmp = coord[(j*num) + i] - coord[(j*num)+x];
        /*float tmp = coord[(j*num) + i] - coord_c[i];*/
        x_cost += tmp * tmp;
    }
    x_cost += p[i].weight;

    float current_cost = p[i].cost;
    int base = i*stride;
        
    if ( x_cost < current_cost ) {	
      switch_membership[i] = 1;
      int addr_1 = base + K;
      work_mem[addr_1] = x_cost - current_cost;
      //lower[K] += x_cost - current_cost;

    } else {

      int assign = p[i].assign;
      int addr_2 = base + center_table[assign];
      work_mem[addr_2] += current_cost - x_cost;
      //lower[center_table[assign]] += current_cost - x_cost;			
    }
  }

#ifdef PROFILE
  double t2 = gettime();
  
  time_gain_dist += t2 - t1;
#endif	

  int number_of_centers_to_close = 0;
  float gl_cost_of_opening_x = z;
  float *gl_lower = &work_mem[stride * nThread];

  for (i = 0; i < num; i++ ) {
    if( is_center[i] ) {

      double low = z;

      for(j = 0; j < num; j++ ) {
				low += work_mem[j*stride + center_table[i]];
      }

      gl_lower[center_table[i]] = low;

      if ( low > 0 ) {

        ++number_of_centers_to_close;  
        work_mem[i*stride+K] -= low;
      }
    }
    gl_cost_of_opening_x += work_mem[i*stride+K];
  }
	
  if ( gl_cost_of_opening_x < 0 ) {
    //  we'd save money by opening x; we'll do it
    for (i = 0; i < num; i++) {

      bool close_center = gl_lower[center_table[points->p[i].assign]] > 0;

      if ( switch_membership[i] || close_center ) {
				points->p[i].cost = dist(points->p[i], points->p[x], dim) * points->p[i].weight;
        points->p[i].assign = x;
      }
    }
		
    for(i = 0; i < num; i++) {
      if( is_center[i] && gl_lower[center_table[i]] > 0 ) {
				is_center[i] = false;
      }
    }

    if( x >= 0 && x < num ) {
      is_center[x] = true;
    }

    *numcenters = *numcenters + 1 - number_of_centers_to_close;
  }
  else {
      gl_cost_of_opening_x = 0;  // the value we'll return
  }


  free(p);
  free(work_mem);

#ifdef PROFILE
  double t3 = gettime();
  time_gain += t3-t0;
#endif
  iter++;
  return -gl_cost_of_opening_x;
}


/* facility location on the points using local search */
/* z is the facility cost, returns the total cost and # of centers */
/* assumes we are seeded with a reasonable solution */
/* cost should represent this solution's cost */
/* halt if there is < e improvement after iter calls to gain */
/* feasible is an array of numfeasible points which may be centers */

float pFL(Points *points, int *feasible, int numfeasible,
	  float z, long *k, double cost, long iter, float e, 
	  int pid)
{
  long i;
  long x;
  double change;
  long numberOfPoints;

  change = cost;
  /* continue until we run iter iterations without improvement */
  /* stop instead if improvement is less than e */
  int num   =  points->num;
  int dim   =  points->dim; 
  float *coord = (float*) malloc(num * dim * sizeof(float));

  for(int j=0; j<dim; j++)
  {
      for(int k=0; k<num; k++)
      {
        coord[(num*j)+k] = points->p[k].coord[j];
      }
  }

#pragma acc data copyin(coord[0:dim*num])
{
  while (change/cost > 1.0*e) {
    change = 0.0;
    numberOfPoints = points->num;
    /* randomize order in which centers are considered */

    if( pid == 0 ) {
      intshuffle(feasible, numfeasible);
    }
    
    //acc_init(acc_device_nvidia);
    //acc_set_device_num(0, acc_device_nvidia);

    for (i=0;i<iter;i++) {
      x = i%numfeasible;
			//printf("iteration %d started********\n", i);
      change += pgain(feasible[x], points, z, k, coord);
			c++;
			//printf("iteration %d finished @@@@@@\n", i);
    }

    cost -= change;
#ifdef PRINTINFO
    if( pid == 0 ) {
      fprintf(stderr, "%d centers, cost %lf, total distance %lf\n",
	      *k, cost, cost - z*(*k));
    }
#endif

  }
}
  free(coord);
  return(cost);
}

int selectfeasible_fast(Points *points, int **feasible, int kmin, int pid)
{
#ifdef PROFILE
  double t1 = gettime();
#endif

  int numfeasible = points->num;
  if (numfeasible > (ITER*kmin*log((double)kmin)))
    numfeasible = (int)(ITER*kmin*log((double)kmin));
  *feasible = (int *)malloc(numfeasible*sizeof(int));
  
  float* accumweight;
  float totalweight;

  /* 
     Calcuate my block. 
     For now this routine does not seem to be the bottleneck, so it is not parallelized. 
     When necessary, this can be parallelized by setting k1 and k2 to 
     proper values and calling this routine from all threads ( it is called only
     by thread 0 for now ). 
     Note that when parallelized, the randomization might not be the same and it might
     not be difficult to measure the parallel speed-up for the whole program. 
   */
  //  long bsize = numfeasible;
  long k1 = 0;
  long k2 = numfeasible;

  float w;
  int l,r,k;

  /* not many points, all will be feasible */
  if (numfeasible == points->num) {
    for (int i=k1;i<k2;i++)
      (*feasible)[i] = i;
    return numfeasible;
  }

  accumweight= (float*)malloc(sizeof(float)*points->num);
  accumweight[0] = points->p[0].weight;
  totalweight=0;
  for( int i = 1; i < points->num; i++ ) {
    accumweight[i] = accumweight[i-1] + points->p[i].weight;
  }
  totalweight=accumweight[points->num-1];

  for(int i=k1; i<k2; i++ ) {
    w = (lrand48()/(float)INT_MAX)*totalweight;
    //binary search
    l=0;
    r=points->num-1;
    if( accumweight[0] > w )  { 
      (*feasible)[i]=0; 
      continue;
    }
    while( l+1 < r ) {
      k = (l+r)/2;
      if( accumweight[k] > w ) {
        r = k;
      } 
      else {
        l = k;
      }
    }
    (*feasible)[i]=r;
  }

  free(accumweight); 

#ifdef PROFILE
  double t2 = gettime();
  time_select_feasible += t2-t1;
#endif
  return numfeasible;
}

/* compute approximate kmedian on the points */
float pkmedian(Points *points, long kmin, long kmax, long* kfinal,
	       int pid)
{
  int i;
  double cost;
  double lastcost;
  double hiz, loz, z;

  static long k;
  static int *feasible;
  static int numfeasible;
  static double* hizs;

  if( pid==0 ) hizs = (double*)calloc(nproc,sizeof(double));
  hiz = loz = 0.0;
  long numberOfPoints = points->num;
  long ptDimension = points->dim;

  //my block
  long bsize = points->num/nproc;
  long k1 = bsize * pid;
  long k2 = k1 + bsize;
  if( pid == nproc-1 ) k2 = points->num;

#ifdef PRINTINFO
  if( pid == 0 )
    {
      printf("Starting Kmedian procedure\n");
      printf("%i points in %i dimensions\n", numberOfPoints, ptDimension);
    }
#endif

  double myhiz = 0;
  for (long kk=k1;kk < k2; kk++ ) {
    myhiz += dist(points->p[kk], points->p[0],
		      ptDimension)*points->p[kk].weight;
  }
  hizs[pid] = myhiz;

  for( int i = 0; i < nproc; i++ )   {
    hiz += hizs[i];
  }

  loz=0.0; z = (hiz+loz)/2.0;
  /* NEW: Check whether more centers than points! */
  if (points->num <= kmax) {
    /* just return all points as facilities */
    for (long kk=k1;kk<k2;kk++) {
      points->p[kk].assign = kk;
      points->p[kk].cost = 0;
    }
    cost = 0;
    if( pid== 0 ) {
      free(hizs); 
      *kfinal = k;
    }
    return cost;
  }

  if( pid == 0 ) shuffle(points);
  cost = pspeedy(points, z, &k, pid);

#ifdef PRINTINFO
  if( pid == 0 )
    printf("thread %d: Finished first call to speedy, cost=%lf, k=%i\n",pid,cost,k);
#endif
  i=0;
  /* give speedy SP chances to get at least kmin/2 facilities */
  while ((k < kmin)&&(i<SP)) {
    cost = pspeedy(points, z, &k, pid);
    i++;
  }

#ifdef PRINTINFO
  if( pid==0)
    printf("thread %d: second call to speedy, cost=%lf, k=%d\n",pid,cost,k);
#endif 
  /* if still not enough facilities, assume z is too high */
  while (k < kmin) {
#ifdef PRINTINFO
    if( pid == 0 ) {
      printf("%lf %lf\n", loz, hiz);
      printf("Speedy indicates we should try lower z\n");
    }
#endif
    if (i >= SP) {hiz=z; z=(hiz+loz)/2.0; i=0;}
    if( pid == 0 ) shuffle(points);
    cost = pspeedy(points, z, &k, pid);
    i++;
  }

  /* now we begin the binary search for real */
  /* must designate some points as feasible centers */
  /* this creates more consistancy between FL runs */
  /* helps to guarantee correct # of centers at the end */
  
  if( pid == 0 )
    {
      numfeasible = selectfeasible_fast(points,&feasible,kmin,pid);
      for( int i = 0; i< points->num; i++ ) {
        is_center[points->p[i].assign]= true;
      }
    }

  while(1) {
		d++;
#ifdef PRINTINFO
    if( pid==0 )
    {
      printf("loz = %lf, hiz = %lf\n", loz, hiz);
      printf("Running Local Search...\n");
    }
#endif
    /* first get a rough estimate on the FL solution */

    lastcost = cost;
    cost = pFL(points, feasible, numfeasible, z, &k, cost, (long)(ITER*kmax*log((double)kmax)), 0.1, pid);

    /* if number of centers seems good, try a more accurate FL */
    if (((k <= (1.1)*kmax)&&(k >= (0.9)*kmin))|| ((k <= kmax+2)&&(k >= kmin-2))) {

#ifdef PRINTINFO
      if( pid== 0)
      {
        printf("Trying a more accurate local search...\n");
      }
#endif
      /* may need to run a little longer here before halting without
	 improvement */
      cost = pFL(points, feasible, numfeasible, z, &k, cost, (long)(ITER*kmax*log((double)kmax)), 0.001, pid);
    }

    if (k > kmax) {
      /* facilities too cheap */
      /* increase facility cost and up the cost accordingly */
      loz = z; z = (hiz+loz)/2.0;
      cost += (z-loz)*k;
    }
    if (k < kmin) {
      /* facilities too expensive */
      /* decrease facility cost and reduce the cost accordingly */
      hiz = z; z = (hiz+loz)/2.0;
      cost += (z-hiz)*k;
    }

    /* if k is good, return the result */
    /* if we're stuck, just give up and return what we have */
    if (((k <= kmax)&&(k >= kmin))||((loz >= (0.999)*hiz)) )
      { 
	break;
      }
  }

  //clean up...
  if( pid==0 ) {
    free(feasible); 
    free(hizs);
    *kfinal = k;
  }

  return cost;
}

/* compute the means for the k clusters */
int contcenters(Points *points)
{
  long i, ii;
  float relweight;

  for (i=0;i<points->num;i++) {
    /* compute relative weight of this point to the cluster */
    if (points->p[i].assign != i) {
      relweight=points->p[points->p[i].assign].weight + points->p[i].weight;
      relweight = points->p[i].weight/relweight;
      for (ii=0;ii<points->dim;ii++) {
        points->p[points->p[i].assign].coord[ii]*= 1.0-relweight;
        points->p[points->p[i].assign].coord[ii]+= points->p[i].coord[ii]*relweight;
      }
      points->p[points->p[i].assign].weight += points->p[i].weight;
    }
  }
  
  return 0;
}

/* copy centers from points to centers */
void copycenters(Points *points, Points* centers, long* centerIDs, long offset)
{
  long i;
  long k;

  bool *is_a_median = (bool *) calloc(points->num, sizeof(bool));

  /* mark the centers */
  for ( i = 0; i < points->num; i++ ) {
    is_a_median[points->p[i].assign] = 1;
  }

  k=centers->num;

  /* count how many  */
  for ( i = 0; i < points->num; i++ ) {
    if ( is_a_median[i] ) {
      memcpy( centers->p[k].coord, points->p[i].coord, points->dim * sizeof(float));
      centers->p[k].weight = points->p[i].weight;
      centerIDs[k] = i + offset;
      k++;
    }
  }

  centers->num = k;

  free(is_a_median);
}

void* localSearchSub(void* arg_) {

  pkmedian_arg_t* arg= (pkmedian_arg_t*)arg_;
  pkmedian(arg->points,arg->kmin,arg->kmax,arg->kfinal,arg->pid);

  return NULL;
}

void localSearch( Points* points, long kmin, long kmax, long* kfinal ) {
#ifdef PROFILE
  double t1 = gettime();
#endif
    
    pkmedian_arg_t* arg = (pkmedian_arg_t*)malloc(nproc*sizeof(pkmedian_arg_t));

    for( int i = 0; i < nproc; i++ ) {
      arg[i].points = points;
      arg[i].kmin = kmin;
      arg[i].kmax = kmax;
      arg[i].pid = i;
      arg[i].kfinal = kfinal;
      localSearchSub(&arg[0]);

    }

    free(arg);

#ifdef PROFILE
  double t2 = gettime();
  time_local_search += t2-t1;
#endif
 
}

void outcenterIDs( Points* centers, long* centerIDs, char* outfile ) {
  FILE* fp = fopen(outfile, "w");
  if( fp==NULL ) {
    fprintf(stderr, "error opening %s\n",outfile);
    exit(1);
  }
  int* is_a_median = (int*)calloc( sizeof(int), centers->num );
  for( int i =0 ; i< centers->num; i++ ) {
    is_a_median[centers->p[i].assign] = 1;
  }

  for( int i = 0; i < centers->num; i++ ) {
    if( is_a_median[i] ) {
      fprintf(fp, "%u\n", centerIDs[i]);
      fprintf(fp, "%lf\n", centers->p[i].weight);
      for( int k = 0; k < centers->dim; k++ ) {
        fprintf(fp, "%lf ", centers->p[i].coord[k]);
      }
      fprintf(fp,"\n\n");
    }
  }
  fclose(fp);
}

void streamCluster( long n, 
		    long kmin, long kmax, int dim,
		    long chunksize, long centersize, char* outfile )
{
  block = (float*)malloc( chunksize*dim*sizeof(float) );
  float* centerBlock = (float*)malloc(centersize*dim*sizeof(float) );
  long* centerIDs = (long*)malloc(centersize*dim*sizeof(long));

  if( block == NULL ) { 
    fprintf(stderr,"not enough memory for a chunk!\n");
    exit(1);
  }

  Points points;
  points.dim = dim;
  points.num = chunksize;
  points.p = (Point *)malloc(chunksize*sizeof(Point));
  for( int i = 0; i < chunksize; i++ ) {		
    points.p[i].coord = &block[i*dim];
  }

  Points centers;
  centers.dim = dim;
  centers.p = (Point *)malloc(centersize*sizeof(Point));
  centers.num = 0;

  for( int i = 0; i< centersize; i++ ) {
    centers.p[i].coord = &centerBlock[i*dim];
    centers.p[i].weight = 1.0;
  }

  long IDoffset = 0;
  long kfinal;

  while(1) {

    //size_t numRead  = stream->read(block, dim, chunksize);
    size_t numRead = 0;

    for( int i = 0; i < chunksize && n > 0; i++ ) {
      for( int k = 0; k < dim; k++ ) {
        block[i*dim + k] = lrand48()/(float)INT_MAX;
      }
      n--;
      numRead++;
    }

    fprintf(stderr,"read %d points\n",numRead);

    if(numRead < (unsigned int)chunksize && n > 0 ) {
      fprintf(stderr, "error reading data!\n");
      exit(1);
    }

    points.num = numRead;
    for( int i = 0; i < points.num; i++ ) {
      points.p[i].weight = 1.0;
    }

    switch_membership = (bool*)malloc(points.num*sizeof(bool));
    is_center = (bool*)calloc(points.num,sizeof(bool));
    center_table = (int*)malloc(points.num*sizeof(int));

    localSearch(&points,kmin, kmax,&kfinal);

    fprintf(stderr,"finish local search\n");
    contcenters(&points);
    isCoordChanged = true;
    if( kfinal + centers.num > centersize ) {
      //here we don't handle the situation where # of centers gets too large. 
      fprintf(stderr,"oops! no more space for centers\n");
      exit(1);
    }

#ifdef PRINTINFO
    printf("finish cont center\n");
#endif

    copycenters(&points, &centers, centerIDs, IDoffset);
    IDoffset += numRead;

#ifdef PRINTINFO
    printf("finish copy centers\n"); 
#endif

    free(is_center);
    free(switch_membership);
    free(center_table);

    if(n <= 0) {
      break;
    }
  }

  //finally cluster all temp centers
  switch_membership = (bool*)malloc(centers.num*sizeof(bool));
  is_center = (bool*)calloc(centers.num,sizeof(bool));
  center_table = (int*)malloc(centers.num*sizeof(int));

  localSearch( &centers, kmin, kmax ,&kfinal );
  contcenters(&centers);
  outcenterIDs( &centers, centerIDs, outfile);
}

int main(int argc, char **argv)
{
  char *outfilename = (char *)malloc(MAXNAMESIZE*sizeof(char));
  char *infilename = (char *)malloc(MAXNAMESIZE*sizeof(char));

  long kmin, kmax, n, chunksize, clustersize;
  int dim;
	int numthreads;
	c = 0;
	d = 0;

#ifdef PARSEC_VERSION
#define __PARSEC_STRING(x) #x
#define __PARSEC_XSTRING(x) __PARSEC_STRING(x)
        printf("PARSEC Benchmark Suite Version "__PARSEC_XSTRING(PARSEC_VERSION)"\n");
	fflush(NULL);
#else
        printf("PARSEC Benchmark Suite\n");
	fflush(NULL);
#endif //PARSEC_VERSION
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_bench_begin(__parsec_streamcluster);
#endif

  if (argc<10) {
    fprintf(stderr,"usage: %s k1 k2 d n chunksize clustersize infile outfile nproc\n",
	    argv[0]);
    fprintf(stderr,"  k1:          Min. number of centers allowed\n");
    fprintf(stderr,"  k2:          Max. number of centers allowed\n");
    fprintf(stderr,"  d:           Dimension of each data point\n");
    fprintf(stderr,"  n:           Number of data points\n");
    fprintf(stderr,"  chunksize:   Number of data points to handle per step\n");
    fprintf(stderr,"  clustersize: Maximum number of intermediate centers\n");
    fprintf(stderr,"  infile:      Input file (if n<=0)\n");
    fprintf(stderr,"  outfile:     Output file\n");
    fprintf(stderr,"  nproc:       Number of threads to use\n");
    fprintf(stderr,"\n");
    fprintf(stderr, "if n > 0, points will be randomly generated instead of reading from infile.\n");
    exit(1);
  }
  kmin = atoi(argv[1]);
  kmax = atoi(argv[2]);
  dim = atoi(argv[3]);
  n = atoi(argv[4]);
  chunksize = atoi(argv[5]);
  clustersize = atoi(argv[6]);
  strcpy(infilename, argv[7]);
  strcpy(outfilename, argv[8]);
  nproc = atoi(argv[9]);
	
	nproc = 1;
	
  srand48(SEED);

  double t1 = gettime();


#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif

  isCoordChanged = false;

  streamCluster(n, kmin, kmax, dim, chunksize, clustersize, outfilename);


#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

  //free(coord);
  //free(p);

  double t2 = gettime();
  printf("time = %lf\n",t2-t1);



  printf("time pgain = %lf\n", time_gain);
  printf("time pgain_dist = %lf\n", time_gain_dist);
  printf("time pgain_init = %lf\n", time_gain_init);
  printf("time pselect = %lf\n", time_select_feasible);
  printf("time pspeedy = %lf\n", time_speedy);
  printf("time pshuffle = %lf\n", time_shuffle);
  printf("time localSearch = %lf\n", time_local_search);
	printf("loops=%d\n", d);


#ifdef ENABLE_PARSEC_HOOKS
  __parsec_bench_end();
#endif
  
  return 0;
}