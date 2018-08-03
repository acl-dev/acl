namespace acl
{
    //files_outdate
    acl::string gson(const files_outdate &$obj);
    acl::json_node& gson(acl::json &$json, const files_outdate &$obj);
    acl::json_node& gson(acl::json &$json, const files_outdate *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, files_outdate &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, files_outdate *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, files_outdate &$obj);

}///end of acl.
