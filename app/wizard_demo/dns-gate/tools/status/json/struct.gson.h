namespace acl
{
    //filter_mac_t
    acl::string gson(const filter_mac_t &$obj);
    acl::json_node& gson(acl::json &$json, const filter_mac_t &$obj);
    acl::json_node& gson(acl::json &$json, const filter_mac_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, filter_mac_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, filter_mac_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, filter_mac_t &$obj);

    //host_info_t
    acl::string gson(const host_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const host_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const host_info_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, host_info_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, host_info_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, host_info_t &$obj);

    //limit_speed_host_management_t
    acl::string gson(const limit_speed_host_management_t &$obj);
    acl::json_node& gson(acl::json &$json, const limit_speed_host_management_t &$obj);
    acl::json_node& gson(acl::json &$json, const limit_speed_host_management_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_host_management_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_host_management_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, limit_speed_host_management_t &$obj);

    //limit_speed_para_t
    acl::string gson(const limit_speed_para_t &$obj);
    acl::json_node& gson(acl::json &$json, const limit_speed_para_t &$obj);
    acl::json_node& gson(acl::json &$json, const limit_speed_para_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_para_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_para_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, limit_speed_para_t &$obj);

    //limit_speed_req_t
    acl::string gson(const limit_speed_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const limit_speed_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const limit_speed_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, limit_speed_req_t &$obj);

    //login_req_t
    acl::string gson(const login_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const login_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const login_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, login_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, login_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, login_req_t &$obj);

    //login_res_t
    acl::string gson(const login_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const login_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const login_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, login_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, login_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, login_res_t &$obj);

    //login_t
    acl::string gson(const login_t &$obj);
    acl::json_node& gson(acl::json &$json, const login_t &$obj);
    acl::json_node& gson(acl::json &$json, const login_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, login_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, login_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, login_t &$obj);

    //req_global_config_t
    acl::string gson(const req_global_config_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_global_config_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_global_config_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_global_config_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_global_config_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, req_global_config_t &$obj);

    //req_host_management_t
    acl::string gson(const req_host_management_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_host_management_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_host_management_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_host_management_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_host_management_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, req_host_management_t &$obj);

    //req_para_t
    acl::string gson(const req_para_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_para_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_para_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_para_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_para_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, req_para_t &$obj);

    //req_wireless_t
    acl::string gson(const req_wireless_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_wireless_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_wireless_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_wireless_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_wireless_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, req_wireless_t &$obj);

    //request_t
    acl::string gson(const request_t &$obj);
    acl::json_node& gson(acl::json &$json, const request_t &$obj);
    acl::json_node& gson(acl::json &$json, const request_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, request_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, request_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, request_t &$obj);

    //res_count_t
    acl::string gson(const res_count_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_count_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_count_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_count_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_count_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, res_count_t &$obj);

    //res_host_management_t
    acl::string gson(const res_host_management_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_host_management_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_host_management_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_host_management_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_host_management_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, res_host_management_t &$obj);

    //res_wireless_t
    acl::string gson(const res_wireless_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_wireless_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_wireless_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_wireless_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_wireless_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, res_wireless_t &$obj);

    //response_t
    acl::string gson(const response_t &$obj);
    acl::json_node& gson(acl::json &$json, const response_t &$obj);
    acl::json_node& gson(acl::json &$json, const response_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, response_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, response_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, response_t &$obj);

    //stat_list_t
    acl::string gson(const stat_list_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_list_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_list_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_list_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_list_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stat_list_t &$obj);

}///end of acl.
