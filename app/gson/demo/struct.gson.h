namespace acl
{
    //base
    acl::string gson(const base &$obj);
    acl::json_node& gson(acl::json &$json, const base &$obj);
    acl::json_node& gson(acl::json &$json, const base *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, base &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, base *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, base &$obj);

    //hello::world
    acl::string gson(const hello::world &$obj);
    acl::json_node& gson(acl::json &$json, const hello::world &$obj);
    acl::json_node& gson(acl::json &$json, const hello::world *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, hello::world &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, hello::world *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, hello::world &$obj);

    //list1
    acl::string gson(const list1 &$obj);
    acl::json_node& gson(acl::json &$json, const list1 &$obj);
    acl::json_node& gson(acl::json &$json, const list1 *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list1 &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list1 *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, list1 &$obj);

}///end of acl.
