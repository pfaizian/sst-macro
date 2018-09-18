/**
Copyright 2009-2018 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2018, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <sprockit/stl_string.h>
#include <sumi-mpi/mpi_api.h> 
#include <iostream>

namespace sumi {

int mpi_api::win_flush(int rank, MPI_Win win) {
  spkt_abort_printf("unimplemented error: MPI_Win_flush");
  return MPI_SUCCESS;
}

int mpi_api::win_flush_local(int rank, MPI_Win win) {
  spkt_abort_printf("unimplemented error: MPI_Win_flush_local");
  return MPI_SUCCESS;
}

int mpi_api::win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info,
                        MPI_Comm comm, MPI_Win *win) {
  spkt_abort_printf("unimplemented error: MPI_Win_create");
  return MPI_SUCCESS;
}

int mpi_api::win_free(MPI_Win *win) {
  spkt_abort_printf("unimplemented error: MPI_Win_free");
  return MPI_SUCCESS;
}

int mpi_api::win_lock(int lock_type, int rank, int assert, MPI_Win win) {
  spkt_abort_printf("unimplemented error: MPI_Win_lock");
  return MPI_SUCCESS;
}

int mpi_api::win_fence(int lock_type, int rank, int assert, MPI_Win *win) {
  spkt_abort_printf("unimplemented error: MPI_Win_fence");
  return MPI_SUCCESS;
}

int mpi_api::accumulate(void *origin, int origin_count,
                        MPI_Datatype origin_datatype, int target_rank,
                        MPI_Aint target_disp, int target_count,
                        MPI_Datatype target_datatype, MPI_Op op, MPI_Win win) {
  spkt_abort_printf("unimplemented error: MPI_Accumulate");
  return MPI_SUCCESS;
}

int mpi_api::win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn,
                               MPI_Win_delete_attr_function *win_delete_attr_fn,
                               int *win_keyval, void *extra_state) {
  spkt_abort_printf("unimplemented error: MPI_Win_create_keyval");
  return MPI_SUCCESS;
}

int mpi_api::win_free_keyval(int *win_keyval) {
  spkt_abort_printf("unimplemented error: MPI_Win_free_keyval");
  return MPI_SUCCESS;
}

int mpi_api::info_create(MPI_Info *info) {
  spkt_abort_printf("unimplemented error: MPI_Info_create");
  return MPI_SUCCESS;
}

int mpi_api::info_set(MPI_Info *info, char* key, char *value) {
  spkt_abort_printf("unimplemented error: MPI_Info_set");
  return MPI_SUCCESS;
}

int mpi_api::info_free(MPI_Info *info) {
  spkt_abort_printf("unimplemented error: MPI_Info_free");
  return MPI_SUCCESS;
}

int mpi_api::win_delete_attr(MPI_Win win, int win_keyval) {
  spkt_abort_printf("unimplemented error: MPI_Win_delete_attr");
  return MPI_SUCCESS;
}

int mpi_api::win_set_attr(MPI_Win win, int win_keyval,
                                   void *attribute_val) {
  spkt_abort_printf("unimplemented error: MPI_Win_set_attr");
  return MPI_SUCCESS;
}

int mpi_api::win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag) {
  spkt_abort_printf("unimplemented error: MPI_Win_get_attr");
  return MPI_SUCCESS;
}

// For now we will ignore the info handle
int mpi_api::alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr) {
  // baseptr = malloc(size); // Does not work
  *((void **) baseptr) = malloc(size);
  return MPI_SUCCESS;
}

// If we start to do something special in alloc_mem we may have to modify our
// call to free_mem
int mpi_api::free_mem(void *baseptr) {
  free(baseptr);
  return MPI_SUCCESS;
}

int mpi_api::win_unlock(int rank, MPI_Win win) {
  spkt_abort_printf("unimplemented error: MPI_Win_unlock");
  return MPI_SUCCESS;
}

int mpi_api::win_set_name(MPI_Win win, const char *win_name) {
  spkt_abort_printf("unimplemented error: MPI_Win_set_name");
  return MPI_SUCCESS;
}

int mpi_api::win_get_name(MPI_Win win, char *win_name, int *resultlen) {
  spkt_abort_printf("unimplemented error: MPI_Win_set_name");
  return MPI_SUCCESS;
}

int mpi_api::get(void *origin_addr, int origin_count,
                 MPI_Datatype origin_datatype, int target_rank,
                 MPI_Aint target_disp, int target_count,
                 MPI_Datatype target_datatype, MPI_Win win) {
  spkt_abort_printf("unimplemented error: MPI_Get");
  return MPI_SUCCESS;
}

int mpi_api::put(const void *origin_addr, int origin_count,
                 MPI_Datatype origin_datatype, int target_rank,
                 MPI_Aint target_disp, int target_count,
                 MPI_Datatype target_datatype, MPI_Win win) {
  spkt_abort_printf("unimplemented error: MPI_Put");
  return MPI_SUCCESS;
}

}  // namespace sumi
