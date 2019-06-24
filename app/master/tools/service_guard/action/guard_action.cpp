#include "stdafx.h"
#include "dao/service_node.h"
#include "dao/service_app.h"
#include "dao/service_dead.h"
#include "guard_action.h"

guard_action::guard_action(const char* ip, const char* data)
: ip_(ip)
, data_(data)
{
}

guard_action::~guard_action(void)
{
}

void* guard_action::run(void)
{
	do_run();
	delete this;
	return NULL;
}

bool guard_action::do_run(void)
{
	acl::json json(data_);
	if (!json.finish()) {
		logger_error("invalid data=|%s|", data_.c_str());
		return false;
	}

	acl::json_node* node = json["cmd"];
	if (node == NULL)
		return on_service_list(json); // just for the old json data;

	const char* cmd = node->get_text();
	if (cmd == NULL)
		return on_service_list(json);

	if (strcasecmp(cmd, "service_dead") == 0)
		return on_service_dead(json);
	else
		return on_service_list(json);

}
bool guard_action::on_service_list(acl::json& json)
{
	service_list_res_t res;
	if (deserialize<service_list_res_t>(json, res) == false) {
		logger_error("deserialize error, json=%s",
			json.to_string().c_str());
		return false;
	}

	bool ret = true;

	service_node node(var_redis);
	if (node.save(ip_, res) == false) {
		logger_error("save error, ip=%s", ip_.c_str());
		ret = false;
	}

	service_app app(var_redis);
	if (app.save(ip_, res) == false) {
		logger_error("save app info error, ip=%s", ip_.c_str());
		ret = false;
	}

	return ret;
}

bool guard_action::on_service_dead(acl::json& json)
{
	service_dead_res_t res;
	if (deserialize<service_dead_res_t>(json, res) == false) {
		return false;
	}

	service_dead dead(var_redis);
	if (dead.save(ip_, res) == false) {
		logger_error("save error, ip=%s", ip_.c_str());
		return false;
	}

	return true;
}
