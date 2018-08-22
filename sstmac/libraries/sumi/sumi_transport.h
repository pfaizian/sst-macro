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

#ifndef sumi_SUMI_TRANSPORT_H
#define sumi_SUMI_TRANSPORT_H

#include <sstmac/common/stats/stat_spyplot_fwd.h>
#include <sstmac/hardware/network/network_message_fwd.h>
#include <sstmac/libraries/sumi/message_fwd.h>
#include <sstmac/software/api/api.h>
#include <sstmac/software/launch/job_launcher_fwd.h>
#include <sstmac/software/libraries/service.h>
#include <sstmac/software/process/key.h>
#include <sumi/collective.h>
#include <sumi/comm_functions.h>
#include <sumi/message_fwd.h>
#include <sumi/transport.h>

/**
 * SUMI = Simulator unified messagine interface
 * It is also the name for a solid ink in Japanese -
 * i.e. the substrate for sending messages!
 */
namespace sstmac {
namespace sumi {

class transport : public sstmac::sw::api, public ::sumi::deprecated::transport {
  RegisterAPI("transport", transport);

 public:
  transport(sprockit::sim_parameters* params, sstmac::sw::software_id sid,
            sstmac::sw::operating_system* os);

  virtual void init() override;

  virtual void finish() override;

  virtual ~transport();

  int pt2pt_cq_id() const { return pt2pt_cq_id_; }

  int collective_cq_id() const { return collective_cq_id_; }

  void incoming_event(event* ev) override;

  void compute(timestamp t);

  void client_server_send(int dest_rank, node_id dest_node, int dest_app,
                          ::sumi::deprecated::message* msg);

  void client_server_rdma_put(int dest_rank, node_id dest_node, int dest_app,
                              ::sumi::deprecated::message* msg);

  /**
   * Block on a collective of a particular type and tag
   * until that collective is complete
   * @param ty
   * @param tag
   * @return
   */
  ::sumi::deprecated::collective_done_message* collective_block(
      ::sumi::collective::type_t ty, int tag, int cq_id = 0) override;

  double wall_time() const override;

  ::sumi::deprecated::transport_message* poll_pending_messages(
      bool blocking, double timeout = -1) override;

  /**
   * @brief send Intra-app. Send within the same process launch (i.e. intra-comm
   * MPI_COMM_WORLD). This contrasts with client_server_send which exchanges
   * messages between different apps
   * @param byte_length
   * @param msg
   * @param ty
   * @param dst
   * @param needs_ack
   */
  void send(uint64_t byte_length, ::sumi::deprecated::message* msg, int ty,
            int dst);

  void incoming_message(transport_message* msg);

  void shutdown_server(int dest_rank, node_id dest_node, int dest_app);

  std::string server_libname() const { return server_libname_; }

  event_scheduler* des_scheduler() const;

  void memcopy(uint64_t bytes) override;

  void pin_rdma(uint64_t bytes);

 private:
  void do_smsg_send(int dst, ::sumi::deprecated::message* msg) override;

  void do_rdma_put(int dst, ::sumi::deprecated::message* msg) override;

  void do_rdma_get(int src, ::sumi::deprecated::message* msg) override;

  void do_nvram_get(int src, ::sumi::deprecated::message* msg) override;

  void send_terminate(int dst) override;

  void go_die() override;

  void go_revive() override;

 public:
  transport(sprockit::sim_parameters* params, const char* prefix,
            sstmac::sw::software_id sid, sstmac::sw::operating_system* os);

  /**
   * @brief transport Ctor with strict library name. We do not create a server
   * here. Since this has been explicitly named, messages will be directly to a
   * named library.
   * @param params
   * @param libname
   * @param sid
   * @param os
   */
  transport(sprockit::sim_parameters* params, const std::string& libname,
            sstmac::sw::software_id sid, sstmac::sw::operating_system* os);

  ::sumi::public_buffer make_public_buffer(void* buffer, int size) override {
    pin_rdma(size);
    return ::sumi::public_buffer(buffer);
  }

  void unmake_public_buffer(::sumi::public_buffer buf, int size) override {}

  void free_public_buffer(::sumi::public_buffer buf, int size) override {
    ::free(buf.ptr);
  }

  int* nidlist() const override;

  //
  // Functions that use to be in sumi::transport
  //

  /**
   * The total size of the input/result buffer in bytes is nelems*type_size
   * This always run in a fault-tolerant fashion
   * This uses a dynamic tree structure that reconnects partners when failures
   * are detected
   * @param vote The vote (currently restricted to integer) from this process
   * @param nelems The number of elements in the input and result buffer.
   * @param tag A unique tag identifier for the collective
   * @param fxn The function that merges vote, usually AND, OR, MAX, MIN
   * @param context The context (i.e. initial set of failed procs)
   */
  void dynamic_tree_vote(
      int vote, int tag, ::sumi::vote_fxn fxn,
      ::sumi::collective::config cfg = ::sumi::collective::cfg());

  /**
   * The total size of the input/result buffer in bytes is nelems*type_size
   * @param dst  Buffer for the result. Can be NULL to ignore payloads.
   * @param src  Buffer for the input. Can be NULL to ignore payloads.
   *             Automatically memcpy from src to dst.
   * @param nelems The number of elements in the input and result buffer.
   * @param type_size The size of the input type, i.e. sizeof(int),
   * sizeof(double)
   * @param tag A unique tag identifier for the collective
   * @param fxn The function that will actually perform the reduction
   * @param fault_aware Whether to execute in a fault-aware fashion to detect
   * failures
   * @param context The context (i.e. initial set of failed procs)
   */
  void allreduce(void* dst, void* src, int nelems, int type_size, int tag,
                 ::sumi::reduce_fxn fxn,
                 ::sumi::collective::config cfg = ::sumi::collective::cfg());

  /**
   * The total size of the input buffer in bytes is nelems*type_size*comm_size
   * @param dst  Buffer for the result. Can be NULL to ignore payloads.
   * @param src  Buffer for the input. Can be NULL to ignore payloads.
   *             Automatically memcpy from src to dst.
   * @param nelems The number of elements in the result buffer at the end
   * @param type_size The size of the input type, i.e. sizeof(int),
   * sizeof(double)
   * @param tag A unique tag identifier for the collective
   * @param fxn The function that will actually perform the reduction
   * @param fault_aware Whether to execute in a fault-aware fashion to detect
   * failures
   * @param context The context (i.e. initial set of failed procs)
   */
  void reduce_scatter(
      void* dst, void* src, int nelems, int type_size, int tag,
      ::sumi::reduce_fxn fxn,
      ::sumi::collective::config cfg = ::sumi::collective::cfg());

  template <typename data_t, template <typename> class Op>
  void reduce_scatter(
      void* dst, void* src, int nelems, int tag,
      ::sumi::collective::config cfg = ::sumi::collective::cfg()) {
    typedef ::sumi::ReduceOp<Op, data_t> op_class_type;
    reduce_scatter(dst, src, nelems, sizeof(data_t), tag, &op_class_type::op,
                   cfg);
  }

  /**
   * The total size of the input/result buffer in bytes is nelems*type_size
   * @param dst  Buffer for the result. Can be NULL to ignore payloads.
   * @param src  Buffer for the input. Can be NULL to ignore payloads.
   *             Automatically memcpy from src to dst.
   * @param nelems The number of elements in the input and result buffer.
   * @param type_size The size of the input type, i.e. sizeof(int),
   * sizeof(double)
   * @param tag A unique tag identifier for the collective
   * @param fxn The function that will actually perform the reduction
   * @param fault_aware Whether to execute in a fault-aware fashion to detect
   * failures
   * @param context The context (i.e. initial set of failed procs)
   */
  void scan(void* dst, void* src, int nelems, int type_size, int tag,
            ::sumi::reduce_fxn fxn,
            ::sumi::collective::config cfg = ::sumi::collective::cfg());

  template <typename data_t, template <typename> class Op>
  void scan(void* dst, void* src, int nelems, int tag,
            ::sumi::collective::config cfg = ::sumi::collective::cfg()) {
    typedef ::sumi::ReduceOp<Op, data_t> op_class_type;
    scan(dst, src, nelems, sizeof(data_t), tag, &op_class_type::op, cfg);
  }

  virtual void reduce(
      int root, void* dst, void* src, int nelems, int type_size, int tag,
      ::sumi::reduce_fxn fxn,
      ::sumi::collective::config cfg = ::sumi::collective::cfg());

  template <typename data_t, template <typename> class Op>
  void reduce(int root, void* dst, void* src, int nelems, int tag,
              ::sumi::collective::config cfg = ::sumi::collective::cfg()) {
    typedef ::sumi::ReduceOp<Op, data_t> op_class_type;
    reduce(root, dst, src, nelems, sizeof(data_t), tag, &op_class_type::op,
           cfg);
  }

  void bcast(int root, void* buf, int nelems, int type_size, int tag,
             ::sumi::collective::config cfg = ::sumi::collective::cfg());

  void gatherv(int root, void* dst, void* src, int sendcnt, int* recv_counts,
               int type_size, int tag,
               ::sumi::collective::config = ::sumi::collective::cfg());

  void gather(int root, void* dst, void* src, int nelems, int type_size,
              int tag, ::sumi::collective::config = ::sumi::collective::cfg());

  void scatter(int root, void* dst, void* src, int nelems, int type_size,
               int tag, ::sumi::collective::config = ::sumi::collective::cfg());

  void scatterv(int root, void* dst, void* src, int* send_counts, int recvcnt,
                int type_size, int tag,
                ::sumi::collective::config = ::sumi::collective::cfg());

  /**
   * The total size of the input/result buffer in bytes is nelems*type_size
   * @param dst  Buffer for the result. Can be NULL to ignore payloads.
   * @param src  Buffer for the input. Can be NULL to ignore payloads. This need
   * not be public! Automatically memcpy from src to public facing dst.
   * @param nelems The number of elements in the input and result buffer.
   * @param type_size The size of the input type, i.e. sizeof(int),
   * sizeof(double)
   * @param tag A unique tag identifier for the collective
   * @param fault_aware Whether to execute in a fault-aware fashion to detect
   * failures
   * @param context The context (i.e. initial set of failed procs)
   */
  void allgather(void* dst, void* src, int nelems, int type_size, int tag,
                 ::sumi::collective::config = ::sumi::collective::cfg());

  void allgatherv(void* dst, void* src, int* recv_counts, int type_size,
                  int tag,
                  ::sumi::collective::config = ::sumi::collective::cfg());

  void alltoall(void* dst, void* src, int nelems, int type_size, int tag,
                ::sumi::collective::config = ::sumi::collective::cfg());

  void alltoallv(void* dst, void* src, int* send_counts, int* recv_counts,
                 int type_size, int tag,
                 ::sumi::collective::config = ::sumi::collective::cfg());

  /**
   * Essentially just executes a zero-byte allgather.
   * @param tag
   * @param fault_aware
   */
  void barrier(int tag, ::sumi::collective::config = ::sumi::collective::cfg());

  void wait_barrier(int tag);

 private:
  static ::sumi::collective_algorithm_selector* allgather_selector_;
  static ::sumi::collective_algorithm_selector* alltoall_selector_;
  static ::sumi::collective_algorithm_selector* alltoallv_selector_;
  static ::sumi::collective_algorithm_selector* allreduce_selector_;
  static ::sumi::collective_algorithm_selector* reduce_scatter_selector_;
  static ::sumi::collective_algorithm_selector* scan_selector_;
  static ::sumi::collective_algorithm_selector* allgatherv_selector_;
  static ::sumi::collective_algorithm_selector* bcast_selector_;
  static ::sumi::collective_algorithm_selector* gather_selector_;
  static ::sumi::collective_algorithm_selector* gatherv_selector_;
  static ::sumi::collective_algorithm_selector* reduce_selector_;
  static ::sumi::collective_algorithm_selector* scatter_selector_;
  static ::sumi::collective_algorithm_selector* scatterv_selector_;

  void send(uint64_t byte_length, int dest_rank, node_id dest_node,
            int dest_app, ::sumi::deprecated::message* msg, int ty);

  void process(sstmac::transport_message* msg);

  void ctor_common(sstmac::sw::software_id sid);

  static sstmac::sw::ftq_tag transport_tag;
  static sstmac::sw::ftq_tag poll_delay_tag;

  std::string server_libname_;

  sstmac::sw::task_mapping_ptr rank_mapper_;

  std::list<transport_message*> pending_messages_;

  std::list<sstmac::sw::thread*> blocked_threads_;

  uint32_t component_id_;

  timestamp post_rdma_delay_;

  timestamp post_header_delay_;

  timestamp poll_delay_;

  sstmac::sw::lib_compute_time* user_lib_time_;

  sstmac::stat_spyplot* spy_num_messages_;

  sstmac::stat_spyplot* spy_bytes_;

  int collective_cq_id_;

  int pt2pt_cq_id_;

  sstmac::timestamp rdma_pin_latency_;
  sstmac::timestamp rdma_page_delay_;
  int page_size_;
  bool pin_delay_;

};

}  // namespace sumi
}  // namespace sstmac

#endif  // sumi_SUMI_TRANSPORT_H
