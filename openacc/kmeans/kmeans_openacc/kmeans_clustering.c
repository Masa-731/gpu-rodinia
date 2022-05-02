/*****************************************************************************/
/*IMPORTANT:  READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.         */
/*By downloading, copying, installing or using the software you agree        */
/*to this license.  If you do not agree to this license, do not download,    */
/*install, copy or use the software.                                         */
/*                                                                           */
/*                                                                           */
/*Copyright (c) 2005 Northwestern University                                 */
/*All rights reserved.                                                       */

/*Redistribution of the software in source and binary forms,                 */
/*with or without modification, is permitted provided that the               */
/*following conditions are met:                                              */
/*                                                                           */
/*1       Redistributions of source code must retain the above copyright     */
/*        notice, this list of conditions and the following disclaimer.      */
/*                                                                           */
/*2       Redistributions in binary form must reproduce the above copyright   */
/*        notice, this list of conditions and the following disclaimer in the */
/*        documentation and/or other materials provided with the distribution.*/ 
/*                                                                            */
/*3       Neither the name of Northwestern University nor the names of its    */
/*        contributors may be used to endorse or promote products derived     */
/*        from this software without specific prior written permission.       */
/*                                                                            */
/*THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS    */
/*IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED      */
/*TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT AND         */
/*FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL          */
/*NORTHWESTERN UNIVERSITY OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,       */
/*INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES          */
/*(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR          */
/*SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)          */
/*HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,         */
/*STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN    */
/*ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             */
/*POSSIBILITY OF SUCH DAMAGE.                                                 */
/******************************************************************************/
/*************************************************************************/
/**   File:         kmeans_clustering.c                                 **/
/**   Description:  Implementation of regular k-means clustering        **/
/**                 algorithm                                           **/
/**   Author:  Wei-keng Liao                                            **/
/**            ECE Department, Northwestern University                  **/
/**            email: wkliao@ece.northwestern.edu                       **/
/**                                                                     **/
/**   Edited by: Jay Pisharath                                          **/
/**              Northwestern University.                               **/
/**                                                                     **/
/**   ================================================================  **/
/**																		**/
/**   Edited by: Sang-Ha  Lee											**/
/**				 University of Virginia									**/
/**																		**/
/**   Description:	No longer supports fuzzy c-means clustering;	 	**/
/**					only regular k-means clustering.					**/
/**					Simplified for main functionality: regular k-means	**/
/**					clustering.											**/
/**                                                                     **/
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "kmeans.h"
#include <omp.h>

#define RANDOM_MAX 2147483647

#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38
#endif

extern double wtime(void);

int find_nearest_point(float  *pt,          /* [nfeatures] */
                       int     nfeatures,
                       float **pts,         /* [npts][nfeatures] */
                       int     npts)
{
    int index, i;
    float min_dist=FLT_MAX;

    /* find the cluster center id with min distance to pt */
    for (i=0; i<npts; i++) {
        float dist;
        dist = euclid_dist_2(pt, pts[i], nfeatures);  /* no need square root */
        if (dist < min_dist) {
            min_dist = dist;
            index    = i;
        }
    }
    return(index);
}

/*----< euclid_dist_2() >----------------------------------------------------*/
/* multi-dimensional spatial Euclid distance square */
__inline
float euclid_dist_2(float *pt1,
                    float *pt2,
                    int    numdims)
{
    int i;
    float ans=0.0;

    for (i=0; i<numdims; i++)
        ans += (pt1[i]-pt2[i]) * (pt1[i]-pt2[i]);

    return(ans);
}

/*----< rms_err(): calculates RMSE of clustering >-------------------------------------*/
float rms_err   (float **feature,         /* [npoints][nfeatures] */
                 int     nfeatures,
                 int     npoints,
                 float **cluster_centres, /* [nclusters][nfeatures] */
                 int     nclusters)
{
    int    i;
    int    nearest_cluster_index;   /* cluster center id with min distance to pt */
    float  sum_euclid = 0.0;        /* sum of Euclidean distance squares */
    float  ret;                     /* return value */
    
    /* calculate and sum the sqaure of euclidean distance*/ 
    /*
    #pragma omp parallel for \
                shared(feature,cluster_centres) \
                firstprivate(npoints,nfeatures,nclusters) \
                private(i, nearest_cluster_index) \
                schedule (static)   
    */
    for (i=0; i<npoints; i++) {
        nearest_cluster_index = find_nearest_point(feature[i], 
                                                    nfeatures, 
                                                    cluster_centres, 
                                                    nclusters);

        sum_euclid += euclid_dist_2(feature[i],
                                    cluster_centres[nearest_cluster_index],
                                    nfeatures);
        
    }   
    /* divide by n, then take sqrt */
    ret = sqrt(sum_euclid / npoints);

    return(ret);
}

/*----< kmeans_clustering() >---------------------------------------------*/
float** kmeans_clustering(float **feature,    /* in: [npoints][nfeatures] */
                            float *feature_d,
                          int     nfeatures,
                          int     npoints,
                          int     nclusters,
                          float   threshold,
                          int    *membership, /* out: [npoints] */
                          int    *membership_tmp
                          ) 
{

    int      i, j, k, n=0, index, loop=0, temp;
    int     *new_centers_len; /* [nclusters]: no. of points in each cluster */
    float    delta;
    float  **clusters;   /* out: [nclusters][nfeatures] */
    float  **new_centers;     /* [nclusters][nfeatures] */
    int *initial;
    int initial_points;
    int c = 0;

    /* allocate space for returning variable clusters[] */
    clusters    = (float**) malloc(nclusters *             sizeof(float*));
    clusters[0] = (float*)  malloc(nclusters * nfeatures * sizeof(float));
    for (i=1; i<nclusters; i++)
        clusters[i] = clusters[i-1] + nfeatures;


    initial = (int *) malloc(npoints * sizeof(int));
    for (i = 0; i < npoints; i++) 
    {
        initial[i] = i;
    }
    initial_points = npoints;

    /* randomly pick cluster centers */
    for (i=0; i<nclusters && initial_points >= 0; i++) {
        //n = (int)rand() % initial_points;     
        
        for (j=0; j<nfeatures; j++)
            clusters[i][j] = feature[initial[n]][j];    // remapped

      
        temp = initial[n];
        initial[n] = initial[initial_points-1];
        initial[initial_points-1] = temp;
        initial_points--;
        n++;
    }
   
    for (i=0; i<npoints; i++)
		membership[i] = -1;

    /* need to initialize new_centers_len and new_centers[0] to all 0 */
    new_centers_len = (int*) calloc(nclusters, sizeof(int));

    new_centers    = (float**) malloc(nclusters *            sizeof(float*));
    new_centers[0] = (float*)  calloc(nclusters * nfeatures, sizeof(float));
    for (i=1; i<nclusters; i++)
        new_centers[i] = new_centers[i-1] + nfeatures;
 

    float *clusters_d;//, *feature_d;
    clusters_d = (float *)malloc(nclusters * nfeatures * sizeof(float));
    //feature_d = (float *)malloc(npoints * nfeatures * sizeof(float));
    
    /*
    for (i = 0; i< npoints; i++) {
        for (j = 0; j < nfeatures; j++)
            feature_d[j * npoints + i] = feature[i][j];
    }
    */

#pragma acc data copyin(feature_d[0:npoints * nfeatures])
{
    do {
		
        delta = 0.0;

        for (i = 0; i< nclusters; i++) {
            for (j = 0; j< nfeatures; j++)
                clusters_d[i * nfeatures + j] = clusters[i][j];
        }

#pragma acc kernels copy(membership_tmp[0:npoints]), copyin(clusters_d[0:nclusters * nfeatures])
{
#pragma acc loop independent, private(j, k, index) 
        for (i = 0; i < npoints; i++) {

            float min_dist = FLT_MAX;
            float dist = 0.0;
            
            for (j = 0; j < nclusters; j++) {
                
                float ans = 0.0;

                for (k = 0; k < nfeatures; k++) {
                   ans += (feature_d[k * npoints + i] - clusters_d[j * nfeatures + k]) *
                            (feature_d[k * npoints + i] - clusters_d[j * nfeatures + k]);
                }
                
                dist = ans;

                if (dist < min_dist) {
                    min_dist = dist;
                    index    = j;
                }
            }

	        membership_tmp[i] = index;
        }
}
        
        for (i = 0; i < npoints; i++)
        {
            int cluster_id = membership_tmp[i];
            new_centers_len[cluster_id]++;
            if (membership_tmp[i] != membership[i])
            {
                delta++;
                membership[i] = membership_tmp[i];
            }

            for (j = 0; j < nfeatures; j++)
            {
                new_centers[cluster_id][j] += feature[i][j];
            }
        }

        

	/* replace old cluster centers with new_centers */
        for (i = 0; i < nclusters; i++) {
            for (j = 0; j < nfeatures; j++) {
                if (new_centers_len[i] > 0)
					clusters[i][j] = new_centers[i][j] / new_centers_len[i];
				new_centers[i][j] = 0.0;   /* set back to 0 */
			}
			new_centers_len[i] = 0;   /* set back to 0 */
		}
        
        c++;
        //delta /= npoints;
    } while ((delta > threshold) && (loop++ < 500));
}
    free(clusters_d);
    //free(feature_d);
    printf("iterated %d times\n", c);
  
    free(new_centers[0]);
    free(new_centers);
    free(new_centers_len);

    return clusters;
}
