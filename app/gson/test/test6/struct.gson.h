namespace acl
{
    //company
    acl::string gson(const company &$obj);
    acl::json_node& gson(acl::json &$json, const company &$obj);
    acl::json_node& gson(acl::json &$json, const company *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, company &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, company *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, company &$obj);

    //group
    acl::string gson(const group &$obj);
    acl::json_node& gson(acl::json &$json, const group &$obj);
    acl::json_node& gson(acl::json &$json, const group *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, group &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, group *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, group &$obj);

    //user
    acl::string gson(const user &$obj);
    acl::json_node& gson(acl::json &$json, const user &$obj);
    acl::json_node& gson(acl::json &$json, const user *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, user &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, user *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, user &$obj);

}///end of acl.
