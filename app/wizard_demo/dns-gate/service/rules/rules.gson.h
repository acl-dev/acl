namespace acl
{
    //filter_rule
    acl::string gson(const filter_rule &$obj);
    acl::json_node& gson(acl::json &$json, const filter_rule &$obj);
    acl::json_node& gson(acl::json &$json, const filter_rule *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rule &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rule *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, filter_rule &$obj);

    //filter_rules
    acl::string gson(const filter_rules &$obj);
    acl::json_node& gson(acl::json &$json, const filter_rules &$obj);
    acl::json_node& gson(acl::json &$json, const filter_rules *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rules &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rules *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, filter_rules &$obj);

    //time_enable
    acl::string gson(const time_enable &$obj);
    acl::json_node& gson(acl::json &$json, const time_enable &$obj);
    acl::json_node& gson(acl::json &$json, const time_enable *$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, time_enable &$obj);
    std::pair<bool,std::string> gson(acl::json_node &$node, time_enable *$obj);
    std::pair<bool,std::string> gson(const acl::string &str, time_enable &$obj);

}///end of acl.
