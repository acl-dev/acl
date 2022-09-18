#pragma once

struct req_global_config_t {
	acl::string name;

	req_global_config_t(void) {
		name = "page_size";
	}
};

struct req_para_t {
	int start;
	int end;

	req_para_t(void) {
		start = 0;
		end = 1;
	}
};

struct req_host_management_t {
	acl::string table;
	req_para_t para;

	req_host_management_t(void) {
		table = "host_info";
	}
};

struct req_wireless_t {
	acl::string table;

	req_wireless_t(void) {
		table = "sta_list";
	}
};

struct request_t {
	acl::string method;
	req_global_config_t global_config;
	req_host_management_t host_management;
	req_wireless_t wireless;

	request_t(void) {
		method = "get";
	}
};

//////////////////////////////////////////////////////////////////////////////

struct stat_list_t {
	acl::string name;
	acl::string mac;
	acl::string ip;
	acl::string tx_rate;
	acl::string rx_rate;
};

struct res_wireless_t {
	std::map<acl::string, stat_list_t> sta_list;
};

struct res_count_t {
	int host_info;
};

struct host_info_t {
	acl::string host_save;
	acl::string ip;
	acl::string is_cur_host;
	acl::string hostname;
	acl::string mac;
	acl::string type;
	acl::string ssid;
	acl::string up_limit;
	acl::string down_limit;
	acl::string limit;
	acl::string dev_state;
};

struct res_host_management_t {
	std::map<acl::string, host_info_t> host_info;
	res_count_t count;
};

struct response_t {
	res_host_management_t host_management;
	res_wireless_t wireless;

	int error_code;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

struct login_t {
	acl::string username;
	acl::string password;
};

struct login_req_t {
	acl::string method;
	login_t login;
};

struct login_res_t {
	int error_code;
	acl::string stok;
};

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

struct filter_mac_t {
	acl::string mac;
};

struct limit_speed_para_t {
	acl::string host_save;
	acl::string type;
	acl::string origin_hostname;
	acl::string hostname;
	acl::string ssid;
	acl::string ip;
	acl::string mac;
	acl::string limit;
	acl::string up_limit;
	acl::string down_limit;
	acl::string time_obj;
	acl::string name;
	acl::string time_mode;
	acl::string is_cur_host;

	limit_speed_para_t(void) {
		host_save = "on";
		type = "wireless";
		origin_hostname = "---";
		hostname = "---";
		ssid = "TP_LINK_668D";
		limit = "1";
		time_obj = "any";
		time_mode = "any";
		is_cur_host = "false";
	}
};

struct limit_speed_host_management_t {
	acl::string table;
	std::vector<filter_mac_t> filter;
	limit_speed_para_t para;

	limit_speed_host_management_t(void) {
		table = "host_info";
	}
};

struct limit_speed_req_t {
	acl::string method;
	limit_speed_host_management_t host_management;

	limit_speed_req_t(void) {
		method = "set";
	}
};
