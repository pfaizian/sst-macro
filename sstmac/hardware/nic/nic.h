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

#ifndef SSTMAC_BACKENDS_NATIVE_COMPONENTS_NIC_NETWORKINTERFACE_H_INCLUDED
#define SSTMAC_BACKENDS_NATIVE_COMPONENTS_NIC_NETWORKINTERFACE_H_INCLUDED

#include <sstmac/common/timestamp.h>
#include <sstmac/hardware/node/node_fwd.h>
#include <sstmac/hardware/common/failable.h>
#include <sstmac/hardware/common/connection.h>
#include <sstmac/hardware/common/packet_fwd.h>
#include <sstmac/hardware/network/network_message_fwd.h>
#include <sstmac/hardware/logp/logp_switch_fwd.h>
#include <sstmac/common/stats/stat_spyplot_fwd.h>
#include <sstmac/common/stats/stat_histogram_fwd.h>
#include <sstmac/common/stats/stat_local_int_fwd.h>
#include <sstmac/common/stats/stat_global_int_fwd.h>
#include <sstmac/hardware/common/flow_fwd.h>
#include <sstmac/software/process/operating_system_fwd.h>
#include <sstmac/software/process/progress_queue.h>

#include <sprockit/debug.h>
#include <sprockit/factories/factory.h>

DeclareDebugSlot(nic);

#define nic_debug(...) \
  debug_printf(sprockit::dbg::nic, "NIC on node %d: %s", \
    int(addr()), sprockit::printf(__VA_ARGS__).c_str())

namespace sstmac {
namespace hw {

/**
 * A networkinterface is a delegate between a node and a server module.
 * This object helps ornament network operations with information about
 * the process (ppid) involved.
 */
class nic : public connectable_subcomponent
{
  DeclareFactory(nic,node*)
 public:
  typedef enum {
    Injection,
    LogP
  } Port;

  virtual std::string to_string() const override = 0;

  virtual ~nic();

  /**
   * @return A unique ID for the NIC positions. Opaque typedef to an int.
   */
  node_id addr() const {
    return my_addr_;
  }

  /**
   * @brief inject_send Perform an operation on the NIC.
   *  This assumes an exlcusive model of NIC use. If NIC is busy,
   *  operation may complete far in the future. If wishing to query for how busy the NIC is,
   *  use #next_free. Calls to hardware taking an OS parameter
   *  indicate 1) they MUST occur on a user-space software thread
   *  and 2) that they should us the os to block and compute
   * @param netmsg The message being injected
   * @param os     The OS to use form software compute delays
   */
  void inject_send(network_message* netmsg);

  event_handler* mtl_handler() const;

  virtual void mtl_handle(event* ev);

  /**
   * Delete all static variables associated with this class.
   * This should be registered with the runtime system via need_delete_statics
   */
  static void delete_statics();

  /**
    Perform the set of operations standard to all NICs.
    This then passes control off to a model-specific #do_send
    function to actually carry out the send
    @param payload The network message to send
  */
  void internode_send(network_message* payload);

  /**
    Perform the set of operations standard to all NICs
    for transfers within a node. This function is model-independent,
    unlike #internode_send which must pass control to #do_send.
   * @param payload
   */
  void intranode_send(network_message* payload);

  /**
   The NIC can either receive an entire message (bypass the byte-transfer layer)
   or it can receive packets.  If an incoming message is a full message (not a packet),
   it gets routed here. Unlike #recv_chunk, this has a default implementation and does not throw.
   @param chunk
   */
  void recv_message(network_message* msg);

  void send_to_node(network_message* netmsg);

  event_link* logp_link() const {
    return logp_link_;
  }

  virtual std::function<void(network_message*)> ctrl_ioctl();

  virtual std::function<void(network_message*)> data_ioctl();

 protected:
  nic(sprockit::sim_parameters* params, node* parent);

  node* parent() const {
    return parent_;
  }

  /**
    Start the message sending and inject it into the network
    This performs all model-specific work
    @param payload The network message to send
  */
  virtual void do_send(network_message* payload) = 0;

  bool negligible_size(int bytes) const {
    return bytes <= negligible_size_;
  }

 protected:
  node_id my_addr_;

  int negligible_size_;

  node* parent_;

 protected:
  event_link* logp_link_;

 private:
  stat_spyplot* spy_num_messages_;
  stat_spyplot* spy_bytes_;
  stat_histogram* hist_msg_size_;
  stat_local_int* local_bytes_sent_;
  stat_global_int* global_bytes_sent_;
  sw::single_progress_queue<network_message> queue_;

 protected:
  sw::operating_system* os_;

 private:
  /**
   For messages requiring an NIC ACK to signal that the message
   has injected into the interconnect.  Create an ack and
   send it up to the parent node.
   */
  void ack_send(network_message* payload);

  void record_message(network_message* msg);

  void finish_memcpy(network_message* msg);

};

class null_nic : public nic
{
 public:
  FactoryRegister("null", nic, null_nic, "implements a nic that models nothing - stand-in only")

  null_nic(sprockit::sim_parameters* params, node* parent) :
    nic(params, parent)
  {
  }

  std::string to_string() const override { return "null nic"; }

  void do_send(network_message* msg) override {}

  void connect_output(sprockit::sim_parameters *params, int src_outport, int dst_inport,
                      event_link *payload_link) override {}

  void connect_input(sprockit::sim_parameters *params, int src_outport, int dst_inport,
                     event_link *credit_link) override {}

  timestamp send_latency(sprockit::sim_parameters *params) const override { return timestamp(); }

  timestamp credit_latency(sprockit::sim_parameters *params) const override { return timestamp(); }

  link_handler* payload_handler(int port) override { return nullptr; }

  link_handler* credit_handler(int port) override { return nullptr; }
};

}
} // end of namespace sstmac.

#endif
