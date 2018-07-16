/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "mpitest.h"

/* This does a local transpose-cum-accumulate operation. Uses
   vector and hvector datatypes (Example 3.32 from MPI 1.1
   Standard). Run on 1 process. */

#define TRANSPOSE6_NROWS 100
#define TRANSPOSE6_NCOLS 100

namespace transpose6 {
int transpose6(int argc, char *argv[])
{
    int rank, nprocs, A[TRANSPOSE6_NROWS][TRANSPOSE6_NCOLS], B[TRANSPOSE6_NROWS][TRANSPOSE6_NCOLS], i, j;
    MPI_Win win;
    MPI_Datatype column, xpose;
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (rank == 0) {
        for (i = 0; i < TRANSPOSE6_NROWS; i++)
            for (j = 0; j < TRANSPOSE6_NCOLS; j++)
                A[i][j] = B[i][j] = i * TRANSPOSE6_NCOLS + j;

        /* create datatype for one column */
        MPI_Type_vector(TRANSPOSE6_NROWS, 1, TRANSPOSE6_NCOLS, MPI_INT, &column);
        /* create datatype for matrix in column-major order */
        MPI_Type_hvector(TRANSPOSE6_NCOLS, 1, sizeof(int), column, &xpose);
        MPI_Type_commit(&xpose);

        MPI_Win_create(B, TRANSPOSE6_NROWS * TRANSPOSE6_NCOLS * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_SELF,
                       &win);

        MPI_Win_fence(0, win);

        MPI_Accumulate(A, TRANSPOSE6_NROWS * TRANSPOSE6_NCOLS, MPI_INT, 0, 0, 1, xpose, MPI_SUM, win);

        MPI_Type_free(&column);
        MPI_Type_free(&xpose);

        MPI_Win_fence(0, win);

        for (j = 0; j < TRANSPOSE6_NCOLS; j++) {
            for (i = 0; i < TRANSPOSE6_NROWS; i++) {
                if (B[j][i] != i * TRANSPOSE6_NCOLS + j + j * TRANSPOSE6_NCOLS + i) {
                    if (errs < 20) {
                        printf("Error: B[%d][%d]=%d should be %d\n", j, i,
                               B[j][i], i * TRANSPOSE6_NCOLS + j + j * TRANSPOSE6_NCOLS + i);
                    }
                    errs++;
                }
            }
        }
        if (errs >= 20) {
            printf("Total number of errors: %d\n", errs);
        }

        MPI_Win_free(&win);
    }
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
} // namespace transpose6
