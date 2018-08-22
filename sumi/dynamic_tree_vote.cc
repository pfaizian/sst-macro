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

#include <sumi/dynamic_tree_vote.h>
#include <sumi/communicator.h>
#include <sprockit/stl_string.h>
#include <algorithm>
#include <sstmac/libraries/sumi/sumi_transport.h>

/*
#undef debug_printf
#define debug_printf(flags, ...) \
  if (tag_ == 221) std::cout << sprockit::spkt_printf(__VA_ARGS__) << std::endl
*/

using namespace sprockit::dbg;

namespace sumi {

#define enumcase(x) case x: return #x
const char*
dynamic_tree_vote_message::tostr(type_t ty)
{
  switch(ty)
  {
  enumcase(down_vote);
  enumcase(up_vote);
  enumcase(request);
  }
  spkt_throw_printf(sprockit::value_error,
      "dynamic_tree_vote_message: unknown type %d", ty);
}

const char*
dynamic_tree_vote_actor::tostr(stage_t stage)
{
  switch(stage)
  {
  enumcase(recv_vote);
  enumcase(up_vote);
  enumcase(down_vote);
  }
  spkt_throw_printf(sprockit::value_error,
      "dynamic_tree_vote_actor: unknown stage %d", stage);
}

void*
dynamic_tree_vote_message::recv_buffer() const
{
  sprockit::abort("dynamic_tree_vote_message: does not have recv buffer");
  return nullptr;
}

void
dynamic_tree_vote_message::serialize_order(sstmac::serializer &ser)
{
  ser & vote_;
  ser & type_;
  collective_work_message::serialize_order(ser);
}

void
dynamic_tree_vote_actor::position(int rank, int &level, int &branch)
{
  level = 0;
  int level_upper_bound = 1;
  int stride = 1;
  while (rank >= level_upper_bound){
    ++level;
    stride *= 2;
    level_upper_bound += stride;
  }
  int level_lower_bound = level_upper_bound - stride;
  branch = rank - level_lower_bound;
}

void
dynamic_tree_vote_actor::level_stats(int level, int& start, int& size)
{
  int lower_bound = 0;
  int stride = 1;
  for (int i=0; i < level; ++i){
    lower_bound += stride;
    stride *= 2;
  }
  start = lower_bound;
  size = stride;
}

int
dynamic_tree_vote_actor::level_lower_bound(int level)
{
  int bound, ignore;
  level_stats(level, bound, ignore);
  return bound;
}

int
dynamic_tree_vote_actor::down_partner(int level, int branch)
{
  int down_branch = branch*2;
  int down_level_start = level_lower_bound(level + 1);
  return down_level_start + down_branch;
}

int
dynamic_tree_vote_actor::up_partner(int level, int branch)
{
  int up_branch = branch / 2;
  int up_level_start = level_lower_bound(level - 1);
  return up_level_start + up_branch;
}

int
dynamic_tree_vote_actor::up_partner(int rank)
{
  int level, branch;
  position(rank, level, branch);
  return up_partner(level, branch);
}

dynamic_tree_vote_actor::dynamic_tree_vote_actor(int vote,
    vote_fxn fxn, int tag, ::sstmac::sumi::transport* my_api,
    const collective::config& cfg) :
  collective_actor(my_api, tag, cfg), //true for always fault-aware
  vote_(vote),
  fxn_(fxn),
  tag_(tag),
  stage_(recv_vote)
{
  int ignore;
  position(dense_me_, my_level_, my_branch_);
  //figure out the bottom level from nproc
  position(dense_nproc_-1,bottom_level_,ignore);
  level_stats(my_level_, my_level_start_, my_level_size_);

  if (my_level_ != bottom_level_){
    int partner = down_partner(my_level_, my_branch_);
    if (partner < dense_nproc_){
      down_partners_.insert(partner);
    }
    ++partner;
    if (partner < dense_nproc_){
      down_partners_.insert(partner);
    }
  }

  if (my_level_ == 0){
    up_partner_ = -1;
  } else {
    up_partner_ = up_partner(my_level_, my_branch_);
  }

  debug_printf(sumi_collective | sumi_vote,
    "Rank %s from nproc=%d(%d) is at level=%d start=%d bottom=%d in branch=%d up=%d down=%s on tag=%d ",
    rank_str().c_str(), dense_nproc_, my_api_->nproc(),
    my_level_, my_level_start_, bottom_level_ + 1,
    my_branch_, up_partner_, stl_string(down_partners_).c_str(), tag_);
}

void
dynamic_tree_vote_actor::send_messages(dynamic_tree_vote_message::type_t ty, const std::set<int>& partners)
{
  for (auto partner : partners){
    send_message(ty, partner);
  }
}

bool
dynamic_tree_vote_actor::up_vote_ready()
{
  //do a set diff - we may actually get MORE votes than we need
  //due to weird failure scenarios
  if (down_partners_.size() > up_votes_recved_.size()){
    return false; //might as well do an easy check
  }

  //this produces down_partners_ - up_votes_recved_
  //in other words, if up_votes_receved contains all the down partners
  //the result set will be empty
  std::set<int> pending;
  std::set_difference(down_partners_.begin(), down_partners_.end(),
                      up_votes_recved_.begin(), up_votes_recved_.end(),
                      std::inserter(pending, pending.end()));
  return pending.empty();
}

void
dynamic_tree_vote_actor::send_up_votes()
{
  if (my_level_ == 0){
    //nothing to do - go straight to sending down
    send_down_votes();
  } else if (stage_ == recv_vote) {
    stage_ = up_vote;
    //it could happen that we are triggered to send up votes
    //because we acquired new down partners
    //when in reailty we have already sent the up vote
    debug_printf(sumi_collective | sumi_vote,
      "Rank %s sending up vote %d to partner=%d on tag=%d ",
      rank_str().c_str(), vote_, up_partner_, tag_);
    send_message(dynamic_tree_vote_message::up_vote, up_partner_);
  }
}

void
dynamic_tree_vote_actor::start()
{
  stage_ = recv_vote;

  //this set can get modified - make a temporary
  //we might be at the bottom of tree or because of failures
  //we can just go ahead and sent up votes
  if (up_vote_ready()){
    send_up_votes();
  }
}

void
dynamic_tree_vote_actor::send_message(dynamic_tree_vote_message::type_t ty, int virtual_dst)
{
  auto msg = new dynamic_tree_vote_message(vote_, ty, tag_, dense_me_, virtual_dst);
  int global_phys_dst = global_rank(virtual_dst);
  my_api_->send_payload(global_phys_dst, msg, ::sumi::deprecated::message::no_ack, cfg_.cq_id);
}

void
dynamic_tree_vote_actor::recv_result(dynamic_tree_vote_message* msg)
{
  vote_ = msg->vote();
}

void
dynamic_tree_vote_actor::merge_result(dynamic_tree_vote_message* msg)
{  
  if (stage_ == recv_vote) (fxn_)(vote_, msg->vote());
}

void
dynamic_tree_vote_actor::put_done_notification()
{
  auto msg = new ::sumi::deprecated::collective_done_message(tag_, collective::dynamic_tree_vote, cfg_.dom, cfg_.cq_id);
  msg->set_vote(vote_);
  msg->set_comm_rank(cfg_.dom->my_comm_rank());
  my_api_->notify_collective_done(msg);
}

void
dynamic_tree_vote_actor::finish()
{
  if (complete_){
    spkt_throw_printf(sprockit::illformed_error,
        "dynamic_vote_actor::finish: tag=%d already complete on rank %d",
        tag_, my_api_->rank());
  }
  complete_ = true;
  put_done_notification();
}


void
dynamic_tree_vote_actor::recv_down_vote(dynamic_tree_vote_message* msg)
{
  if (stage_ == down_vote){
    if (vote_ != msg->vote()){
      spkt_abort_printf("dynamic_vote_actor::inconsistent_down_vote: %d %d",
        msg->vote(), vote_);
    }
    return;  //already got this
  }

  recv_result(msg);
  //where I got this from is totally irrelevant - I'm good to go
  send_down_votes();
}

void
dynamic_tree_vote_actor::send_down_votes()
{
  debug_printf(sumi_collective | sumi_vote,
    "Rank %s sending down vote %d to down=%s on tag=%d ",
    rank_str().c_str(), vote_,
    stl_string(down_partners_).c_str(), tag_);

  stage_ = down_vote;
  send_messages(dynamic_tree_vote_message::down_vote, down_partners_);

  // I'm sort of done here...
  finish();
}

void
dynamic_tree_vote_actor::recv_expected_up_vote(dynamic_tree_vote_message* msg)
{
  debug_printf(sumi_collective | sumi_vote,
    "Rank %s got expected up vote %d on stage %s from rank=%d on tag=%d - need %s, have %s",
    rank_str().c_str(), msg->vote(), tostr(stage_), msg->dense_sender(), tag_,
    stl_string(down_partners_).c_str(), stl_string(up_votes_recved_).c_str());

  if (up_vote_ready()){
    send_up_votes();
  }
}

void
dynamic_tree_vote_actor::recv_up_vote(dynamic_tree_vote_message* msg)
{
  debug_printf(sumi_collective | sumi_vote,
    "Rank %s got up vote %d on stage %s from rank=%d on tag=%d ",
    rank_str().c_str(), msg->vote(), tostr(stage_), msg->dense_sender(), tag_);

  merge_result(msg);
  int src = msg->dense_sender();
  up_votes_recved_.insert(src);
  recv_expected_up_vote(msg);
}

void
dynamic_tree_vote_actor::recv(dynamic_tree_vote_message* msg)
{
  if (is_failed(msg->dense_sender())){
    debug_printf(sumi_collective | sumi_vote | sumi_collective_sendrecv,
      "Rank %s skipping message from %d:%d on tag=%d because that rank is already dead",
      rank_str().c_str(), msg->dense_sender(), msg->sender(), tag_);
    //ignore this - sometimes messages from dead nodes get caught in transit
    return;
  }

  switch (msg->type())
  {
  case dynamic_tree_vote_message::up_vote:
    recv_up_vote(msg);
    break;
  case dynamic_tree_vote_message::down_vote:
    recv_down_vote(msg);
    break;
#ifdef FEATURE_TAG_SUMI_RESILIENCE
  case dynamic_tree_vote_message::request:
    recv_adoption_request(msg);
    break;
#endif
  default:
    sprockit::abort("invalid message type in dynamic tree vote");
    break;
  }
}

dynamic_tree_vote_collective::dynamic_tree_vote_collective(
  int vote, vote_fxn fxn, int tag,
  ::sstmac::sumi::transport* my_api, const config& cfg) :
  collective(collective::dynamic_tree_vote, my_api, tag, cfg),
  vote_(vote),
  fxn_(fxn)
{
  actors_[dense_me_] = new dynamic_tree_vote_actor(vote, fxn, tag, my_api, cfg);
  refcounts_[cfg_.dom->my_comm_rank()] = actors_.size();
}

void
dynamic_tree_vote_collective::recv(int target, ::sumi::deprecated::collective_work_message* msg)
{
  actor_map::iterator it = actors_.find(target);
  if (it == actors_.end()){
    spkt_throw_printf(sprockit::value_error,
        "vote_collective::recv: invalid virtual destination %d on rank %d for tag=%d ",
        target, my_api_->rank(), tag_);
  }

  dynamic_tree_vote_actor* schauspieler = it->second;
  schauspieler->recv(dynamic_cast<dynamic_tree_vote_message*>(msg));
}

void
dynamic_tree_vote_collective::start()
{
  actor_map::iterator it, end = actors_.end();
  for (it=actors_.begin(); it != end; ++it){
    dynamic_tree_vote_actor* actor = it->second;
    actor->start();
  }
}

}
