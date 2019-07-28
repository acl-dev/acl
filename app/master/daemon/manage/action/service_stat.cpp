#include "stdafx.h"
#include "master/master_api.h"
#include "manage/http_client.h"
#include "service_stat.h"

#define	CMD	"stat"

bool service_stat::stat_one(const char* path, serv_info_t& info)
{
	ACL_MASTER_SERV *serv = acl_master_lookup(path);

	if (serv == NULL) {
		info.status = 404;
		info.conf   = path;
		return false;
	}

	info.status          = 200;
	info.name            = serv->name;
	info.type            = serv->type;
	info.path            = serv->path;
	info.conf            = serv->conf;
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
		info.owner = serv->owner;
	if (serv->notify_addr && *serv->notify_addr)
		info.notify_addr = serv->notify_addr;
	if (serv->notify_recipients && *serv->notify_recipients)
		info.notify_recipients = serv->notify_recipients;

	info.version = serv->version;

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

	info.status = 200;
	return true;
}

bool service_stat::run(acl::json& json)
{
	stat_req_t req;
	stat_res_t res;

	if (deserialize<stat_req_t>(json, req) == false) {
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<stat_res_t>(res.status, CMD, res);
		return false;
	}

	return handle(req, res);
}

bool service_stat::handle(const stat_req_t& req, stat_res_t& res)
{
	size_t n = 0;

	for (std::vector<stat_req_data_t>::const_iterator
		cit = req.data.begin(); cit != req.data.end(); ++cit) {

		serv_info_t info;
		if (stat_one((*cit).path.c_str(), info))
			n++;
		res.data.push_back(info);
	}

	if (n == req.data.size()) {
		res.status = 200;
		res.msg    = "ok";
	} else {
		res.status = 500;
		res.msg    = "error";
		logger_error("not all service have been started!, n=%d, %d",
			(int) n, (int) req.data.size());
	}

	client_.reply<stat_res_t>(res.status, CMD, res, false);
	client_.on_finish();

	return true;
}
