#include <sstmac/software/launch/job_launcher.h>
#include <sstmac/hardware/interconnect/interconnect.h>
#include <sstmac/hardware/node/node.h>
#include <sstmac/software/launch/launch_event.h>
#include <sstmac/software/launch/job_launch_event.h>
#include <sstmac/software/process/app_manager.h>
#include <sstmac/common/runtime.h>
#include <sprockit/util.h>

ImplementFactory(sstmac::sw::job_launcher)

namespace sstmac {
namespace sw {

SpktRegister("default", job_launcher, default_job_launcher);

void
job_launcher::handle(event *ev)
{
  job_launch_event* lev = safe_cast(job_launch_event, ev);
  handle_new_launch_request(lev->appnum(), lev->appman());
}

void
job_launcher::set_interconnect(hw::interconnect *ic)
{
  interconnect_ = ic;
  int num_nodes = ic->num_nodes();
  for (int i=0; i < num_nodes; ++i){
    available_.insert(i);
  }
}

app_manager*
job_launcher::task_mapper(app_id aid) const
{
  auto iter = apps_launched_.find(aid);
  if (iter == apps_launched_.end()){
    spkt_throw_printf(sprockit::value_error,
                      "cannot find application launched with id %d",
                      int(aid));
  }
  return iter->second;
}

node_id
job_launcher::node_for_task(app_id aid, task_id tid) const
{
  auto iter = apps_launched_.find(aid);
  if (iter == apps_launched_.end()){
    spkt_throw(sprockit::value_error,
               "cannot find launched application %d",
               int(aid));
  }
  app_manager* appman = iter->second;
  return appman->node_assignment(int(tid));
}

void
default_job_launcher::handle_new_launch_request(int appnum, app_manager* appman)
{
  ordered_node_set allocation;
  appman->request_allocation(available_, allocation);
  for (const node_id& nid : allocation){
    if (available_.find(nid) == available_.end()){
      spkt_throw_printf(sprockit::value_error,
                        "allocation requested node %d, but it's not available",
                        int(nid));
    }
    available_.erase(nid);
  }
  appman->index_allocation(allocation);
  apps_launched_[appnum] = appman;

  launch_info* linfo = appman->launch_info();
  sstmac::sw::app_id aid(appnum);
  for (int i=0; i < appman->nproc(); ++i) {
    node_id dst_nid = appman->node_assignment(i);

    hw::node* dst_node = interconnect_->node_at(dst_nid);
    if (!dst_node) {
      // mpiparallel, this node belongs to someone else
      continue;
    }

    sw::launch_event* lmsg = new launch_event(linfo, sw::launch_event::ARRIVE, task_id(i));
    dst_node->handle(lmsg);
  }


}

}
}
