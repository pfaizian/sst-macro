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

#include <sstmac/common/sstmac_config.h>
#if SSTMAC_INTEGRATED_SST_CORE
#include <sst/core/event.h>
#endif
#include <sprockit/output.h>
#include <sstmac/common/event_callback.h>
#include <sstmac/common/runtime.h>
#include <sstmac/common/stats/stat_spyplot.h>
#include <sstmac/hardware/node/node.h>
#include <sstmac/libraries/sumi/message.h>
#include <sstmac/libraries/sumi/sumi_transport.h>
#include <sstmac/software/launch/job_launcher.h>
#include <sstmac/software/process/app.h>
#include <sstmac/software/process/key.h>
#include <sstmac/software/process/operating_system.h>
#include <sumi/allgather.h>
#include <sumi/allgatherv.h>
#include <sumi/allreduce.h>
#include <sumi/alltoall.h>
#include <sumi/alltoallv.h>
#include <sumi/bcast.h>
#include <sumi/communicator.h>
#include <sumi/dynamic_tree_vote.h>
#include <sumi/gather.h>
#include <sumi/gatherv.h>
#include <sumi/message.h>
#include <sumi/reduce.h>
#include <sumi/reduce_scatter.h>
#include <sumi/scan.h>
#include <sumi/scatter.h>
#include <sumi/scatterv.h>

using namespace sprockit::dbg;

RegisterDebugSlot(sumi);

namespace sumi {
// TODO THIS should be set somewhere else, but I am not sure yet as to where.
const int options::initial_context = -2;
}  // namespace sumi

RegisterKeywords(
    {"post_rdma_delay", "the time it takes to post an RDMA operation"},
    {"post_header_delay", "the time it takes to send an eager message"},
    {"poll_delay", "the time it takes to poll for an incoming message"},
    {"rdma_pin_latency", "the latency for each RDMA pin information"},
    {"rdma_page_delay", "the per-page delay for RDMA pinning"},
    {"lazy_watch",
     "whether failure notifications can be receive without active pinging"},
    {"eager_cutoff",
     "what message size in bytes to switch from eager to rendezvous"},
    {"use_put_protocol",
     "whether to use a put or get protocol for pt2pt sends"},
    {"algorithm", "the specific algorithm to use for a given collecitve"},
    {"comm_sync_stats",
     "whether to track synchronization stats for communication"},
    {"smp_single_copy_size",
     "the minimum size of message for single-copy protocol"},
    {"max_eager_msg_size", "the maximum size for using eager pt2pt protocol"},
    {"max_vshort_msg_size", "the maximum size for mailbox protocol"});

namespace sstmac {
namespace sumi {

::sumi::collective_algorithm_selector* transport::allgather_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::alltoall_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::alltoallv_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::allreduce_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::allgatherv_selector_ =
    nullptr;
::sumi::collective_algorithm_selector* transport::bcast_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::gather_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::gatherv_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::reduce_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::reduce_scatter_selector_ =
    nullptr;
::sumi::collective_algorithm_selector* transport::scan_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::scatter_selector_ = nullptr;
::sumi::collective_algorithm_selector* transport::scatterv_selector_ = nullptr;

class sumi_server : public sstmac::sw::service {
 public:
  sumi_server(transport* tport)
      : service(tport->server_libname(),
                sstmac::sw::software_id(-1, -1),  // belongs to no application
                tport->os()) {}

  void register_proc(int rank, transport* proc) {
    int app_id = proc->sid().app_;
    debug_printf(sprockit::dbg::sumi,
                 "sumi_server registering rank %d for app %d", rank, app_id);
    transport*& slot = procs_[app_id][rank];
    if (slot) {
      spkt_abort_printf(
          "sumi_server: already registered rank %d for app %d on node %d", rank,
          app_id, os_->addr());
    }
    slot = proc;

    auto iter = pending_.begin();
    auto end = pending_.end();
    while (iter != end) {
      auto tmp = iter++;
      transport_message* msg = *tmp;
      if (msg->dest_rank() == rank && msg->dest_app() == proc->sid().app_) {
        pending_.erase(tmp);
        proc->incoming_message(msg);
      }
    }
  }

  bool unregister_proc(int rank, transport* proc) {
    int app_id = proc->sid().app_;
    auto iter = procs_.find(app_id);
    auto& subMap = iter->second;
    subMap.erase(rank);
    if (subMap.empty()) {
      procs_.erase(iter);
    }
    return procs_.empty();
  }

  void incoming_event(event* ev) {
    transport_message* smsg = safe_cast(transport_message, ev);
    debug_printf(sprockit::dbg::sumi, "sumi_server %d: incoming message %s",
                 os_->addr(), smsg->get_payload()->to_string().c_str());
    transport* tport = procs_[smsg->dest_app()][smsg->dest_rank()];
    if (!tport) {
      pending_.push_back(smsg);
    } else {
      tport->incoming_message(smsg);
    }
  }

 private:
  std::map<int, std::map<int, transport*>> procs_;
  std::list<transport_message*> pending_;
};

// These were in transport.cc, but my guess is they should be here
template <class Map, class Val, class Key>
bool pull_from_map(Val& val, const Map& m, const Key& k) {
  typedef typename Map::iterator iterator;
  iterator it = m.find(k);
  if (m.find(k) == m.end()) {
    return false;
  } else {
    val = it->second;
    m.erase(it);
    return true;
  }
}

template <class Map, class Val, class Key1, class Key2>
bool pull_from_map(Val& val, const Map& m, const Key1& k1, const Key2& k2) {
  typedef typename Map::iterator iterator;
  iterator it = m.find(k1);
  if (it == m.end()) return false;

  bool ret = check_map(val, it->second, k2);
  if (it->second.empty()) m.erase(it);
  return ret;
}

template <class Map, class Val, class Key1, class Key2, class Key3>
bool pull_from_map(Val& val, const Map& m, const Key1& k1, const Key2& k2,
                   const Key3& k3) {
  typedef typename Map::iterator iterator;
  iterator it = m.find(k1);
  if (it == m.end()) return false;

  bool ret = check_map(val, it->second, k2, k3);
  if (it->second.empty()) m.erase(it);
  return ret;
}
// END THESE WERE IN

transport::transport(sprockit::sim_parameters* params, const char* prefix,
                     sstmac::sw::software_id sid,
                     sstmac::sw::operating_system* os)
    : transport(params, standard_lib_name(prefix, sid), sid, os) {}

transport::transport(sprockit::sim_parameters* params,
                     sstmac::sw::software_id sid,
                     sstmac::sw::operating_system* os)
    : transport(params, "sumi", sid, os) {}

transport::transport(sprockit::sim_parameters* params,
                     const std::string& libname, sstmac::sw::software_id sid,
                     sstmac::sw::operating_system* os)
    : completion_queues_(1),  // guaranteed one to begin with
      inited_(false),
      system_collective_tag_(-1),  // negative tags reserved
      finalized_(false),
      eager_cutoff_(512),
      use_put_protocol_(false),
      use_hardware_ack_(false),
      global_domain_(nullptr),
      // the name of the transport itself should be mapped to a unique name
      api(params, libname, sid, os),
      // the server is what takes on the specified libname
      server_libname_("sumi_server"),
      user_lib_time_(nullptr),
      spy_num_messages_(nullptr),
      spy_bytes_(nullptr),
      collective_cq_id_(1),  // this gets assigned elsewhere
      pt2pt_cq_id_(0)        // put pt2pt sends on the default cq
{
  eager_cutoff_ = params->get_optional_int_param("eager_cutoff", 512);
  use_put_protocol_ =
      params->get_optional_bool_param("use_put_protocol", false);

  collective_cq_id_ = allocate_cq();
  rank_ = sid.task_;
  library* server_lib = os_->lib(server_libname_);
  sumi_server* server;
  // only do one server per app per node
  if (server_lib == nullptr) {
    server = new sumi_server(this);
    server->start();
  } else {
    server = safe_cast(sumi_server, server_lib);
  }

  post_rdma_delay_ = params->get_optional_time_param("post_rdma_delay", 0);
  post_header_delay_ = params->get_optional_time_param("post_header_delay", 0);
  poll_delay_ = params->get_optional_time_param("poll_delay", 0);
  user_lib_time_ =
      new sstmac::sw::lib_compute_time(params, "sumi-user-lib-time", sid, os);

  rdma_pin_latency_ = params->get_optional_time_param("rdma_pin_latency", 0);
  rdma_page_delay_ = params->get_optional_time_param("rdma_page_delay", 0);
  pin_delay_ = rdma_pin_latency_.ticks() || rdma_page_delay_.ticks();
  page_size_ = params->get_optional_byte_length_param("rdma_page_size", 4096);

  rank_mapper_ = sstmac::sw::task_mapping::global_mapping(sid.app_);
  nproc_ = rank_mapper_->nproc();
  component_id_ = os_->component_id();

  server->register_proc(rank_, this);

  spy_num_messages_ = sstmac::optional_stats<sstmac::stat_spyplot>(
      des_scheduler(), params, "traffic_matrix", "ascii", "num_messages");
  spy_bytes_ = sstmac::optional_stats<sstmac::stat_spyplot>(
      des_scheduler(), params, "traffic_matrix", "ascii", "bytes");
}

void transport::pin_rdma(uint64_t bytes) {
  int num_pages = bytes / page_size_;
  if (bytes % page_size_) ++num_pages;
  sstmac::timestamp pin_delay =
      rdma_pin_latency_ + num_pages * rdma_page_delay_;
  compute(pin_delay);
}

void transport::ctor_common(sstmac::sw::software_id sid) {
  rank_ = sid.task_;
  sw::thread* thr = sw::operating_system::current_thread();
  sw::app* my_app = safe_cast(sw::app, thr);
  my_app->compute(timestamp(1e-6));
}

transport::~transport() {
  if (global_domain_) {
    delete global_domain_;
  }
  delete user_lib_time_;
  sumi_server* server = safe_cast(sumi_server, os_->lib(server_libname_));
  bool del = server->unregister_proc(rank_, this);
  if (del) {
    delete server;
  }
}

// transport::transport(sprockit::sim_parameters* params)
//     :
//       completion_queues_(1),  // guaranteed one to begin with
//       inited_(false),
//       system_collective_tag_(
//           -1),  // negative tags reserved for special system work
//       finalized_(false),
//       eager_cutoff_(512),
//       use_put_protocol_(false),
//       use_hardware_ack_(false),
//       global_domain_(nullptr) {
//   eager_cutoff_ = params->get_optional_int_param("eager_cutoff", 512);
//   use_put_protocol_ =
//       params->get_optional_bool_param("use_put_protocol", false);
// }

event_scheduler* transport::des_scheduler() const { return os_->node(); }

void transport::memcopy(uint64_t bytes) {
  os_->current_thread()->parent_app()->compute_block_memcpy(bytes);
}

void transport::incoming_event(event* ev) {
  spkt_abort_printf(
      "transport::incoming_event: should not directly handle events");
}

void transport::shutdown_server(int dest_rank, node_id dest_node,
                                int dest_app) {
  auto msg = new ::sumi::deprecated::system_bcast_message(
      ::sumi::deprecated::system_bcast_message::shutdown, dest_rank);
  client_server_send(dest_rank, dest_node, dest_app, msg);
}

void transport::client_server_send(int dst_task, node_id dst_node, int dst_app,
                                   ::sumi::deprecated::message* msg) {
  msg->set_send_cq(::sumi::deprecated::message::no_ack);
  msg->set_recv_cq(::sumi::deprecated::message::default_cq);
  send(msg->byte_length(), dst_task, dst_node, dst_app, msg,
       hw::network_message::payload);
}

void transport::client_server_rdma_put(int dst_task, node_id dst_node,
                                       int dst_app,
                                       ::sumi::deprecated::message* msg) {
  msg->set_send_cq(::sumi::deprecated::message::no_ack);
  msg->set_recv_cq(::sumi::deprecated::message::default_cq);
  send(msg->byte_length(), dst_task, dst_node, dst_app, msg,
       hw::network_message::rdma_put_payload);
}

void transport::rdma_get(int src, ::sumi::deprecated::message* msg, int send_cq,
                         int recv_cq) {
  if (send_cq == -1 && recv_cq == -1) {
    spkt_abort_printf(
        "no completion queues specified for ::sumi::deprecated::message %s",
        msg->to_string().c_str());
  }

  configure_send(src, ::sumi::deprecated::message::rdma_get, msg);
  msg->set_send_cq(send_cq);
  msg->set_recv_cq(recv_cq);
  do_rdma_get(src, msg);
}

void transport::nvram_get(int src, ::sumi::deprecated::message* msg) {
  configure_send(src, ::sumi::deprecated::message::nvram_get, msg);
  do_nvram_get(src, msg);
}

::sumi::deprecated::message* transport::poll(bool blocking, int cq_id,
                                             double timeout) {
  auto& queue = completion_queues_[cq_id];
  debug_printf(
      sprockit::dbg::sumi,
      "Rank %d polling for ::sumi::deprecated::message on cq %d with time %f",
      rank_, cq_id, timeout);

  if (!queue.empty()) {
    ::sumi::deprecated::message* ret = queue.front();
    queue.pop_front();
    return ret;
  }

  return poll_new(blocking, cq_id, timeout);
}

::sumi::deprecated::message* transport::poll(bool blocking, double timeout) {
  for (auto& queue : completion_queues_) {
    if (!queue.empty()) {
      ::sumi::deprecated::message* ret = queue.front();
      queue.pop_front();
      return ret;
    }
  }
  return poll_new(blocking, timeout);
}

::sumi::deprecated::message* transport::blocking_poll(int cq_id,
                                                      double timeout) {
  return poll(true, cq_id, timeout);  // blocking
}

::sumi::deprecated::message* transport::poll(
    ::sumi::deprecated::message::payload_type_t ty, bool blocking, int cq_id,
    double timeout) {
  auto& queue = completion_queues_[cq_id];
  auto end = queue.end();
  for (auto iter = cq_begin(cq_id); iter != end; ++iter) {
    ::sumi::deprecated::message* msg = *iter;
    if (msg->payload_type() == ty) {
      queue.erase(iter);
      return msg;
    }
  }

  return poll_new(ty, blocking, cq_id, timeout);
}

::sumi::deprecated::message* transport::poll_new(bool blocking, int cq_id,
                                                 double timeout) {
  while (1) {
    ::sumi::deprecated::message* msg = poll_new(blocking, timeout);
    if (msg) {
      if (msg->cq_id() == cq_id) {
        debug_printf(sprockit::dbg::sumi, "Rank %d got hit %p on CQ %d", rank_,
                     msg, cq_id);
        return msg;
      } else {
        completion_queues_[msg->cq_id()].push_back(msg);
      }
    }
    // if I got here and am non-blocking, return null
    if (!blocking || timeout > 0) return nullptr;
  }
}

::sumi::deprecated::message* transport::poll_new(
    ::sumi::deprecated::message::payload_type_t ty, bool blocking, int cq_id,
    double timeout) {
  while (1) {
    ::sumi::deprecated::message* msg = poll_new(blocking, timeout);
    if (msg) {
      if (msg->cq_id() == cq_id && msg->payload_type() == ty) {
        return msg;
      } else {
        completion_queues_[msg->cq_id()].push_back(msg);
      }
    }
    // if I got here and am non-blocking, return null
    if (!blocking) return nullptr;
  }
}

void transport::set_use_hardware_ack(bool flag) {
  if (flag && !supports_hardware_ack()) {
    sprockit::abort(
        "transport::chosen transport does not support hardware acks");
  }
  use_hardware_ack_ = flag;
}

void transport::send_self_terminate() {
  ::sumi::deprecated::message* msg = new ::sumi::deprecated::message();
  msg->set_class_type(::sumi::deprecated::message::terminate);
  send_header(rank_, msg, ::sumi::deprecated::message::no_ack,
              ::sumi::deprecated::message::default_cq);  // send to self
}

void transport::system_bcast(::sumi::deprecated::message* msg) {
  auto bmsg = dynamic_cast<::sumi::deprecated::system_bcast_message*>(msg);
  int root = bmsg->root();
  int my_effective_rank = (rank_ - root + nproc_) % nproc_;

  /**
   0->1
   0->2
   1->3
   0->4
   1->5
   2->6
   3->7
   ... and so on
  */

  int partner_gap = 1;
  int rank_check = rank_;
  while (rank_check > 0) {
    partner_gap *= 2;
    rank_check /= 2;
  }

  int effective_target = my_effective_rank + partner_gap;
  while (effective_target < nproc_) {
    int target = (effective_target + root) % nproc_;
    ::sumi::deprecated::message* next_msg =
        new ::sumi::deprecated::system_bcast_message(bmsg->action(),
                                                     bmsg->root());
    send_header(target, next_msg, ::sumi::deprecated::message::no_ack,
                ::sumi::deprecated::message::default_cq);
    partner_gap *= 2;
    effective_target = my_effective_rank + partner_gap;
  }
}

void transport::notify_collective_done(
    ::sumi::deprecated::collective_done_message* msg) {
  ::sumi::collective* coll = collectives_[msg->type()][msg->tag()];
  if (!coll) {
    spkt_throw_printf(sprockit::value_error,
                      "transport::notify_collective_done: invalid collective "
                      "of type %s, tag %d",
                      ::sumi::collective::tostr(msg->type()), msg->tag());
  }
  finish_collective(coll, msg);
}

void transport::process(sstmac::transport_message* smsg) {
  ::sumi::deprecated::message* my_msg = smsg->get_payload();
  debug_printf(
      sprockit::dbg::sumi,
      "sumi transport rank %d in app %d processing message of type %s: %s",
      rank(), sid().app_, transport_message::tostr(smsg->type()),
      my_msg->to_string().c_str());
  switch (smsg->type()) {
    // depending on the type, we might have to mutate the incoming message
    case sstmac::hw::network_message::failure_notification:
      my_msg->set_payload_type(::sumi::deprecated::message::failure);
      break;
    case sstmac::hw::network_message::rdma_get_nack:
      my_msg->set_payload_type(::sumi::deprecated::message::rdma_get_nack);
      break;
    case sstmac::hw::network_message::payload_sent_ack:
      my_msg->set_payload_type(::sumi::deprecated::message::eager_payload_ack);
      break;
    case sstmac::hw::network_message::rdma_get_sent_ack:
      my_msg->set_payload_type(::sumi::deprecated::message::rdma_get_ack);
      break;
    case sstmac::hw::network_message::rdma_put_sent_ack:
      my_msg->set_payload_type(::sumi::deprecated::message::rdma_put_ack);
      break;
    default:
      break;  // do nothing
  }
}

void transport::init() {
  //::sumi::deprecated::transport::init();
  inited_ = true;
  global_domain_ = new ::sumi::global_communicator(this);
}

void transport::finish() {
  debug_printf(sprockit::dbg::sumi, "Rank %d finalizing", rank_);
  clean_up();
  // TODO (Old todo) this should really loop through and kill off all the pings
  // so none of them execute
  finalized_ = true;
}

void transport::deadlock_check() {
  collective_map::iterator it, end = collectives_.end();
  for (it = collectives_.begin(); it != end; ++it) {
    tag_to_collective_map& next = it->second;
    tag_to_collective_map::iterator cit, cend = next.end();
    for (cit = next.begin(); cit != cend; ++cit) {
      ::sumi::collective* coll = cit->second;
      if (!coll->complete()) {
        coll->deadlock_check();
      }
    }
  }
}

int transport::allocate_cq() {
  uint8_t ret = completion_queues_.size();
  completion_queues_.emplace_back();
  return ret;
}

void transport::smsg_send(int dst,
                          ::sumi::deprecated::message::payload_type_t ev,
                          ::sumi::deprecated::message* msg, int send_cq,
                          int recv_cq) {
  configure_send(dst, ev, msg);
  msg->set_send_cq(send_cq);
  msg->set_recv_cq(recv_cq);

  if (send_cq == -1 && recv_cq == -1) abort();

  debug_printf(
      sprockit::dbg::sumi,
      "Rank %d SUMI sending short message to %d, send ack %srequested ", rank_,
      dst, (msg->needs_send_ack() ? "" : "NOT "));

  if (dst == rank_) {
    // deliver to self
    debug_printf(sprockit::dbg::sumi, "Rank %d SUMI sending self message",
                 rank_);

    if (msg->needs_recv_ack()) {
      completion_queues_[msg->recv_cq()].push_back(msg);
    }
    if (msg->needs_send_ack()) {
      ::sumi::deprecated::message* ack =
          msg->clone(::sumi::deprecated::message::eager_payload_ack);
      ack->set_payload_type(::sumi::deprecated::message::eager_payload_ack);
      completion_queues_[msg->send_cq()].push_back(ack);
    }
  } else {
    do_smsg_send(dst, msg);
  }
#if SSTMAC_COMM_SYNC_STATS
  msg->set_time_sent(wall_time());
#endif
}

void transport::send_header(int dst, ::sumi::deprecated::message* msg,
                            int send_cq, int recv_cq) {
  smsg_send(dst, ::sumi::deprecated::message::header, msg, send_cq, recv_cq);
}

void transport::send_payload(int dst, ::sumi::deprecated::message* msg,
                             int send_cq, int recv_cq) {
  smsg_send(dst, ::sumi::deprecated::message::eager_payload, msg, send_cq,
            recv_cq);
}

void transport::rdma_put(int dst, ::sumi::deprecated::message* msg, int send_cq,
                         int recv_cq) {
  configure_send(dst, ::sumi::deprecated::message::rdma_put, msg);
  msg->set_send_cq(send_cq);
  msg->set_recv_cq(recv_cq);
  do_rdma_put(dst, msg);
}

int* transport::nidlist() const {
  // just cast an int* - it's fine
  // the types are the same size and the bits can be
  // interpreted correctly
  return (int*)rank_mapper_->rank_to_node().data();
}

void transport::send(uint64_t byte_length, ::sumi::deprecated::message* msg,
                     int sendType, int dst_rank) {
  node_id dst_node = rank_mapper_->rank_to_node(dst_rank);
  send(byte_length, dst_rank, dst_node, sid().app_, msg, sendType);
}

void transport::send(uint64_t byte_length, int dst_task, node_id dst_node,
                     int dst_app, ::sumi::deprecated::message* msg, int ty) {
  sstmac::sw::app_id aid = sid().app_;
  transport_message* tmsg =
      new transport_message(server_libname_, aid, msg, byte_length);
  tmsg->hw::network_message::set_type((hw::network_message::type_t)ty);
  tmsg->toaddr_ = dst_node;
  tmsg->set_src_rank(rank_);
  tmsg->set_dest_rank(dst_task);
  // send intra-app
  tmsg->set_apps(sid().app_, dst_app);
  tmsg->set_needs_ack(msg->needs_send_ack());

  if (spy_num_messages_) spy_num_messages_->add_one(rank_, dst_task);
  if (spy_bytes_) spy_bytes_->add(rank_, dst_task, byte_length);

  sw::library::os_->execute(ami::COMM_SEND, tmsg);
}

void transport::compute(timestamp t) { user_lib_time_->compute(t); }

void transport::go_die() { os_->kill_node(); }

void transport::go_revive() {
  sprockit::abort("SST cannot revive a dead process currently");
}

void transport::do_smsg_send(int dst, ::sumi::deprecated::message* msg) {
  if (post_header_delay_.ticks_int64()) {
    user_lib_time_->compute(post_header_delay_);
  }
  send(msg->byte_length(), msg, sstmac::hw::network_message::payload, dst);
}

double transport::wall_time() const { return now().sec(); }

void transport::do_rdma_get(int dst, ::sumi::deprecated::message* msg) {
  if (post_rdma_delay_.ticks_int64()) {
    user_lib_time_->compute(post_rdma_delay_);
  }

  send(msg->byte_length(), msg, sstmac::hw::network_message::rdma_get_request,
       dst);
}

void transport::do_rdma_put(int dst, ::sumi::deprecated::message* msg) {
  if (post_rdma_delay_.ticks_int64()) {
    user_lib_time_->compute(post_rdma_delay_);
  }
  send(msg->byte_length(), msg, sstmac::hw::network_message::rdma_put_payload,
       dst);
}

void transport::do_nvram_get(int dst, ::sumi::deprecated::message* msg) {
  send(msg->byte_length(), msg, sstmac::hw::network_message::nvram_get_request,
       dst);
}

::sumi::deprecated::transport_message* transport::poll_pending_messages(
    bool blocking, double timeout) {
  if (poll_delay_.ticks_int64()) {
    user_lib_time_->compute(poll_delay_);
  }

  sstmac::sw::thread* thr = os_->active_thread();
  if (pending_messages_.empty() && blocking) {
    blocked_threads_.push_back(thr);
    debug_printf(
        sprockit::dbg::sumi,
        "Rank %d sumi queue %p has no pending messages, blocking poller %p: %s",
        rank(), this, thr, timeout >= 0. ? "with timeout" : "no timeout");
    if (timeout >= 0.) {
      os_->block_timeout(timeout);
    } else {
      os_->block();
    }
    if (timeout <= 0) {
      if (pending_messages_.empty()) {
        spkt_abort_printf(
            "SUMI transport rank %d unblocked with no messages and no timeout",
            rank_, timeout);
      }
    } else {
      if (pending_messages_.empty()) {
        // timed out, erase blocker from list
        auto iter = blocked_threads_.begin();
        while (*iter != thr) ++iter;
        blocked_threads_.erase(iter);
        return nullptr;
      }
    }
  } else if (pending_messages_.empty()) {
    return nullptr;
  }

  // we have been unblocked because a message has arrived
  transport_message* msg = pending_messages_.front();
  debug_printf(
      sprockit::dbg::sumi,
      "Rank %d sumi queue %p has no pending messages - blocking poller %p",
      rank(), this, thr);
  pending_messages_.pop_front();
  process(msg);

  debug_printf(sprockit::dbg::sumi, "rank %d polling on %p returned msg %s",
               rank(), this, msg->to_string().c_str());

  return msg;
}

::sumi::deprecated::collective_done_message* transport::collective_block(
    ::sumi::collective::type_t ty, int tag, int cq_id) {
  // first we have to loop through the completion queue to see if it already
  // exists
  while (1) {
    auto& collective_cq = completion_queues_[collective_cq_id_];
    auto end = collective_cq.end();
    for (auto it = collective_cq.begin(); it != end; ++it) {
      ::sumi::deprecated::message* msg = *it;
      if (msg->class_type() == ::sumi::deprecated::message::collective_done) {
        // this is a collective done message
        auto cmsg =
            dynamic_cast<::sumi::deprecated::collective_done_message*>(msg);
        if (tag == cmsg->tag() && ty == cmsg->type()) {  // done!
          collective_cq.erase(it);
          return cmsg;
        }
      }
    }

    ::sumi::deprecated::message* msg = poll_new(true);  // blocking
    if (msg->class_type() == ::sumi::deprecated::message::collective_done) {
      // this is a collective done message
      auto cmsg =
          dynamic_cast<::sumi::deprecated::collective_done_message*>(msg);
      if (tag == cmsg->tag() && ty == cmsg->type()) {  // done!
        return cmsg;
      }
      collective_cq.push_back(msg);
    } else {
      completion_queues_[pt2pt_cq_id_].push_back(msg);
    }
  }
}

void transport::clean_up() {
  for (::sumi::collective* coll : todel_) {
    delete coll;
  }
  todel_.clear();
}

void transport::configure_send(int dst,
                               ::sumi::deprecated::message::payload_type_t ev,
                               ::sumi::deprecated::message* msg) {
  switch (ev) {
    // this is a bit weird here
    // we want to maintain notation of send/recv dst/src as in MPI
    // for an RDMA get the recver sends a request - thus it is sort of a sender
    // however we want to maintain the notion that the source of the
    // ::sumi::deprecated::message is where the data lives thus the destination
    // sends a request to the source
    case ::sumi::deprecated::message::rdma_get:
      msg->set_sender(dst);
      msg->set_recver(rank_);
      break;
    default:
      msg->set_sender(rank_);
      msg->set_recver(dst);
      break;
  }
  msg->set_payload_type(ev);

  if (msg->class_type() == ::sumi::deprecated::message::no_class) {
    spkt_throw_printf(sprockit::value_error,
                      "sending ::sumi::deprecated::message %s with no class",
                      msg->to_string().c_str());
  }
}

void transport::finish_collective(
    ::sumi::collective* coll,
    ::sumi::deprecated::collective_done_message* dmsg) {
  bool deliver_cq_msg;
  bool delete_collective;
  coll->actor_done(dmsg->comm_rank(), deliver_cq_msg, delete_collective);
  debug_printf(sprockit::dbg::sumi,
               "Rank %d finishing collective of type %s tag %d", rank_,
               ::sumi::collective::tostr(dmsg->type()), dmsg->tag());

  if (!deliver_cq_msg) return;

  coll->complete();
  ::sumi::collective::type_t ty = dmsg->type();
  int tag = dmsg->tag();
  if (delete_collective &&
      !coll->persistent()) {  // otherwise collective must exist FOREVER
    collectives_[ty].erase(tag);
    todel_.push_back(coll);
  }

  if (ty == ::sumi::collective::dynamic_tree_vote) {
    vote_done(coll->context(), dmsg);
  } else {
    completion_queues_[dmsg->recv_cq()].push_back(dmsg);
  }

  pending_collective_msgs_[ty].erase(tag);
  debug_printf(sprockit::dbg::sumi,
               "Rank %d finished collective of type %s tag %d", rank_,
               ::sumi::collective::tostr(dmsg->type()), dmsg->tag());
}

void transport::start_collective(::sumi::collective* coll) {
  coll->init_actors();
  int tag = coll->tag();
  ::sumi::collective::type_t ty = coll->type();
  // validate_collective(ty, tag);
  ::sumi::collective*& existing = collectives_[ty][tag];
  if (existing) {
    coll->start();
    existing->add_actors(coll);
    delete coll;
  } else {
    existing = coll;
    coll->start();
    deliver_pending(coll, tag, ty);
  }
}

void transport::validate_collective(::sumi::collective::type_t ty, int tag) {
  tag_to_collective_map::iterator it = collectives_[ty].find(tag);
  if (it == collectives_[ty].end()) {
    return;  // all good
  }

  ::sumi::collective* coll = it->second;
  if (!coll) {
    spkt_throw_printf(sprockit::illformed_error,
                      "sumi_api::validate_collective: lingering null "
                      "collective of type %s with tag %d",
                      ::sumi::collective::tostr(ty), tag);
  }

  if (coll->persistent() && coll->complete()) {
    return;  // all good
  }

  spkt_throw_printf(sprockit::illformed_error,
                    "sumi_api::validate_collective: cannot overwrite "
                    "collective of type %s with tag %d",
                    ::sumi::collective::tostr(ty), tag);
}

void transport::deliver_pending(::sumi::collective* coll, int tag,
                                ::sumi::collective::type_t ty) {
  std::list<::sumi::deprecated::collective_work_message*> pending =
      pending_collective_msgs_[ty][tag];
  pending_collective_msgs_[ty].erase(tag);
  std::list<::sumi::deprecated::collective_work_message*>::iterator it,
      end = pending.end();

  for (it = pending.begin(); it != end; ++it) {
    ::sumi::deprecated::collective_work_message* msg = *it;
    coll->recv(msg);
  }
}

void transport::validate_api() {
  if (!inited_ || finalized_) {
    sprockit::abort(
        "SUMI transport calling function while not inited or already "
        "finalized");
  }
}

::sumi::deprecated::message* transport::poll_new(bool blocking,
                                                 double timeout) {
  debug_printf(sprockit::dbg::sumi,
               "Rank %d polling for NEW ::sumi::deprecated::message", rank_);
  while (1) {
    ::sumi::deprecated::transport_message* tmsg =
        poll_pending_messages(blocking, timeout);
    if (tmsg) {
      ::sumi::deprecated::message* cq_notifier = handle(tmsg->take_payload());
      delete tmsg;
      if (cq_notifier || !blocking) {
        return cq_notifier;
      }
    } else {
      return nullptr;
    }
  }
}

void transport::vote_done(int context,
                          ::sumi::deprecated::collective_done_message* dmsg) {
  completion_queues_[dmsg->cq_id()].push_back(dmsg);
}

::sumi::deprecated::message* transport::handle(
    ::sumi::deprecated::message* msg) {
  debug_printf(sprockit::dbg::sumi,
               "Rank %d got ::sumi::deprecated::message %p of class %s, "
               "payload %s for sender %d, recver %d, send cq %d, recv cq %d",
               rank_, msg,
               ::sumi::deprecated::message::tostr(msg->class_type()),
               ::sumi::deprecated::message::tostr(msg->payload_type()),
               msg->sender(), msg->recver(), msg->send_cq(), msg->recv_cq());

  // we might have collectives to delete and cleanup
  if (!todel_.empty()) {
    clean_up();
  }

  switch (msg->class_type()) {
    case ::sumi::deprecated::message::bcast:
      // root initiated global broadcast of some metadata
      system_bcast(msg);
      return msg;
    case ::sumi::deprecated::message::terminate:
    case ::sumi::deprecated::message::collective_done:
    case ::sumi::deprecated::message::pt2pt: {
      switch (msg->payload_type()) {
        case ::sumi::deprecated::message::rdma_get:
        case ::sumi::deprecated::message::rdma_put:
        case ::sumi::deprecated::message::eager_payload:
          msg->write_sync_value();
          if (msg->needs_recv_ack()) {
            return msg;
          }
          break;
        case ::sumi::deprecated::message::rdma_get_ack:
        case ::sumi::deprecated::message::rdma_put_ack:
        case ::sumi::deprecated::message::eager_payload_ack:
          if (msg->needs_send_ack()) {
            return msg;
          }
          break;
        default:
          break;
      }
      return msg;
    }
    case ::sumi::deprecated::message::collective: {
      ::sumi::deprecated::collective_work_message* cmsg =
          dynamic_cast<::sumi::deprecated::collective_work_message*>(msg);
      if (cmsg->send_cq() == -1 && cmsg->recv_cq() == -1) abort();
      int tag = cmsg->tag();
      ::sumi::collective::type_t ty = cmsg->type();
      tag_to_collective_map::iterator it = collectives_[ty].find(tag);
      if (it == collectives_[ty].end()) {
        debug_printf(sprockit::dbg::sumi_collective_sendrecv,
                     "Rank %d, queuing %p %s %s from %d on tag %d for type %s",
                     rank_, msg,
                     ::sumi::deprecated::message::tostr(msg->payload_type()),
                     ::sumi::deprecated::message::tostr(msg->class_type()),
                     msg->sender(), tag, ::sumi::collective::tostr(ty));
        //::sumi::deprecated::message for collective we haven't started yet
        pending_collective_msgs_[ty][tag].push_back(cmsg);
        return nullptr;
      } else {
        ::sumi::collective* coll = it->second;
        auto& queue = completion_queues_[cmsg->cq_id()];
        int old_queue_size = queue.size();
        coll->recv(cmsg);
        if (queue.size() != old_queue_size) {
          // oh, a new collective finished
          ::sumi::deprecated::message* dmsg = queue.back();
          queue.pop_back();
          return dmsg;
        } else {
          return nullptr;
        }
      }
    }
    case ::sumi::deprecated::message::no_class: {
      spkt_throw_printf(
          sprockit::value_error,
          "transport::handle: got ::sumi::deprecated::message %s with no class "
          "of type %s",
          msg->to_string().c_str(),
          ::sumi::deprecated::message::tostr(msg->payload_type()));
    }
    default: {
      spkt_throw_printf(
          sprockit::value_error,
          "transport::handle: got unknown ::sumi::deprecated::message class %d",
          msg->class_type());
    }
  }
  return nullptr;
}

bool transport::skip_collective(::sumi::collective::type_t ty,
                                ::sumi::collective::config& cfg, void* dst,
                                void* src, int nelems, int type_size, int tag) {
  if (cfg.dom == nullptr) cfg.dom = global_domain_;

  if (cfg.dom->nproc() == 1) {
    if (dst && src && (dst != src)) {
      ::memcpy(dst, src, nelems * type_size);
    }
    auto dmsg = new ::sumi::deprecated::collective_done_message(
        tag, ty, cfg.dom, cfg.cq_id);
    dmsg->set_comm_rank(0);
    dmsg->set_result(dst);
    completion_queues_[cfg.cq_id].push_back(dmsg);
    return true;  // null indicates no work to do
  } else {
    return false;
  }
}

void transport::incoming_message(transport_message* msg) {
#if SSTMAC_COMM_SYNC_STATS
  if (msg) {
    msg->get_payload()->set_time_arrived(wall_time());
  }
#endif
  pending_messages_.push_back(msg);

  if (!blocked_threads_.empty()) {
    sstmac::sw::thread* next_thr = blocked_threads_.front();
    debug_printf(sprockit::dbg::sumi,
                 "sumi queue %p unblocking poller %p to handle message %s",
                 this, next_thr, msg->get_payload()->to_string().c_str());
    blocked_threads_.pop_front();
    os_->unblock(next_thr);
  } else {
    debug_printf(sprockit::dbg::sumi,
                 "sumi queue %p has no pollers to unblock for message %s", this,
                 msg->get_payload()->to_string().c_str());
  }
}

void transport::send_terminate(int dst) {
  // make a no-op
}

void transport::allreduce(void* dst, void* src, int nelems, int type_size,
                          int tag, ::sumi::reduce_fxn fxn,
                          ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::allreduce, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      allreduce_selector_ == nullptr
          ? new ::sumi::wilke_halving_allreduce
          : allreduce_selector_->select(cfg.dom->nproc(), nelems);
  coll->init(::sumi::collective::allreduce, this, dst, src, nelems, type_size,
             tag, cfg);
  coll->init_reduce(fxn);
  start_collective(coll);
}

void transport::reduce_scatter(void* dst, void* src, int nelems, int type_size,
                               int tag, ::sumi::reduce_fxn fxn,
                               ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::reduce_scatter, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      reduce_scatter_selector_ == nullptr
          ? new ::sumi::halving_reduce_scatter
          : reduce_scatter_selector_->select(cfg.dom->nproc(), nelems);
  coll->init(::sumi::collective::reduce_scatter, this, dst, src, nelems,
             type_size, tag, cfg);
  coll->init_reduce(fxn);
  start_collective(coll);
}

void transport::scan(void* dst, void* src, int nelems, int type_size, int tag,
                     ::sumi::reduce_fxn fxn, ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::scan, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      scan_selector_ == nullptr
          ? new ::sumi::simultaneous_btree_scan
          : scan_selector_->select(cfg.dom->nproc(), nelems);

  coll->init(::sumi::collective::scan, this, dst, src, nelems, type_size, tag,
             cfg);
  coll->init_reduce(fxn);
  start_collective(coll);
}

void transport::reduce(int root, void* dst, void* src, int nelems,
                       int type_size, int tag, ::sumi::reduce_fxn fxn,
                       ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::reduce, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      reduce_selector_ == nullptr
          ? new ::sumi::wilke_halving_reduce
          : reduce_selector_->select(cfg.dom->nproc(), nelems);
  coll->init(::sumi::collective::reduce, this, dst, src, nelems, type_size, tag,
             cfg);
  coll->init_root(root);
  coll->init_reduce(fxn);
  start_collective(coll);
}

void transport::bcast(int root, void* buf, int nelems, int type_size, int tag,
                      ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::bcast, cfg, buf, buf, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      bcast_selector_ == nullptr
          ? new ::sumi::binary_tree_bcast_collective
          : bcast_selector_->select(cfg.dom->nproc(), nelems);

  coll->init(::sumi::collective::bcast, this, buf, buf, nelems, type_size, tag,
             cfg);
  coll->init_root(root);
  start_collective(coll);
}

void transport::gatherv(int root, void* dst, void* src, int sendcnt,
                        int* recv_counts, int type_size, int tag,
                        ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::gatherv, cfg, dst, src, sendcnt,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      gatherv_selector_ == nullptr
          ? new ::sumi::btree_gatherv
          : gatherv_selector_->select(cfg.dom->nproc(), recv_counts);
  coll->init(::sumi::collective::gatherv, this, dst, src, sendcnt, type_size,
             tag, cfg);
  coll->init_root(root);
  coll->init_recv_counts(recv_counts);
  sprockit::abort("gatherv");
  start_collective(coll);
}

void transport::gather(int root, void* dst, void* src, int nelems,
                       int type_size, int tag, ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::gather, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      gather_selector_ == nullptr
          ? new ::sumi::btree_gather
          : gather_selector_->select(cfg.dom->nproc(), nelems);

  coll->init(::sumi::collective::gather, this, dst, src, nelems, type_size, tag,
             cfg);
  coll->init_root(root);
  start_collective(coll);
}

void transport::scatter(int root, void* dst, void* src, int nelems,
                        int type_size, int tag,
                        ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::scatter, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      scatter_selector_ == nullptr
          ? new ::sumi::btree_scatter
          : scatter_selector_->select(cfg.dom->nproc(), nelems);

  coll->init(::sumi::collective::scatter, this, dst, src, nelems, type_size,
             tag, cfg);
  coll->init_root(root);
  start_collective(coll);
}

void transport::scatterv(int root, void* dst, void* src, int* send_counts,
                         int recvcnt, int type_size, int tag,
                         ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::scatterv, cfg, dst, src, recvcnt,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      scatterv_selector_ == nullptr
          ? new ::sumi::btree_scatterv
          : scatterv_selector_->select(cfg.dom->nproc(), send_counts);

  coll->init(::sumi::collective::scatterv, this, dst, src, recvcnt, type_size,
             tag, cfg);
  coll->init_root(root);
  coll->init_send_counts(send_counts);
  sprockit::abort("scatterv");
  start_collective(coll);
}

void transport::alltoall(void* dst, void* src, int nelems, int type_size,
                         int tag, ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::alltoall, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      alltoall_selector_ == nullptr
          ? new ::sumi::bruck_alltoall_collective
          : alltoall_selector_->select(cfg.dom->nproc(), nelems);

  coll->init(::sumi::collective::alltoall, this, dst, src, nelems, type_size,
             tag, cfg);
  start_collective(coll);
}

void transport::alltoallv(void* dst, void* src, int* send_counts,
                          int* recv_counts, int type_size, int tag,
                          ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::alltoallv, cfg, dst, src,
                      send_counts[0], type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      alltoallv_selector_ == nullptr
          ? new ::sumi::direct_alltoallv_collective
          : alltoallv_selector_->select(cfg.dom->nproc(), send_counts);

  coll->init(::sumi::collective::alltoallv, this, dst, src, 0, type_size, tag,
             cfg);
  coll->init_recv_counts(recv_counts);
  coll->init_send_counts(send_counts);
  start_collective(coll);
}

void transport::allgather(void* dst, void* src, int nelems, int type_size,
                          int tag, ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::allgather, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      allgather_selector_ == nullptr
          ? new ::sumi::bruck_allgather_collective
          : allgather_selector_->select(cfg.dom->nproc(), nelems);

  coll->init(::sumi::collective::allgather, this, dst, src, nelems, type_size,
             tag, cfg);
  start_collective(coll);
}

void transport::allgatherv(void* dst, void* src, int* recv_counts,
                           int type_size, int tag,
                           ::sumi::collective::config cfg) {
  // if the allgatherv is skipped, we have a single recv count
  int nelems = *recv_counts;
  if (skip_collective(::sumi::collective::allgatherv, cfg, dst, src, nelems,
                      type_size, tag))
    return;

  ::sumi::dag_collective* coll =
      allgatherv_selector_ == nullptr
          ? new ::sumi::bruck_allgatherv_collective
          : allgatherv_selector_->select(cfg.dom->nproc(), recv_counts);

  // for the time being, ignore size and use the small-message algorithm
  coll->init(::sumi::collective::allgatherv, this, dst, src, 0, type_size, tag,
             cfg);
  coll->init_recv_counts(recv_counts);
  start_collective(coll);
}

void transport::barrier(int tag, ::sumi::collective::config cfg) {
  if (skip_collective(::sumi::collective::barrier, cfg, 0, 0, 0, 0, tag))
    return;

  ::sumi::dag_collective* coll = new ::sumi::bruck_allgather_collective;
  coll->init(::sumi::collective::barrier, this, 0, 0, 0, 0, tag, cfg);
  start_collective(coll);
}

void transport::wait_barrier(int tag) {
  if (nproc_ == 1) return;
  barrier(tag);
  collective_block(::sumi::collective::barrier, tag);
}

void transport::dynamic_tree_vote(int vote, int tag, ::sumi::vote_fxn fxn,
                                  ::sumi::collective::config cfg) {
  if (cfg.dom == nullptr) cfg.dom = global_domain_;

  if (cfg.dom->nproc() == 1) {
    auto dmsg = new ::sumi::deprecated::collective_done_message(
        tag, ::sumi::collective::dynamic_tree_vote, cfg.dom, cfg.cq_id);
    dmsg->set_comm_rank(0);
    dmsg->set_vote(vote);
    vote_done(cfg.context, dmsg);
    return;
  }

  ::sumi::dynamic_tree_vote_collective* voter =
      new ::sumi::dynamic_tree_vote_collective(vote, fxn, tag, this, cfg);
  start_collective(voter);
}

}  // namespace sumi
}  // namespace sstmac
