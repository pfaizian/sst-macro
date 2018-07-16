/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "mpitest.h"

/* transposes a matrix using put, fence, and derived
   datatypes. Uses vector and struct (Example 3.33 from MPI 1.1
   Standard). We could use vector and type_create_resized instead. Run
   on 2 processes */

#define TRANSPOSE2_NROWS 100
#define TRANSPOSE2_NCOLS 100

namespace transpose2 {
int transpose2(int argc, char *argv[])
{
    int rank, nprocs, A[TRANSPOSE2_NROWS][TRANSPOSE2_NCOLS], i, j, blocklen[2];
    MPI_Comm CommDeuce;
    MPI_Aint disp[2];
    MPI_Win win;
    MPI_Datatype column, column1, type[2];
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
        if (rank == 0) {
            for (i = 0; i < TRANSPOSE2_NROWS; i++)
                for (j = 0; j < TRANSPOSE2_NCOLS; j++)
                    A[i][j] = i * TRANSPOSE2_NCOLS + j;

            /* create datatype for one column */
            MPI_Type_vector(TRANSPOSE2_NROWS, 1, TRANSPOSE2_NCOLS, MPI_INT, &column);

            /* create datatype for one column, with the extent of one
             * integer. we could use type_create_resized instead. */
            disp[0] = 0;
            disp[1] = sizeof(int);
            type[0] = column;
            type[1] = MPI_UB;
            blocklen[0] = 1;
            blocklen[1] = 1;
            MPI_Type_struct(2, blocklen, disp, type, &column1);
            MPI_Type_commit(&column1);

            MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, CommDeuce, &win);

            MPI_Win_fence(0, win);

            MPI_Put(A, TRANSPOSE2_NROWS * TRANSPOSE2_NCOLS, MPI_INT, 1, 0, TRANSPOSE2_NCOLS, column1, win);

            MPI_Type_free(&column);
            MPI_Type_free(&column1);

            MPI_Win_fence(0, win);
        } else {        /* rank=1 */
            for (i = 0; i < TRANSPOSE2_NROWS; i++)
                for (j = 0; j < TRANSPOSE2_NCOLS; j++)
                    A[i][j] = -1;
            MPI_Win_create(A, TRANSPOSE2_NROWS * TRANSPOSE2_NCOLS * sizeof(int), sizeof(int), MPI_INFO_NULL, CommDeuce,
                           &win);
            MPI_Win_fence(0, win);

            MPI_Win_fence(0, win);

            for (j = 0; j < TRANSPOSE2_NCOLS; j++) {
                for (i = 0; i < TRANSPOSE2_NROWS; i++) {
                    if (A[j][i] != i * TRANSPOSE2_NCOLS + j) {
                        if (errs < 50) {
                            printf("Error: A[%d][%d]=%d should be %d\n", j, i,
                                   A[j][i], i * TRANSPOSE2_NCOLS + j);
                        }
                        errs++;
                    }
                }
            }
            if (errs >= 50) {
                printf("Total number of errors: %d\n", errs);
            }
        }
        MPI_Win_free(&win);
    }

    MPI_Comm_free(&CommDeuce);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
} // namespace transpose2
