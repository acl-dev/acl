namespace acl
{
    //kill_req_data_t
    acl::string gson(const kill_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_req_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, kill_req_data_t &$obj);

    //kill_req_t
    acl::string gson(const kill_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, kill_req_t &$obj);

    //kill_res_data_t
    acl::string gson(const kill_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_res_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, kill_res_data_t &$obj);

    //kill_res_t
    acl::string gson(const kill_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const kill_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, kill_res_t &$obj);

    //list_req_t
    acl::string gson(const list_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, list_req_t &$obj);

    //list_res_t
    acl::string gson(const list_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const list_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, list_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, list_res_t &$obj);

    //master_config_req_t
    acl::string gson(const master_config_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const master_config_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const master_config_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, master_config_req_t &$obj);

    //master_config_res_t
    acl::string gson(const master_config_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const master_config_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const master_config_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, master_config_res_t &$obj);

    //proc_info_t
    acl::string gson(const proc_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const proc_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const proc_info_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, proc_info_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, proc_info_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, proc_info_t &$obj);

    //reload_req_data_t
    acl::string gson(const reload_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_req_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, reload_req_data_t &$obj);

    //reload_req_t
    acl::string gson(const reload_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, reload_req_t &$obj);

    //reload_res_data_t
    acl::string gson(const reload_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_res_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, reload_res_data_t &$obj);

    //reload_res_t
    acl::string gson(const reload_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const reload_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, reload_res_t &$obj);

    //req_t
    acl::string gson(const req_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_t &$obj);
    acl::json_node& gson(acl::json &$json, const req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, req_t &$obj);

    //res_t
    acl::string gson(const res_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_t &$obj);
    acl::json_node& gson(acl::json &$json, const res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, res_t &$obj);

    //restart_req_data_t
    acl::string gson(const restart_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_req_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, restart_req_data_t &$obj);

    //restart_req_t
    acl::string gson(const restart_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, restart_req_t &$obj);

    //restart_res_data_t
    acl::string gson(const restart_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_res_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, restart_res_data_t &$obj);

    //restart_res_t
    acl::string gson(const restart_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const restart_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, restart_res_t &$obj);

    //serv_info_t
    acl::string gson(const serv_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const serv_info_t &$obj);
    acl::json_node& gson(acl::json &$json, const serv_info_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, serv_info_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, serv_info_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, serv_info_t &$obj);

    //start_req_data_t
    acl::string gson(const start_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_req_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, start_req_data_t &$obj);

    //start_req_t
    acl::string gson(const start_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, start_req_t &$obj);

    //start_res_data_t
    acl::string gson(const start_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_res_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, start_res_data_t &$obj);

    //start_res_t
    acl::string gson(const start_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const start_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, start_res_t &$obj);

    //stat_req_data_t
    acl::string gson(const stat_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_req_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stat_req_data_t &$obj);

    //stat_req_t
    acl::string gson(const stat_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stat_req_t &$obj);

    //stat_res_t
    acl::string gson(const stat_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stat_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stat_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stat_res_t &$obj);

    //stop_req_data_t
    acl::string gson(const stop_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_req_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_req_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stop_req_data_t &$obj);

    //stop_req_t
    acl::string gson(const stop_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_req_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_req_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stop_req_t &$obj);

    //stop_res_data_t
    acl::string gson(const stop_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_res_data_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_res_data_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_data_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_data_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stop_res_data_t &$obj);

    //stop_res_t
    acl::string gson(const stop_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_res_t &$obj);
    acl::json_node& gson(acl::json &$json, const stop_res_t *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_t &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_t *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, stop_res_t &$obj);

}///end of acl.
