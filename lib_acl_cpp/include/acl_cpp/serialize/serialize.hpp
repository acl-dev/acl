#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/json.hpp"
#include <string>

namespace acl
{

template<typename T>
bool deserialize(json& json, T& o, string* err = NULL)
{
	if (!json.finish())
	{
		if (err)
			err->append("json not complete yet!");
		return false;
	}

	std::pair<bool, std::string> r = acl::gson(json.get_root(), o);
	if (r.first == false)
	{
		if (err)
			err->format_append("deserialize error=%s, json=[%s]",
				r.second.c_str(), json.to_string().c_str());
		return false;
	}

	return true;
}

template<typename T>
void serialize(T& o, string& buf)
{
	json json;
	json_node& node = json(json, o);
	(void) node.to_string(&buf);
	buf += "\r\n";
}

} // namespace acl
