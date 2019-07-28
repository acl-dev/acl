#pragma once

struct req_t
{
	acl::string cmd;
};

struct res_t
{
	int status;
	acl::string msg;
};

//////////////////////////////////////////////////////////////////////////////

struct proc_info_t
{
	int  pid;
	long start;

	proc_info_t()
	{
		pid   = -1;
		start = 0;
	}
};

struct serv_info_t
{
	int  status;
	acl::string name;
	int  type;
	long start;
	// Gson@optional
	acl::string owner;
	acl::string path;
	acl::string conf;
	int  proc_max;
	int  proc_prefork;
	int  proc_total;
	int  proc_avail;
	int  throttle_delay;
	int  listen_fd_count;
	// Gson@optional
	acl::string notify_addr;
	// Gson@optional
	acl::string notify_recipients;
	// Gson@optional
	acl::string version;
	// Gson@optional
	std::map<acl::string, acl::string> env;
	// Gson@optional
	std::list<proc_info_t> procs;
	// Gson@optional
	bool check_fds;
	// Gson@optional
	bool check_mem;
	// Gson@optional
	bool check_cpu;
	// Gson@optional
	bool check_io;
	// Gson@optional
	bool check_limits;
	// Gson@optional
	bool check_net;

	serv_info_t()
	{
		status = 0;
		type   = 0;
		start  = 0;

		check_fds    = false;
		check_mem    = false;
		check_cpu    = false;
		check_io     = false;
		check_limits = false;
		check_net    = false;
	}
};

//////////////////////////////////////////////////////////////////////////////

struct list_req_t : req_t
{
};

struct list_res_t : res_t
{
	// Gson@optional
	std::vector<serv_info_t> data;
};

//////////////////////////////////////////////////////////////////////////////

struct stat_req_data_t
{
	acl::string path;
};

struct stat_req_t : req_t
{
	std::vector<stat_req_data_t> data;
};

struct stat_res_t : res_t
{
	std::vector<serv_info_t> data;
};

//////////////////////////////////////////////////////////////////////////////

struct start_req_data_t
{
	acl::string path;
	// Gson@optional
	acl::string ext;
};

struct start_req_t : req_t
{
	// Gson@optional
	long long timeout;
	std::vector<start_req_data_t> data;

	start_req_t(void) : timeout(-1) {}
};

struct start_res_data_t
{
	start_res_data_t(void)
	{
		status = 0;
		proc_count    = 0;
		proc_signaled = 0;
		proc_ok       = 0;
		proc_err      = 0;
	}

	int status;
	int proc_count;
	int proc_signaled;
	int proc_ok;
	int proc_err;

	acl::string name;
	// Gson@optional
	acl::string path;
};

struct start_res_t : res_t
{
	std::vector<start_res_data_t> data;
};

//////////////////////////////////////////////////////////////////////////////

struct restart_req_data_t
{
	acl::string path;
	// Gson@optional
	acl::string ext;
};

struct restart_req_t : req_t
{
	std::vector<restart_req_data_t> data;

};

struct restart_res_data_t
{
	int status;
	acl::string name;
	// Gson@optional
	acl::string path;
};

struct restart_res_t : res_t
{
	std::vector<restart_res_data_t> data;
};

//////////////////////////////////////////////////////////////////////////////

struct stop_req_data_t
{
	acl::string path;
};

struct stop_req_t : req_t
{
	std::vector<stop_req_data_t> data;
};

struct stop_res_data_t
{
	int status;
	acl::string path;
};

struct stop_res_t : res_t
{
	std::vector<stop_res_data_t> data;
};

//////////////////////////////////////////////////////////////////////////////

struct kill_req_data_t
{
	acl::string path;
};

struct kill_req_t : req_t
{
	std::vector<kill_req_data_t> data;
};

struct kill_res_data_t
{
	int status;
	acl::string path;
};

struct kill_res_t : res_t
{
	std::vector<kill_res_data_t> data;
};

//////////////////////////////////////////////////////////////////////////////

struct reload_req_data_t
{
	acl::string path;
};

struct reload_req_t : req_t
{
	// Gson@optional
	long long timeout;
	std::vector<reload_req_data_t> data;

	reload_req_t(void) : timeout(-1) {}
};

struct reload_res_data_t
{
	reload_res_data_t(void)
	{
		status        = 0;
		proc_count    = 0;
		proc_signaled = 0;
		proc_ok       = 0;
		proc_err      = 0;
	}

	int status;
	int proc_count;
	int proc_signaled;
	int proc_ok;
	int proc_err;

	acl::string path;
};

struct reload_res_t : res_t
{
	std::vector<reload_res_data_t> data;
};

//////////////////////////////////////////////////////////////////////////////

struct master_config_req_t : req_t
{
};

struct master_config_res_t : res_t
{
	std::map<acl::string, acl::string> data;
};
