#include "stdafx.h"
#include "tools/lib_global/json/service_struct.h"
#include "tools/lib_global/json/service_struct.gson.h"
#include "daemon/json/serialize.h"
#include "tools.h"
#include "guard_report.h"
#include "http_request.h"
#include "service_list.h"

static int get_fds(const std::list<proc_info_t>& procs)
{
	int n = 0;
	for (std::list<proc_info_t>::const_iterator cit = procs.begin();
		cit != procs.end(); ++cit)
	{
		int ret = tools::get_fds((*cit).pid);
		if (ret > 0)
			n += ret;
	}

	return n;
}

static long get_mem(const std::list<proc_info_t>& procs)
{
	long n = 0;
	for (std::list<proc_info_t>::const_iterator cit = procs.begin();
		cit != procs.end(); ++cit)
	{
		long ret = tools::get_mem((*cit).pid);
		if (ret > 0)
			n += ret;
	}

	return n;
}

service_list::service_list(const char* master_ctld, const char* guard_manager,
	acl::tcp_ipc& ipc, int conn_timeout, int rw_timeout)
: master_ctld_(master_ctld)
, guard_manager_(guard_manager)
, ipc_(ipc)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
}

bool service_list::run(void)
{
	list_req_t req;
	req.cmd = "list";

	list_res_t res;
	if (!http_request_run<list_req_t, list_res_t>(master_ctld_, req, res))
		return false;

	service_list_res_t list_res;
	list_res.status = 200;
	list_res.cmd    = "service_list";

	for (std::vector<serv_info_t>::const_iterator cit = res.data.begin();
		cit != res.data.end(); ++cit)
	{
		service_info_t info;
		info.status = (*cit).status;
		info.start  = (*cit).start;
		info.name   = (*cit).name;
		info.conf   = (*cit).conf;
		info.path   = (*cit).path;
		if ((*cit).version.empty())
			info.version = "none";
		else if ((*cit).version.equal("-v", false))
			tools::get_version((*cit).path, info.version);
		else
			info.version = (*cit).version;

		if ((*cit).check_fds)
			info.fds = get_fds((*cit).procs);
		if ((*cit).check_mem)
			info.mem = get_mem((*cit).procs);

		list_res.data.push_back(info);
	}

	acl::string body;
	serialize<service_list_res_t>(list_res, body);
	//logger("|%s|, len=%d", body.c_str(), (int) body.size());

	guard_report report(guard_manager_, ipc_, conn_timeout_, rw_timeout_);
	return report.report(body);
}
