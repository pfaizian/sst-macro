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

#ifndef sumi_msg_api_h
#define sumi_msg_api_h

#include <sumi/options.h>
#include <sumi/message.h>
#include <sumi/comm_functions.h>
#include <sumi/collective_message.h>
#include <sumi/timeout.h>
#include <sumi/communicator.h>
#include <sstmac/libraries/sumi/sumi_transport.h>

namespace sumi {

void comm_init();

void comm_finalize();

int comm_rank();

int comm_nproc();

/**
    @param dst The destination to send to
*/
void comm_send_header(int dst, deprecated::message* msg);

void comm_cancel_ping(int dst, int tag);

void comm_ping(int dst, int tag, timeout_function* func);

void comm_send_payload(int dst, deprecated::message* msg);

void comm_send(int dst, deprecated::message::payload_type_t ev, deprecated::message* msg);

void comm_rdma_put(int dst, deprecated::message* msg);

void comm_rdma_get(int dst, deprecated::message* msg);

void comm_nvram_get(int dst, deprecated::message* msg);

void comm_alltoall(void* dst, void* src, int nelems, int type_size, int tag,
                   collective::config cfg = collective::cfg());

void comm_allgather(void* dst, void* src, int nelems, int type_size, int tag,
                    collective::config cfg = collective::cfg());

void comm_allgatherv(void* dst, void* src, int* recv_counts, int type_size, int tag,
                     collective::config cfg = collective::cfg());

void comm_gather(int root, void* dst, void* src, int nelems, int type_size, int tag,
                 collective::config cfg = collective::cfg());

void comm_scatter(int root, void* dst, void* src, int nelems, int type_size, int tag,
                  collective::config cfg = collective::cfg());

void comm_bcast(int root, void* buffer, int nelems, int type_size, int tag,
                collective::config cfg = collective::cfg());

/**
* The total size of the input/result buffer in bytes is nelems*type_size
* @param dst  Buffer for the result. Can be NULL to ignore payloads.
* @param src  Buffer for the input. Can be NULL to ignore payloads.
* @param nelems The number of elements in the input and result buffer.
* @param type_size The size of the input type, i.e. sizeof(int), sizeof(double)
* @param tag A unique tag identifier for the collective
* @param fxn The function that will actually perform the reduction
* @param fault_aware Whether to execute in a fault-aware fashion to detect failures
* @param context The context (i.e. initial set of failed procs)
*/
void comm_allreduce(void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
                    collective::config cfg = collective::cfg());

template <typename data_t, template <typename> class Op>
void comm_allreduce(void* dst, void* src, int nelems, int tag, collective::config cfg = collective::cfg()){
  typedef ReduceOp<Op, data_t> op_class_type;
  comm_allreduce(dst, src, nelems, sizeof(data_t), tag, &op_class_type::op, cfg);
}

void comm_scan(void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
               collective::config cfg = collective::cfg());

template <typename data_t, template <typename> class Op>
void comm_scan(void* dst, void* src, int nelems, int tag,
               collective::config cfg = collective::cfg()){
  typedef ReduceOp<Op, data_t> op_class_type;
  comm_scan(dst, src, nelems, sizeof(data_t), tag, &op_class_type::op, cfg);
}

void comm_reduce(int root, void* dst, void* src, int nelems, int type_size, int tag, reduce_fxn fxn,
                 collective::config cfg = collective::cfg());

template <typename data_t, template <typename> class Op>
void comm_reduce(int root, void* dst, void* src, int nelems, int tag,
                 collective::config cfg = collective::cfg()){
  typedef ReduceOp<Op, data_t> op_class_type;
  comm_reduce(root, dst, src, nelems, sizeof(data_t), tag, &op_class_type::op, cfg);
}

void comm_barrier(int tag, collective::config cfg = collective::cfg());

/**
* The total size of the input/result buffer in bytes is nelems*type_size
* This always run in a fault-tolerant fashion
* This uses a dynamic tree structure that reconnects partners when failures are detected
* @param vote The vote (currently restricted to integer) from this process
* @param nelems The number of elements in the input and result buffer.
* @param tag A unique tag identifier for the collective
* @param fxn The function that merges vote, usually AND, OR, MAX, MIN
* @param context The context (i.e. initial set of failed procs)
*/
void comm_vote(int vote, int tag, vote_fxn fxn, collective::config cfg = collective::cfg());

template <template <class> class VoteOp>
void comm_vote(int vote, int tag, collective::config cfg = collective::cfg()){
  typedef VoteOp<int> op_class_type;
  comm_vote(vote, tag, &op_class_type::op, cfg);
}

deprecated::collective_done_message* comm_collective_block(collective::type_t ty, int tag);

deprecated::message* comm_poll();

void compute(double sec);

void sleep_hires(double sec);

void sleep_until(double sec);

/**
 * Every node has exactly the same notion of time - universal, global clock.
 * Thus, if rank 0 starts and 10 minuts later rank 1 starts,
 * even though rank 1 has only been running for 30 seconds, the time will still return
 * 10 mins, 30 seconds.
 * @return The current system wall-clock time in seconds.
 *         At application launch, time is zero.
 */
double wall_time();

sstmac::sumi::transport* sumi_api();


}


#endif // SIMPMSG_H
