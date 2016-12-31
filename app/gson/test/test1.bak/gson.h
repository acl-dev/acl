namespace acl
{
    //people
    acl::string gson(const people &$obj);
    acl::json_node& gson(acl::json &$json, const people &$obj);
    acl::json_node& gson(acl::json &$json, const people *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, people &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, people *$obj);
    //user
    acl::string gson(const user &$obj);
    acl::json_node& gson(acl::json &$json, const user &$obj);
    acl::json_node& gson(acl::json &$json, const user *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, user &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, user *$obj);
}///end of acl.
