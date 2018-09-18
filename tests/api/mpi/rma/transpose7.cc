/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* This does a transpose with a get operation, fence, and derived
   datatypes. Uses vector and hvector (Example 3.32 from MPI 1.1
   Standard). Run on 2 processes */

#define TRANSPOSE7_NROWS 1000
#define TRANSPOSE7_NCOLS 1000

namespace transpose7 {
int transpose7(int argc, char *argv[])
{
    int rank, nprocs, **A, *A_data, i, j;
    MPI_Comm CommDeuce;
    MPI_Win win;
    MPI_Datatype column, xpose;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_split(MPI_COMM_WORLD, (rank < 2), rank, &CommDeuce);

    if (rank < 2) {
        A_data = (int *) malloc(TRANSPOSE7_NROWS * TRANSPOSE7_NCOLS * sizeof(int));
        A = (int **) malloc(TRANSPOSE7_NROWS * sizeof(int *));

        A[0] = A_data;
        for (i = 1; i < TRANSPOSE7_NROWS; i++)
            A[i] = A[i - 1] + TRANSPOSE7_NCOLS;

        if (rank == 0) {
            for (i = 0; i < TRANSPOSE7_NROWS; i++)
                for (j = 0; j < TRANSPOSE7_NCOLS; j++)
                    A[i][j] = -1;

            /* create datatype for one column */
            MPI_Type_vector(TRANSPOSE7_NROWS, 1, TRANSPOSE7_NCOLS, MPI_INT, &column);
            /* create datatype for matrix in column-major order */
            MPI_Type_hvector(TRANSPOSE7_NCOLS, 1, sizeof(int), column, &xpose);
            MPI_Type_commit(&xpose);

            MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);

            MPI_Win_fence(0, win);

            MPI_Get(&A[0][0], TRANSPOSE7_NROWS * TRANSPOSE7_NCOLS, MPI_INT, 1, 0, 1, xpose, win);

            MPI_Type_free(&column);
            MPI_Type_free(&xpose);

            MPI_Win_fence(0, win);

            for (j = 0; j < TRANSPOSE7_NCOLS; j++) {
                for (i = 0; i < TRANSPOSE7_NROWS; i++) {
                    if (A[j][i] != i * TRANSPOSE7_NCOLS + j) {
                        if (errs < 50) {
                            printf("Error: A[%d][%d]=%d should be %d\n", j, i,
                                   A[j][i], i * TRANSPOSE7_NCOLS + j);
                        }
                        errs++;
                    }
                }
            }
            if (errs >= 50) {
                printf("Total number of errors: %d\n", errs);
            }
        } else {
            for (i = 0; i < TRANSPOSE7_NROWS; i++)
                for (j = 0; j < TRANSPOSE7_NCOLS; j++)
                    A[i][j] = i * TRANSPOSE7_NCOLS + j;

            MPI_Win_create(&A[0][0], TRANSPOSE7_NROWS * TRANSPOSE7_NCOLS * sizeof(int), sizeof(int), MPI_INFO_NULL,
                           CommDeuce, &win);
            MPI_Win_fence(0, win);
            MPI_Win_fence(0, win);
        }
        MPI_Win_free(&win);
        free(A_data);
        free(A);
    }
    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
} // namespace transpose7