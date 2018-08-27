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

#include <sstmac/software/process/app.h>
#include <sstmac/software/process/operating_system.h>
#include <sprockit/util.h>
#include <sstmac/libraries/sumi/sumi_transport.h>

using namespace sstmac;
using namespace sstmac::sw;


//this redirection macro foobars things here
#ifdef sleep
#if sleep == sstmac_sleep
#undef sleep
#endif
#endif

namespace sumi {

static ::sstmac::sumi::transport* current_transport()
{
  thread* t = thread::current();
  return t->get_api<::sstmac::sumi::transport>();
}

::sstmac::sumi::transport* sumi_api()
{
  return current_transport();
}

void comm_init()
{
  current_transport()->init();
}

void comm_kill_process()
{
  sprockit::abort("unimplemented: comm kill process");
}

void comm_kill_node()
{
  sstmac::sw::operating_system::current_os()->kill_node();
  throw ::sstmac::sumi::terminate_exception();
}

void comm_finalize()
{
  current_transport()->finish();
}

void comm_vote(int vote, int tag, ::sumi::vote_fxn fxn, ::sumi::collective::config cfg)
{
  current_transport()->dynamic_tree_vote(vote, tag, fxn, cfg);
}

void
comm_allreduce(void *dst, void *src, int nelems, int type_size, int tag, ::sumi::reduce_fxn fxn, ::sumi::collective::config cfg)
{
  current_transport()->allreduce(dst, src, nelems, type_size, tag, fxn, cfg);
}

void comm_scan(void *dst, void *src, int nelems, int type_size, int tag, ::sumi::reduce_fxn fxn, ::sumi::collective::config cfg)
{
  current_transport()->scan(dst, src, nelems, type_size, tag, fxn, cfg);
}

void
comm_reduce(int root, void *dst, void *src, int nelems, int type_size, int tag, ::sumi::reduce_fxn fxn, ::sumi::collective::config cfg)
{
  current_transport()->reduce(root, dst, src, nelems, type_size, tag, fxn, cfg);
}

void comm_alltoall(void *dst, void *src, int nelems, int type_size, int tag, ::sumi::collective::config cfg)
{
  current_transport()->alltoall(dst, src, nelems, type_size, tag, cfg);
}

void comm_allgather(void *dst, void *src, int nelems, int type_size, int tag, ::sumi::collective::config cfg)
{
  current_transport()->allgather(dst, src, nelems, type_size, tag, cfg);
}

void comm_allgatherv(void *dst, void *src, int* recv_counts, int type_size, int tag, ::sumi::collective::config cfg)
{
  current_transport()->allgatherv(dst, src, recv_counts, type_size, tag, cfg);
}

void comm_gather(int root, void *dst, void *src, int nelems, int type_size, int tag, ::sumi::collective::config cfg)
{
  current_transport()->gather(root, dst, src, nelems, type_size, tag, cfg);
}

void comm_scatter(int root, void *dst, void *src, int nelems, int type_size, int tag, ::sumi::collective::config cfg)
{
  current_transport()->scatter(root, dst, src, nelems, type_size, tag, cfg);
}

void comm_bcast(int root, void *buffer, int nelems, int type_size, int tag, ::sumi::collective::config cfg)
{
  current_transport()->bcast(root, buffer, nelems, type_size, tag, cfg);
}

void comm_barrier(int tag, ::sumi::collective::config cfg)
{
  current_transport()->barrier(tag, cfg);
}

::sumi::deprecated::collective_done_message* comm_collective_block(::sumi::collective::type_t ty, int tag)
{
  return current_transport()->collective_block(ty, tag);
}

int comm_rank()
{
  return current_transport()->rank();
}

int comm_nproc()
{
  return current_transport()->nproc();
}

/**
    @param dst The destination to send to
*/
void comm_send(int dst, ::sumi::deprecated::message::payload_type_t ty, ::sumi::deprecated::message* msg)
{
  msg->set_class_type(::sumi::deprecated::message::pt2pt);
  current_transport()->smsg_send(dst, ty, msg, ::sumi::deprecated::message::no_ack, ::sumi::deprecated::message::default_cq);
}

void comm_send_header(int dst, ::sumi::deprecated::message* msg)
{
  msg->set_class_type(::sumi::deprecated::message::pt2pt);
  current_transport()->send_header(dst, msg, ::sumi::deprecated::message::no_ack, ::sumi::deprecated::message::default_cq);
}

void comm_send_payload(int dst, ::sumi::deprecated::message* msg)
{
  msg->set_class_type(::sumi::deprecated::message::pt2pt);
  current_transport()->send_payload(dst, msg, ::sumi::deprecated::message::no_ack, ::sumi::deprecated::message::default_cq);
}

void comm_rdma_put(int dst, ::sumi::deprecated::message* msg)
{
  msg->set_class_type(::sumi::deprecated::message::pt2pt);
  current_transport()->rdma_put(dst, msg, ::sumi::deprecated::message::no_ack, ::sumi::deprecated::message::default_cq);
}

void comm_nvram_get(int dst, ::sumi::deprecated::message* msg)
{
  msg->set_class_type(::sumi::deprecated::message::pt2pt);
  current_transport()->nvram_get(dst, msg);
}

void comm_rdma_get(int dst, ::sumi::deprecated::message* msg)
{
  msg->set_class_type(::sumi::deprecated::message::pt2pt);
  current_transport()->rdma_get(dst, msg, ::sumi::deprecated::message::no_ack, ::sumi::deprecated::message::default_cq);
}

::sumi::deprecated::message* comm_poll()
{
  return current_transport()->blocking_poll(::sumi::deprecated::message::default_cq);
}

double wall_time()
{
  return operating_system::current_os()->now().sec();
}

void sleep_until(double sec)
{
  thread* thr = thread::current();
  app* my_app = thr->parent_app();
  double time = sec - my_app->now().sec();
  my_app->sleep(timestamp(time));
}

void sleep(double sec)
{
  thread* thr = thread::current();
  app* my_app = thr->parent_app();
  my_app->sleep(timestamp(sec));
}

void compute(double sec)
{
  thread* thr = thread::current();
  app* my_app = thr->parent_app();
  my_app->compute(timestamp(sec));
}

#ifdef FEATURE_TAG_SUMI_RESILIENCE
void
comm_start_heartbeat(double interval)
{
  current_transport()->start_heartbeat(interval);
}

void
comm_stop_heartbeat()
{
  current_transport()->stop_heartbeat();
}

const thread_safe_set<int>&
comm_failed_ranks()
{
  return current_transport()->failed_ranks();
}

const thread_safe_set<int>&
comm_failed_ranks(int context)
{
  return current_transport()->failed_ranks(context);
}

void
comm_cancel_ping(int dst, timeout_function* func)
{
  current_transport()->cancel_ping(dst, func);
}

void
comm_ping(int dst, timeout_function* func)
{
  current_transport()->ping(dst, func);
}
#endif


} // namespace sumi 
