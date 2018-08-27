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

DeclareDebugSlot(sumi);

/**
 * SUMI = Simulator unified messagine interface
 * It is also the name for a solid ink in Japanese -
 * i.e. the substrate for sending messages!
 */
namespace sstmac {
namespace sumi {

struct enum_hash {
  template <typename T>
  inline typename std::enable_if<std::is_enum<T>::value, std::size_t>::type
  operator()(T const value) const {
    return static_cast<std::size_t>(value);
  }
};

class transport : public sstmac::sw::api {
  RegisterAPI("transport", transport);

 public:
  transport(sprockit::sim_parameters* params, sstmac::sw::software_id sid,
            sstmac::sw::operating_system* os);

  /*
   * TODO Clean up and comment
   * Functions that used to be in sumi::transport, but are now here
   */
  virtual void init() override;
  virtual void finish() override;
  void deadlock_check();
  int allocate_cq();

  /**
   Send a message directly to a destination node.
   This assumes there is buffer space available at the destination to eagerly
   receive.
   * @param dst         The destination of the message
   * @param ev          An enum for the type of payload carried
   * @param needs_ack   Whether an ack should be generated notifying that the
   message was fully injected
   * @param msg         The message to send (full message at the MTL layer)
   */
  void smsg_send(int dst, ::sumi::deprecated::message::payload_type_t ev,
                 ::sumi::deprecated::message* msg, int send_cq, int recv_cq);

  /**
   Helper function for #smsg_send. Directly send a message header.
   * @param dst
   * @param msg
   */
  void send_header(int dst, ::sumi::deprecated::message* msg, int send_cq,
                   int recv_cq);
  /**
   Helper function for #smsg_send. Directly send an actual data payload.
   * @param dst
   * @param msg
   */
  void send_payload(int dst, ::sumi::deprecated::message* msg, int send_cq,
                    int recv_cq);

  /**
   Put a message directly to the destination node.
   This assumes the application has properly configured local/remote buffers for
   the transfer.
   * @param dst      Where the data is being put (destination)
   * @param msg      The message to send (full message at the MTL layer).
   *                 Should have local/remote buffers configured for RDMA.
   * @param needs_send_ack  Whether an ack should be generated send-side when
   the message is fully injected
   * @param needs_recv_ack  Whether an ack should be generated recv-side when
   the message is fully received
   */
  void rdma_put(int dst, ::sumi::deprecated::message* msg, int send_cq,
                int recv_cq);

  /**
   Get a message directly from the source node.
   This assumes the application has properly configured local/remote buffers for
   the transfer.
   * @param src      Where the data is being put (destination)
   * @param msg      The message to send (full message at the MTL layer).
   *                 Should have local/remote buffers configured for RDMA.
   * @param send_cq  Where an ack should be generated send-side (source) when
   the message is fully injected
   * @param recv_cq  Where an ack should be generated recv-side when the message
   is fully received
   */
  void rdma_get(int src, ::sumi::deprecated::message* msg, int send_cq,
                int recv_cq);

  void nvram_get(int src, ::sumi::deprecated::message* msg);

  /**
   Check if a message has been received on a specific completion queue.
   Message returned is removed from the internal queue.
   Successive calls to the function do NOT return the same message.
   @param blocking Whether to block until a message is received
   @param cq_id The specific completion queue to check
   @param timeout  An optional timeout - only valid with blocking
   @return    The next message to be received, null if no messages
  */
  ::sumi::deprecated::message* poll(bool blocking, int cq_id,
                                    double timeout = -1);

  /**
   Check all completion queues if a message has been received.
   Message returned is removed from the internal queue.
   Successive calls to the function do NOT return the same message.
   @param blocking Whether to block until a message is received
   @param timeout  An optional timeout - only valid with blocking
   @return    The next message to be received, null if no messages
  */
  ::sumi::deprecated::message* poll(bool blocking, double timeout = -1);

  /**
   Block until a message is received.
   Returns immediately if message already waiting.
   Message returned is removed from the internal queue.
   Successive calls to the function do NOT return the same message.
   @param cq_id The specific completion queue to check
   @param timeout   Timeout in seconds
   @return          The next message to be received. Message is NULL on timeout
  */
  ::sumi::deprecated::message* blocking_poll(int cq_id, double timeout = -1);

  template <class T>
  T* poll(const char* file, int line, const char* cls) {
    ::sumi::deprecated::message* msg = poll(true);
    T* result = dynamic_cast<T*>(msg);
    if (!result) {
      poll_cast_error(file, line, cls, msg);
    }
    return result;
  }

  template <class T>
  T* poll(double timeout, const char* file, int line, const char* cls) {
    ::sumi::deprecated::message* msg = poll(true, timeout);
    T* result = dynamic_cast<T*>(msg);
    if (msg && !result) {
      poll_cast_error(file, line, cls, msg);
    }
    return result;
  }

#define SUMI_POLL(tport, msgtype) \
  tport->poll<msgtype>(__FILE__, __LINE__, #msgtype)
#define SUMI_POLL_TIME(tport, msgtype, to) \
  tport->poll<msgtype>(to, __FILE__, __LINE__, #msgtype)

  void cq_push_back(int cq_id, ::sumi::deprecated::message* msg) {
    completion_queues_[cq_id].push_back(msg);
  }

  std::list<::sumi::deprecated::message*>::iterator cq_begin(int cq_id) {
    return completion_queues_[cq_id].begin();
  }

  std::list<::sumi::deprecated::message*>::iterator cq_end(int cq_id) {
    return completion_queues_[cq_id].end();
  }

  ::sumi::deprecated::message* pop(
      int cq_id, std::list<::sumi::deprecated::message*>::iterator it) {
    ::sumi::deprecated::message* msg = *it;
    completion_queues_[cq_id].erase(it);
    return msg;
  }

  ::sumi::deprecated::message* poll(
      ::sumi::deprecated::message::payload_type_t ty, bool blocking, int cq_id,
      double timeout = -1);

  ::sumi::deprecated::message* poll_new(
      ::sumi::deprecated::message::payload_type_t ty, bool blocking, int cq_id,
      double timeout);

  ::sumi::deprecated::message* poll_new(bool blocking, int cq_id,
                                        double timeout);

  bool use_eager_protocol(uint64_t byte_length) const {
    return byte_length < eager_cutoff_;
  }

  void set_eager_cutoff(uint64_t bytes) { eager_cutoff_ = bytes; }

  bool use_put_protocol() const { return use_put_protocol_; }

  bool use_get_protocol() const { return !use_put_protocol_; }

  void set_put_protocol(bool flag) { use_put_protocol_ = flag; }

  void set_use_hardware_ack(bool flag);

  virtual bool supports_hardware_ack() const { return false; }

  void send_self_terminate();

  template <template <class> class VoteOp>
  void vote(int vote, int tag,
            ::sumi::collective::config cfg = ::sumi::collective::cfg()) {
    using op_class_type = VoteOp<int>;
    dynamic_tree_vote(vote, tag, &op_class_type::op, cfg);
  }

  template <typename data_t, template <typename> class Op>
  void reduce_scatter(
      void* dst, void* src, int nelems, int tag,
      ::sumi::collective::config cfg = ::sumi::collective::cfg()) {
    using op_class_type = ::sumi::ReduceOp<Op, data_t>;
    reduce_scatter(dst, src, nelems, sizeof(data_t), tag, &op_class_type::op,
                   cfg);
  }

  void system_bcast(::sumi::deprecated::message* msg);
  int rank() const { return rank_; }

  int allocate_global_collective_tag() {
    system_collective_tag_--;
    if (system_collective_tag_ >= 0) system_collective_tag_ = -1;
    return system_collective_tag_;
  }

  int nproc() const { return nproc_; }

  ::sumi::communicator* global_dom() const { return global_domain_; }

  /**
   * The cutoff for message size in bytes
   * for switching between an eager protocol and a rendezvous RDMA protocol
   * @return
   */
  uint32_t eager_cutoff() const { return eager_cutoff_; }

  void notify_collective_done(::sumi::deprecated::collective_done_message* msg);

  /**
   * @brief handle Receive some version of point-2-point message.
   *  Return either the original message or a collective done message
   *  if the message is part of a collective and the collective is done
   * @param msg A point to point message that might be part of a collective
   * @return Null, if collective message and collective is not done
   */
  ::sumi::deprecated::message* handle(::sumi::deprecated::message* msg);

  virtual ::sumi::public_buffer allocate_public_buffer(int size) {
    return ::sumi::public_buffer(::malloc(size));
  }

 private:
  void configure_send(int dst, ::sumi::deprecated::message::payload_type_t ev,
                      ::sumi::deprecated::message* msg);
  void clean_up();
  void validate_api();
  /**
   * @brief poll_new Return a new message not already in the completion queue
   * @param blocking
   * @param timeout
   * @return A message if found, null message if non-blocking or timed out
   */
  ::sumi::deprecated::message* poll_new(bool blocking, double timeout = -1);

  void poll_cast_error(const char* file, int line, const char* cls,
                       ::sumi::deprecated::message* msg) {
    spkt_throw_printf(
        sprockit::value_error,
        "Could not cast incoming ::sumi::deprecated::message to type %s\n"
        "Got %s\n"
        "%s:%d",
        cls, msg->to_string().c_str(), file, line);
  }

 public:
  void finish_collective(::sumi::collective* coll,
                         ::sumi::deprecated::collective_done_message* dmsg);
  void start_collective(::sumi::collective* coll);
  void validate_collective(::sumi::collective::type_t ty, int tag);
  void deliver_pending(::sumi::collective* coll, int tag,
                       ::sumi::collective::type_t ty);
  /**
   * Helper function for doing operations necessary to close out a heartbeat
   * @param dmsg
   */
  void vote_done(int context,
                 ::sumi::deprecated::collective_done_message* dmsg);

  bool skip_collective(::sumi::collective::type_t ty,
                       ::sumi::collective::config& cfg, void* dst, void* src,
                       int nelems, int type_size, int tag);

  transport(sprockit::sim_parameters* params);

#if SSTMAC_COMM_SYNC_STATS
 public:
  virtual void collect_sync_delays(double wait_start, message* msg) {}

  virtual void start_collective_sync_delays() {}
#endif
  /*
   *
   * End cleanup
   *
   *
   */
 public:
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
      ::sumi::collective::type_t ty, int tag, int cq_id = 0);

  double wall_time() const;

  /**
   * @brief poll_pending_messages
   * @param blocking  Whether to block if no messages are available
   * @param timeout   How long to block if no messages found
   * @return A transport message carrying a message
   */
  ::sumi::deprecated::transport_message* poll_pending_messages(
      bool blocking, double timeout = -1);

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

  virtual void memcopy(uint64_t bytes);

  void pin_rdma(uint64_t bytes);

 private:
  virtual void do_smsg_send(int dst, ::sumi::deprecated::message* msg);
  virtual void do_rdma_put(int dst, ::sumi::deprecated::message* msg);
  virtual void do_rdma_get(int src, ::sumi::deprecated::message* msg);
  virtual void do_nvram_get(int src, ::sumi::deprecated::message* msg);

  void send_terminate(int dst);

  virtual void go_die();
  virtual void go_revive();

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

  ::sumi::public_buffer make_public_buffer(void* buffer, int size) {
    pin_rdma(size);
    return ::sumi::public_buffer(buffer);
  }

  void unmake_public_buffer(::sumi::public_buffer buf, int size) {}

  void free_public_buffer(::sumi::public_buffer buf, int size) {
    ::free(buf.ptr);
  }

  int* nidlist() const;

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

  /*
   * TODO Clean up comments here
   * Variables that use to be in sumi::transport
   */
  bool inited_;
  bool finalized_;
  int rank_;
  int nproc_;

  template <typename Key, typename Value>
  using spkt_enum_map = std::unordered_map<Key, Value, enum_hash>;

  using tag_to_collective_map = std::unordered_map<int, ::sumi::collective*>;
  using collective_map =
      spkt_enum_map<::sumi::collective::type_t, tag_to_collective_map>;
  collective_map collectives_;

  using tag_to_pending_map = std::unordered_map<
      int, std::list<::sumi::deprecated::collective_work_message*>>;
  using pending_map =
      spkt_enum_map<::sumi::collective::type_t, tag_to_pending_map>;
  pending_map pending_collective_msgs_;
  std::list<::sumi::collective*> todel_;

  std::vector<std::list<::sumi::deprecated::message*>> completion_queues_;
  uint32_t eager_cutoff_;
  bool use_put_protocol_;
  bool use_hardware_ack_;
  ::sumi::communicator* global_domain_;
  int system_collective_tag_;

  /* End comment clean up */

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

};  // class transport

class terminate_exception : public std::exception {};
static inline bool isNonNull(void* buf) { return buf != nullptr; }
static inline bool isNull(void* buf) { return !(isNonNull(buf)); }

}  // namespace sumi
}  // namespace sstmac

#endif  // sumi_SUMI_TRANSPORT_H
