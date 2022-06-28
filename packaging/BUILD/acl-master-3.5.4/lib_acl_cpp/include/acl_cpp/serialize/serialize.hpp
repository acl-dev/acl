#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/json.hpp"
#include <string>

namespace acl
{

template<typename T>
bool deserialize(json& j, T& o, string* err = NULL)
{
	if (!j.finish())
	{
		if (err)
			err->append("json not complete yet!");
		return false;
	}

	std::pair<bool, std::string> r = gson(j.get_root(), o);
	if (r.first == false)
	{
		if (err)
			err->format_append("deserialize error=%s, json=[%s]",
				r.second.c_str(), j.to_string().c_str());
		return false;
	}

	return true;
}

template<typename T>
void serialize(T& o, string& buf)
{
	json j;
	json_node& n = gson(j, o);
	(void) n.to_string(&buf);
	buf += "\r\n";
}

} // namespace acl
