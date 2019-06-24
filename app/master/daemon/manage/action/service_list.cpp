#include "stdafx.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "service_list.h"

#define CMD	"list"

void service_list::add_one(list_res_t& res, const ACL_MASTER_SERV* serv)
{
	serv_info_t info;

	info.status          = 200;
	info.name            = serv->name;
	info.type            = serv->type;
	info.start           = serv->start;
	info.path            = serv->path;
	info.conf            = serv->conf;
	info.version         = serv->version;
	info.proc_max        = serv->max_proc;
	info.proc_prefork    = serv->prefork_proc;
	info.proc_total      = serv->total_proc;
	info.proc_avail      = serv->avail_proc;
	info.throttle_delay  = serv->throttle_delay;
	info.listen_fd_count = serv->listen_fd_count;
	info.check_fds       = serv->check_fds ? true : false;
	info.check_mem       = serv->check_mem ? true : false;
	info.check_cpu       = serv->check_cpu ? true : false;
	info.check_io        = serv->check_io ? true : false;
	info.check_net       = serv->check_net ? true : false;
	info.check_limits    = serv->check_limits ? true : false;

	if (serv->owner && *serv->owner)
		info.owner             = serv->owner;
	if (serv->notify_addr && *serv->notify_addr)
		info.notify_addr       = serv->notify_addr;
	if (serv->notify_recipients && *serv->notify_recipients)
		info.notify_recipients = serv->notify_recipients;

	ACL_ITER iter;
	acl_foreach(iter, serv->children_env) {
		ACL_MASTER_NV* v = (ACL_MASTER_NV *) iter.data;
		info.env[v->name] = v->value;
	}

	ACL_RING_ITER iter2;
	acl_ring_foreach(iter2, &serv->children) {
		ACL_MASTER_PROC* proc = (ACL_MASTER_PROC *)
			acl_ring_to_appl(iter2.ptr, ACL_MASTER_PROC, me);
		proc_info_t pi;
		pi.pid = proc->pid;
		pi.start = proc->start;
		info.procs.push_back(pi);
	}

	res.data.push_back(info);
}

bool service_list::run(acl::json& json)
{
	list_req_t req;
	list_res_t res;

	if (deserialize<list_req_t>(json, req) == false) {
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<list_res_t>(res.status, CMD, res);
		return false;
	}

	return handle(req, res);
}

bool service_list::handle(const list_req_t&, list_res_t& res)
{
	ACL_MASTER_SERV *serv;

	for (serv = acl_var_master_head; serv != 0; serv = serv->next)
		add_one(res, serv);

	res.status = 200;
	res.msg    = "ok";

	client_.reply<list_res_t>(res.status, CMD, res, false);
	client_.on_finish();

	return true;
}
