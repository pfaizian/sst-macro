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

#include <sumi-mpi/mpi_api.h>

extern "C" int sstmac_init(int* argc, char*** argv){ return sumi::sstmac_mpi()->init(argc, argv); }
extern "C" int sstmac_finalize(){ return sumi::sstmac_mpi()->finalize(); }
extern "C" int sstmac_comm_rank(MPI_Comm comm, int* rank){ return sumi::sstmac_mpi()->comm_rank(comm,rank); }
extern "C" int sstmac_comm_size(MPI_Comm comm, int* size){ return sumi::sstmac_mpi()->comm_size(comm,size); }
extern "C" int sstmac_type_size(MPI_Datatype type, int* size){ return sumi::sstmac_mpi()->type_size(type,size); }
extern "C" int sstmac_initialized(int* flag){ return sumi::sstmac_mpi()->initialized(flag); }
extern "C" int sstmac_finalized(int* flag){ return sumi::sstmac_mpi()->finalized(flag); }
extern "C" int sstmac_buffer_attach(void* buffer, int size){ return sumi::sstmac_mpi()->buffer_attach(buffer,size); }
extern "C" int sstmac_buffer_detach(void* buffer, int* size){ return sumi::sstmac_mpi()->buffer_detach(buffer,size); }
extern "C" int sstmac_init_thread(int* argc, char*** argv, int required, int* provided){ return sumi::sstmac_mpi()->init_thread(argc,argv,required,provided); }
extern "C" int sstmac_errhandler_set(MPI_Comm comm, MPI_Errhandler handler){ return sumi::sstmac_mpi()->errhandler_set(comm,handler); }
extern "C" int sstmac_error_class(int errorcode, int* errorclass){ return sumi::sstmac_mpi()->error_class(errorcode,errorclass); }
extern "C" int sstmac_error_string(int errorcode, char* str, int* resultlen){ return sumi::sstmac_mpi()->error_string(errorcode,str,resultlen); }
extern "C" int sstmac_comm_split(MPI_Comm incomm, int color, int key,
             MPI_Comm* outcomm){ return sumi::sstmac_mpi()->comm_split(incomm,color,key,outcomm); }
extern "C" int sstmac_comm_dup(MPI_Comm input, MPI_Comm* output){ return sumi::sstmac_mpi()->comm_dup(input,output); }
extern "C" int sstmac_comm_create(MPI_Comm input, MPI_Group group,
              MPI_Comm* output){ return sumi::sstmac_mpi()->comm_create(input,group,output); }
extern "C" int sstmac_comm_group(MPI_Comm comm, MPI_Group* grp){ return sumi::sstmac_mpi()->comm_group(comm,grp); }
extern "C" int sstmac_cart_create(MPI_Comm comm_old, int ndims, const int dims[],
              const int periods[], int reorder, MPI_Comm *comm_cart){ return sumi::sstmac_mpi()->cart_create(comm_old,ndims,dims,periods,reorder,comm_cart); }
extern "C" int sstmac_cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[],
                   int coords[]){ return sumi::sstmac_mpi()->cart_get(comm,maxdims,dims,periods,coords); }
extern "C" int sstmac_cartdim_get(MPI_Comm comm, int *ndims){ return sumi::sstmac_mpi()->cartdim_get(comm,ndims); }
extern "C" int sstmac_cart_rank(MPI_Comm comm, const int coords[], int *rank){ return sumi::sstmac_mpi()->cart_rank(comm,coords,rank); }
extern "C" int sstmac_cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source,
             int *rank_dest){ return sumi::sstmac_mpi()->cart_shift(comm,direction,disp,rank_source,rank_dest); }
extern "C" int sstmac_cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]){ return sumi::sstmac_mpi()->cart_coords(comm,rank,maxdims,coords); }
extern "C" int sstmac_comm_free(MPI_Comm* input){ return sumi::sstmac_mpi()->comm_free(input); }
extern "C" int sstmac_comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler){ return sumi::sstmac_mpi()->comm_set_errhandler(comm,errhandler); }
extern "C" int sstmac_group_free(MPI_Group* grp){ return sumi::sstmac_mpi()->group_free(grp); }
extern "C" int sstmac_group_incl(MPI_Group oldgrp,
             int num_ranks,
             const int* ranks,
             MPI_Group* newgrp){ return sumi::sstmac_mpi()->group_incl(oldgrp,num_ranks,ranks,newgrp); }
extern "C" int sstmac_sendrecv(const void* sendbuf, int sendcount,
        MPI_Datatype sendtype, int dest, int sendtag,
        void* recvbuf, int recvcount,
        MPI_Datatype recvtype, int source, int recvtag,
        MPI_Comm comm, MPI_Status* status){ return sumi::sstmac_mpi()->sendrecv(sendbuf,sendcount,sendtype,dest,sendtag,recvbuf,recvcount,recvtype,source,recvtag,comm,status); }
extern "C" int sstmac_send(const void *buf, int count,
           MPI_Datatype datatype, int dest, int tag,
           MPI_Comm comm){ return sumi::sstmac_mpi()->send(buf,count,datatype,dest,tag,comm); }
extern "C" int sstmac_send_init(const void *buf, int count, MPI_Datatype datatype, int dest,
                int tag, MPI_Comm comm, MPI_Request *request){ return sumi::sstmac_mpi()->send_init(buf,count,datatype,dest,tag,comm,request); }
extern "C" int sstmac_isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
            MPI_Comm comm, MPI_Request *request){ return sumi::sstmac_mpi()->isend(buf,count,datatype,dest,tag,comm,request); }
extern "C" int sstmac_recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
           MPI_Comm comm, MPI_Status *status){ return sumi::sstmac_mpi()->recv(buf,count,datatype,source,tag,comm,status); }
extern "C" int sstmac_irecv(void *buf, int count, MPI_Datatype datatype, int source,
            int tag, MPI_Comm comm, MPI_Request *request){ return sumi::sstmac_mpi()->irecv(buf,count,datatype,source,tag,comm,request); }
extern "C" int sstmac_recv_init(void *buf, int count, MPI_Datatype datatype,
      int source, int tag, MPI_Comm comm, MPI_Request *request){ return sumi::sstmac_mpi()->recv_init(buf,count,datatype,source,tag,comm,request); }
extern "C" int sstmac_request_free(MPI_Request* req){ return sumi::sstmac_mpi()->request_free(req); }
extern "C" int sstmac_start(MPI_Request* req){ return sumi::sstmac_mpi()->start(req); }
extern "C" int sstmac_startall(int count, MPI_Request* req){ return sumi::sstmac_mpi()->startall(count,req); }
extern "C" int sstmac_wait(MPI_Request *request, MPI_Status *status){ return sumi::sstmac_mpi()->wait(request,status); }
extern "C" int sstmac_waitall(int count, MPI_Request array_of_requests[],
          MPI_Status array_of_statuses[]){ return sumi::sstmac_mpi()->waitall(count,array_of_requests,array_of_statuses); }
extern "C" int sstmac_waitany(int count, MPI_Request array_of_requests[], int *indx,
          MPI_Status *status){ return sumi::sstmac_mpi()->waitany(count,array_of_requests,indx,status); }
extern "C" int sstmac_waitsome(int incount, MPI_Request array_of_requests[],
           int *outcount, int array_of_indices[],
           MPI_Status array_of_statuses[]){ return sumi::sstmac_mpi()->waitsome(incount,array_of_requests,outcount,array_of_indices,array_of_statuses); }
extern "C" int sstmac_test(MPI_Request *request, int *flag, MPI_Status *status){ return sumi::sstmac_mpi()->test(request,flag,status); }
extern "C" int sstmac_testall(int count, MPI_Request array_of_requests[], int *flag,
          MPI_Status array_of_statuses[]){ return sumi::sstmac_mpi()->testall(count,array_of_requests,flag,array_of_statuses); }
extern "C" int sstmac_testany(int count, MPI_Request array_of_requests[], int *indx,
          int *flag, MPI_Status *status){ return sumi::sstmac_mpi()->testany(count,array_of_requests,indx,flag,status); }
extern "C" int sstmac_testsome(int incount, MPI_Request array_of_requests[], int *outcount,
           int array_of_indices[], MPI_Status array_of_statuses[]){ return sumi::sstmac_mpi()->testsome(incount,array_of_requests,outcount,array_of_indices,array_of_statuses); }
extern "C" int sstmac_probe(int source, int tag, MPI_Comm comm,
         MPI_Status *status){ return sumi::sstmac_mpi()->probe(source,tag,comm,status); }
extern "C" int sstmac_iprobe(int source, int tag, MPI_Comm comm, int* flag,
         MPI_Status *status){ return sumi::sstmac_mpi()->iprobe(source,tag,comm,flag,status); }
extern "C" int sstmac_barrier(MPI_Comm comm){ return sumi::sstmac_mpi()->barrier(comm); }
extern "C" int sstmac_bcast(void *buffer, int count, MPI_Datatype datatype, int root,
        MPI_Comm comm){ return sumi::sstmac_mpi()->bcast(buffer,count,datatype,root,comm); }
extern "C" int sstmac_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
           void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
           MPI_Comm comm){ return sumi::sstmac_mpi()->scatter(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm); }
extern "C" int sstmac_scatterv(const void *sendbuf, const int *sendcounts, const int *displs,
           MPI_Datatype sendtype, void *recvbuf, int recvcount,
           MPI_Datatype recvtype,
           int root, MPI_Comm comm){ return sumi::sstmac_mpi()->scatterv(sendbuf,sendcounts,displs,sendtype,recvbuf,recvcount,recvtype,root,comm); }
extern "C" int sstmac_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
         void *recvbuf, int recvcount, MPI_Datatype recvtype,
         int root, MPI_Comm comm){ return sumi::sstmac_mpi()->gather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm); }
extern "C" int sstmac_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
          void *recvbuf, const int *recvcounts, const int *displs,
          MPI_Datatype recvtype, int root, MPI_Comm comm){ return sumi::sstmac_mpi()->gatherv(sendbuf,sendcount,sendtype,recvbuf,recvcounts,displs,recvtype,root,comm); }
extern "C" int sstmac_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm){ return sumi::sstmac_mpi()->allgather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm); }
extern "C" int sstmac_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
             void *recvbuf, const int *recvcounts, const int *displs,
             MPI_Datatype recvtype, MPI_Comm comm){ return sumi::sstmac_mpi()->allgatherv(sendbuf,sendcount,sendtype,recvbuf,recvcounts,displs,recvtype,comm); }
extern "C" int sstmac_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm){ return sumi::sstmac_mpi()->alltoall(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm); }
extern "C" int sstmac_alltoallv(const void *sendbuf, const int *sendcounts,
            const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
            const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
            MPI_Comm comm){ return sumi::sstmac_mpi()->alltoallv(sendbuf,sendcounts,sdispls,sendtype,recvbuf,recvcounts,rdispls,recvtype,comm); }
extern "C" int sstmac_reduce(const void* src, void* dst,
         int count, MPI_Datatype type, MPI_Op op, int root,
         MPI_Comm comm){ return sumi::sstmac_mpi()->reduce(src,dst,count,type,op,root,comm); }
extern "C" int sstmac_allreduce(const void* src, void* dst,
            int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm){ return sumi::sstmac_mpi()->allreduce(src,dst,count,type,op,comm); }
extern "C" int sstmac_reduce_scatter(const void* src, void* dst,
                 const int* recvcnts, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm){ return sumi::sstmac_mpi()->reduce_scatter(src,dst,recvcnts,type,op,comm); }
extern "C" int sstmac_reduce_scatter_block(const void* src, void* dst,
                 int recvcnt, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm){ return sumi::sstmac_mpi()->reduce_scatter_block(src,dst,recvcnt,type,op,comm); }
extern "C" int sstmac_scan(const void* src, void* dst,
      int count, MPI_Datatype type, MPI_Op op,
       MPI_Comm comm){ return sumi::sstmac_mpi()->scan(src,dst,count,type,op,comm); }
extern "C" int sstmac_ibarrier(MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->ibarrier(comm,req); }
extern "C" int sstmac_ibcast(void *buffer, int count, MPI_Datatype datatype, int root,
        MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->ibcast(buffer,count,datatype,root,comm,req); }
extern "C" int sstmac_iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
           void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
           MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->iscatter(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm,req); }
extern "C" int sstmac_iscatterv(const void *sendbuf, const int *sendcounts, const int *displs,
           MPI_Datatype sendtype, void *recvbuf, int recvcount,
           MPI_Datatype recvtype,
           int root, MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->iscatterv(sendbuf,sendcounts,displs,sendtype,recvbuf,recvcount,recvtype,root,comm,req); }
extern "C" int sstmac_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
         void *recvbuf, int recvcount, MPI_Datatype recvtype,
         int root, MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->igather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,root,comm,req); }
extern "C" int sstmac_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
          void *recvbuf, const int *recvcounts, const int *displs,
          MPI_Datatype recvtype, int root,
          MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->igatherv(sendbuf,sendcount,sendtype,recvbuf,recvcounts,displs,recvtype,root,comm,req); }
extern "C" int sstmac_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->iallgather(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm,req); }
extern "C" int sstmac_iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
             void *recvbuf, const int *recvcounts, const int *displs,
             MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->iallgatherv(sendbuf,sendcount,sendtype,recvbuf,recvcounts,displs,recvtype,comm,req); }
extern "C" int sstmac_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->ialltoall(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm,req); }
extern "C" int sstmac_ialltoallv(const void *sendbuf, const int *sendcounts,
            const int *sdispls, MPI_Datatype sendtype, void *recvbuf,
            const int *recvcounts, const int *rdispls, MPI_Datatype recvtype,
            MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->ialltoallv(sendbuf,sendcounts,sdispls,sendtype,recvbuf,recvcounts,rdispls,recvtype,comm,req); }
extern "C" int sstmac_ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request *request){
              return sumi::sstmac_mpi()->ialltoallw(sendbuf,sendcounts,sdispls,sendtypes,
                                                    recvbuf,recvcounts,rdispls,recvtypes,
                                                    comm,request);
            }
extern "C" int sstmac_ireduce(const void* src, void* dst,
         int count, MPI_Datatype type, MPI_Op op, int root,
         MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->ireduce(src,dst,count,type,op,root,comm,req); }
extern "C" int sstmac_iallreduce(const void* src, void* dst,
            int count, MPI_Datatype type, MPI_Op op,
            MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->iallreduce(src,dst,count,type,op,comm,req); }
extern "C" int sstmac_ireduce_scatter(const void* src, void* dst,
                 const int* recvcnts, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->ireduce_scatter(src,dst,recvcnts,type,op,comm,req); }
extern "C" int sstmac_ireduce_scatter_block(const void* src, void* dst,
                 int recvcnt, MPI_Datatype type,
                 MPI_Op op, MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->ireduce_scatter_block(src,dst,recvcnt,type,op,comm,req); }
extern "C" int sstmac_iscan(const void* src, void* dst,
      int count, MPI_Datatype type, MPI_Op op,
       MPI_Comm comm, MPI_Request* req){ return sumi::sstmac_mpi()->iscan(src,dst,count,type,op,comm,req); }
extern "C" int sstmac_type_get_name(MPI_Datatype type, char* type_name, int* resultlen){ return sumi::sstmac_mpi()->type_get_name(type,type_name,resultlen); }
extern "C" int sstmac_type_set_name(MPI_Datatype type, const char* type_name){ return sumi::sstmac_mpi()->type_set_name(type,type_name); }
extern "C" int sstmac_type_extent(MPI_Datatype type, MPI_Aint* extent){ return sumi::sstmac_mpi()->type_extent(type,extent); }
extern "C" int sstmac_op_create(MPI_User_function* user_fn, int commute, MPI_Op* op){ return sumi::sstmac_mpi()->op_create(user_fn,commute,op); }
extern "C" int sstmac_op_free(MPI_Op* op){ return sumi::sstmac_mpi()->op_free(op); }
extern "C" int sstmac_get_count(const MPI_Status* status, MPI_Datatype datatype, int* count){ return sumi::sstmac_mpi()->get_count(status,datatype,count); }
extern "C" int sstmac_type_dup(MPI_Datatype intype, MPI_Datatype* outtype){ return sumi::sstmac_mpi()->type_dup(intype,outtype); }
extern "C" int sstmac_type_indexed(int count, const int _blocklens_[], const int* _indices_,
               MPI_Datatype intype, MPI_Datatype* outtype){ return sumi::sstmac_mpi()->type_indexed(count,_blocklens_,_indices_,intype,outtype); }
extern "C" int sstmac_type_hindexed(int count, const int _blocklens_[], const MPI_Aint* _indices_,
               MPI_Datatype intype, MPI_Datatype* outtype){ return sumi::sstmac_mpi()->type_hindexed(count,_blocklens_,_indices_,intype,outtype); }
extern "C" int sstmac_type_contiguous(int count, MPI_Datatype old_type, MPI_Datatype* new_type){ return sumi::sstmac_mpi()->type_contiguous(count,old_type,new_type); }
extern "C" int sstmac_type_vector(int count, int blocklength, int stride,
              MPI_Datatype old_type,
              MPI_Datatype* new_type){ return sumi::sstmac_mpi()->type_vector(count,blocklength,stride,old_type,new_type); }
extern "C" int sstmac_type_hvector(int count, int blocklength, MPI_Aint stride,
              MPI_Datatype old_type,
              MPI_Datatype* new_type){ return sumi::sstmac_mpi()->type_hvector(count,blocklength,stride,old_type,new_type); }
extern "C" int sstmac_type_create_struct(int count, const int* blocklens,
              const MPI_Aint* displs,
              const MPI_Datatype* old_types,
              MPI_Datatype* newtype){ return sumi::sstmac_mpi()->type_create_struct(count,blocklens,displs,old_types,newtype); }
extern "C" int sstmac_type_commit(MPI_Datatype* type){ return sumi::sstmac_mpi()->type_commit(type); }
extern "C" int sstmac_type_free(MPI_Datatype* type){ return sumi::sstmac_mpi()->type_free(type); }

extern "C" double sstmac_wtime(){ return sumi::sstmac_mpi()->wtime(); }
extern "C" double sstmac_wticks(){ return sumi::sstmac_mpi()->wtime(); }

extern "C" int sstmac_abort(MPI_Comm comm, int errcode){ return sumi::sstmac_mpi()->abort(comm,errcode); }

extern "C" int sstmac_group_translate_ranks(MPI_Group group1, int n, const int ranks1[],
                                            MPI_Group group2, int ranks2[]){
  return sumi::sstmac_mpi()->group_translate_ranks(group1, n, ranks1, group2, ranks2);
}

extern "C" int sstmac_errhandler_create(MPI_Handler_function *function, MPI_Errhandler *errhandler){
  return sumi::sstmac_mpi()->errhandler_create(function,errhandler);
}

extern "C" int sstmac_comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag){
  return sumi::sstmac_mpi()->comm_get_attr(comm,comm_keyval,attribute_val,flag);
}

extern "C" int sstmac_win_flush(int rank, MPI_Win win){
  return sumi::sstmac_mpi()->win_flush(rank, win);
}

extern "C" int sstmac_win_flush_local(int rank, MPI_Win win){
  return sumi::sstmac_mpi()->win_flush_local(rank, win);
}

extern "C" int sstmac_comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm * newcomm){
  return sumi::sstmac_mpi()->comm_create_group(comm,group,tag,newcomm);
}

extern "C" int sstmac_win_create(void *base, MPI_Aint size, int disp_unit,
                                 MPI_Info info, MPI_Comm comm, MPI_Win *win) {
  return sumi::sstmac_mpi()->win_create(base, size, disp_unit, info, comm, win);
}

extern "C" int sstmac_win_free(MPI_Win *win) {
  return sumi::sstmac_mpi()->win_free(win);
}

extern "C" int sstmac_win_fence(int lock_type, int rank, int assert,
                                MPI_Win *win) {
  return sumi::sstmac_mpi()->win_fence(lock_type, rank, assert, win);
}

extern "C" int sstmac_accumulate(void *origin, int origin_count,
                                 MPI_Datatype origin_datatype, int target_rank,
                                 MPI_Aint target_disp, int target_count,
                                 MPI_Datatype target_datatype, MPI_Op op,
                                 MPI_Win win) {
  return sumi::sstmac_mpi()->accumulate(origin, origin_count, origin_datatype,
                                        target_rank, target_disp, target_count,
                                        target_datatype, op, win);
}

extern "C" int sstmac_alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr) {
  return sumi::sstmac_mpi()->alloc_mem(size, info, baseptr);
}

extern "C" int sstmac_win_set_name(MPI_Win win, const char *win_name) {
  return sumi::sstmac_mpi()->win_set_name(win, win_name);
}

extern "C" int sstmac_win_get_name(MPI_Win win, char *win_name,
                                   int *resultlen) {
  return sumi::sstmac_mpi()->win_get_name(win, win_name, resultlen);
}

extern "C" int sstmac_free_mem(void *baseptr) {
  return sumi::sstmac_mpi()->free_mem(baseptr);
}

extern "C" int sstmac_win_lock(int lock_type, int rank, int assert, MPI_Win win){
  return sumi::sstmac_mpi()->win_lock(lock_type, rank, assert, win);
}

extern "C" int sstmac_win_unlock(int rank, MPI_Win win){
  return sumi::sstmac_mpi()->win_unlock(rank, win);
}

extern "C" int sstmac_mpi_get(void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win win){
  return sumi::sstmac_mpi()->get(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype, win);
}

extern "C" int sstmac_mpi_put(const void *origin_addr, int origin_count, MPI_Datatype
            origin_datatype, int target_rank, MPI_Aint target_disp,
            int target_count, MPI_Datatype target_datatype, MPI_Win win){
  return sumi::sstmac_mpi()->put(origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count,
                                 target_datatype, win);
}

extern "C" int sstmac_win_create_keyval(
    MPI_Win_copy_attr_function *win_copy_attr_fn,
    MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval,
    void *extra_state) {
  return sumi::sstmac_mpi()->win_create_keyval(
      win_copy_attr_fn, win_delete_attr_fn, win_keyval, extra_state);
}

extern "C" int sstmac_win_free_keyval(int *win_keyval) {
  return sumi::sstmac_mpi()->win_free_keyval(win_keyval);
}

extern "C" int sstmac_info_set(MPI_Info *info, char *key, char *value) {
  return sumi::sstmac_mpi()->info_set(info, key, value);
}

extern "C" int sstmac_info_create(MPI_Info *info) {
  return sumi::sstmac_mpi()->info_create(info);
}

extern "C" int sstmac_info_free(MPI_Info *info) {
  return sumi::sstmac_mpi()->info_free(info);
}

extern "C" int sstmac_win_delete_attr(MPI_Win win, int win_keyval) {
  return sumi::sstmac_mpi()->win_delete_attr(win, win_keyval);
}

extern "C" int sstmac_win_set_attr(MPI_Win win, int win_keyval,
                                   void *attribute_val) {
  return sumi::sstmac_mpi()->win_set_attr(win, win_keyval, attribute_val);
}

extern "C" int sstmac_win_get_attr(MPI_Win win, int win_keyval,
                                   void *attribute_val, int *flag) {
  return sumi::sstmac_mpi()->win_get_attr(win, win_keyval, attribute_val, flag);
}

extern "C" int sstmac_group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup){
  return sumi::sstmac_mpi()->group_range_incl(group, n, ranges, newgroup);
}

extern "C" int sstmac_pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
  return sumi::sstmac_mpi()->pack_size(incount, datatype, comm, size);
}
