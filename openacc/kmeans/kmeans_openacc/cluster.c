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
/**   File:         cluster.c                                           **/
/**   Description:  Takes as input a file, containing 1 data point per  **/
/**                 per line, and performs a fuzzy c-means clustering   **/
/**                 on the data. Fuzzy clustering is performed using    **/
/**                 min to max clusters and the clustering that gets    **/
/**                 the best score according to a compactness and       **/
/**                 separation criterion are returned.                  **/
/**   Author:  Brendan McCane                                           **/
/**            James Cook University of North Queensland.               **/
/**            Australia. email: mccane@cs.jcu.edu.au                   **/
/**                                                                     **/
/**   Edited by: Jay Pisharath, Wei-keng Liao                           **/
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
#include <string.h>
#include <limits.h>
#include <math.h>
#include <float.h>

#include "kmeans.h"

float   min_rmse_ref = FLT_MAX;

/*---< cluster() >-----------------------------------------------------------*/
int cluster(int      npoints,      /* number of input objects */
            int      nfeatures,   /* size of attribute of each object */
            float  **features,      /* [numObjects][numAttributes] */
            int     min_nclusters,
            int      max_nclusters,
            float    threshold,       /* in:  loop terminating factor  */
            int     *best_nclusters,        /* out: number between min and max with lowest RMSE */
            float ***cluster_centres, /* out: [best_nclusters][numAttributes] */
            float   *min_rmse,              /* out: minimum RMSE */
            int      isRMSE,                /* calculate RMSE */
            int      nloops                 /* number of iteration for each number of clusters */
            )
{
    int     nclusters;
    int     index = 0;
    int     rmse;
    int    *membership;
    float **tmp_cluster_centres;
    int     i, j;
    int     *membership_tmp;
    float   *feature_d;

    membership = (int*) malloc(npoints * sizeof(int));

    for(nclusters = min_nclusters; nclusters <= max_nclusters; nclusters++)
    {
        if (nclusters > npoints) break;

        membership_tmp = (int*) malloc(npoints * sizeof(int));

        for(i = 0;i < npoints;i++) {
            membership_tmp[i] = -1;
        }

        feature_d = (float *)malloc(npoints * nfeatures * sizeof(float));

        for (i = 0; i< npoints; i++) {
            for (j = 0; j < nfeatures; j++)
                feature_d[i + j * npoints] = features[i][j];
        }

#pragma acc data copyin(feature_d[0:npoints * nfeatures])
{
        for (i = 0; i < nloops; i++)
        {
            tmp_cluster_centres = kmeans_clustering(features,
                                            feature_d,
                                            nfeatures,
                                            npoints,
                                            nclusters,
                                            threshold,
                                            membership,
                                            membership_tmp
                                            );

            if (*cluster_centres) {
                free((*cluster_centres)[0]);
                free(*cluster_centres);
            }

            *cluster_centres = tmp_cluster_centres;


            if(isRMSE)
            {   
                
                rmse = rms_err(features,
                               nfeatures,
                               npoints,
                               tmp_cluster_centres,
                               nclusters);
                
                if(rmse < min_rmse_ref){
                    min_rmse_ref = rmse;            //update reference min RMSE
                    *min_rmse = min_rmse_ref;       //update return min RMSE
                    *best_nclusters = nclusters;    //update optimum number of clusters
                    index = i;                      //update number of iteration to reach best RMSE
                }
                
            }   

        }
}

        free(membership_tmp);
        free(feature_d);
    }
	

   
    free(membership);

    return index;
}