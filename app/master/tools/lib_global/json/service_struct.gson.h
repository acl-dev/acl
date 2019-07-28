namespace acl
{
    //service_base
    acl::string gson(const service_base &$obj);
    acl::json_node& gson(acl::json &$json, const service_base &$obj);
    acl::json_node& gson(acl::json &$json, const service_base *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_base &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_base *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, service_base &$obj);

    //service_dead_res_t
    acl::string gson(const service_dead_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const service_dead_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const service_dead_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_dead_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_dead_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, service_dead_res_t &$obj);

    //service_info_t
    acl::string gson(const service_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const service_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const service_info_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_info_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_info_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, service_info_t &$obj);

    //service_list_res_t
    acl::string gson(const service_list_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const service_list_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const service_list_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_list_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, service_list_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, service_list_res_t &$obj);

}///end of acl.
