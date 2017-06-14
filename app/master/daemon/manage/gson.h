namespace acl
{
    //list_req_t
    acl::string gson(const list_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_req_t *$obj);
    //list_res_t
    acl::string gson(const list_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_res_t *$obj);
    //req_t
    acl::string gson(const req_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_t *$obj);
    //res_t
    acl::string gson(const res_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_t *$obj);
    //start_req_t
    acl::string gson(const start_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_t *$obj);
    //start_res_t
    acl::string gson(const start_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_t *$obj);
    //stat_req_t
    acl::string gson(const stat_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_t *$obj);
    //stat_res_t
    acl::string gson(const stat_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_res_t *$obj);
    //stop_req_t
    acl::string gson(const stop_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_t *$obj);
    //stop_res_t
    acl::string gson(const stop_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_t *$obj);
}///end of acl.
