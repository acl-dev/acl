#pragma once
#include "struct.h"
#include "struct.gson.h"

template<typename T>
bool deserialize(acl::json& json, T& o)
{
	if (!json.finish())
	{
		logger_error("json not complete yet!");
		return false;
	}

	std::pair<bool, std::string> r = acl::gson(json.get_root(), o);
	if (r.first == false)
	{
		logger_error("deserialize error=%s, json=[%s]",
			r.second.c_str(), json.to_string().c_str());
		return false;
	}

	return true;
}

template<typename T>
void serialize(T& o, acl::string& buf)
{
	acl::json json;
	acl::json_node& node = acl::gson(json, o);
	buf = node.to_string();
	buf += "\r\n";
}
