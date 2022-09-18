#include "stdafx.h"
#include "struct.h"
#include "struct.gson.h"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const filter_mac_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.mac))
            $node.add_null("mac");
        else
            $node.add_text("mac", acl::get_value($obj.mac));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const filter_mac_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const filter_mac_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, filter_mac_t &$obj)
    {
        acl::json_node *mac = $node["mac"];
        std::pair<bool, std::string> $result;

        if(!mac ||!($result = gson(*mac, &$obj.mac), $result.first))
            return std::make_pair(false, "required [filter_mac_t.mac] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, filter_mac_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, filter_mac_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const host_info_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.host_save))
            $node.add_null("host_save");
        else
            $node.add_text("host_save", acl::get_value($obj.host_save));

        if (check_nullptr($obj.ip))
            $node.add_null("ip");
        else
            $node.add_text("ip", acl::get_value($obj.ip));

        if (check_nullptr($obj.is_cur_host))
            $node.add_null("is_cur_host");
        else
            $node.add_text("is_cur_host", acl::get_value($obj.is_cur_host));

        if (check_nullptr($obj.hostname))
            $node.add_null("hostname");
        else
            $node.add_text("hostname", acl::get_value($obj.hostname));

        if (check_nullptr($obj.mac))
            $node.add_null("mac");
        else
            $node.add_text("mac", acl::get_value($obj.mac));

        if (check_nullptr($obj.type))
            $node.add_null("type");
        else
            $node.add_text("type", acl::get_value($obj.type));

        if (check_nullptr($obj.ssid))
            $node.add_null("ssid");
        else
            $node.add_text("ssid", acl::get_value($obj.ssid));

        if (check_nullptr($obj.up_limit))
            $node.add_null("up_limit");
        else
            $node.add_text("up_limit", acl::get_value($obj.up_limit));

        if (check_nullptr($obj.down_limit))
            $node.add_null("down_limit");
        else
            $node.add_text("down_limit", acl::get_value($obj.down_limit));

        if (check_nullptr($obj.limit))
            $node.add_null("limit");
        else
            $node.add_text("limit", acl::get_value($obj.limit));

        if (check_nullptr($obj.dev_state))
            $node.add_null("dev_state");
        else
            $node.add_text("dev_state", acl::get_value($obj.dev_state));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const host_info_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const host_info_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, host_info_t &$obj)
    {
        acl::json_node *host_save = $node["host_save"];
        acl::json_node *ip = $node["ip"];
        acl::json_node *is_cur_host = $node["is_cur_host"];
        acl::json_node *hostname = $node["hostname"];
        acl::json_node *mac = $node["mac"];
        acl::json_node *type = $node["type"];
        acl::json_node *ssid = $node["ssid"];
        acl::json_node *up_limit = $node["up_limit"];
        acl::json_node *down_limit = $node["down_limit"];
        acl::json_node *limit = $node["limit"];
        acl::json_node *dev_state = $node["dev_state"];
        std::pair<bool, std::string> $result;

        if(!host_save ||!($result = gson(*host_save, &$obj.host_save), $result.first))
            return std::make_pair(false, "required [host_info_t.host_save] failed:{"+$result.second+"}");
     
        if(!ip ||!($result = gson(*ip, &$obj.ip), $result.first))
            return std::make_pair(false, "required [host_info_t.ip] failed:{"+$result.second+"}");
     
        if(!is_cur_host ||!($result = gson(*is_cur_host, &$obj.is_cur_host), $result.first))
            return std::make_pair(false, "required [host_info_t.is_cur_host] failed:{"+$result.second+"}");
     
        if(!hostname ||!($result = gson(*hostname, &$obj.hostname), $result.first))
            return std::make_pair(false, "required [host_info_t.hostname] failed:{"+$result.second+"}");
     
        if(!mac ||!($result = gson(*mac, &$obj.mac), $result.first))
            return std::make_pair(false, "required [host_info_t.mac] failed:{"+$result.second+"}");
     
        if(!type ||!($result = gson(*type, &$obj.type), $result.first))
            return std::make_pair(false, "required [host_info_t.type] failed:{"+$result.second+"}");
     
        if(!ssid ||!($result = gson(*ssid, &$obj.ssid), $result.first))
            return std::make_pair(false, "required [host_info_t.ssid] failed:{"+$result.second+"}");
     
        if(!up_limit ||!($result = gson(*up_limit, &$obj.up_limit), $result.first))
            return std::make_pair(false, "required [host_info_t.up_limit] failed:{"+$result.second+"}");
     
        if(!down_limit ||!($result = gson(*down_limit, &$obj.down_limit), $result.first))
            return std::make_pair(false, "required [host_info_t.down_limit] failed:{"+$result.second+"}");
     
        if(!limit ||!($result = gson(*limit, &$obj.limit), $result.first))
            return std::make_pair(false, "required [host_info_t.limit] failed:{"+$result.second+"}");
     
        if(!dev_state ||!($result = gson(*dev_state, &$obj.dev_state), $result.first))
            return std::make_pair(false, "required [host_info_t.dev_state] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, host_info_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, host_info_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const limit_speed_host_management_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.table))
            $node.add_null("table");
        else
            $node.add_text("table", acl::get_value($obj.table));

        if (check_nullptr($obj.filter))
            $node.add_null("filter");
        else
            $node.add_child("filter", acl::gson($json, $obj.filter));

        if (check_nullptr($obj.para))
            $node.add_null("para");
        else
            $node.add_child("para", acl::gson($json, $obj.para));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const limit_speed_host_management_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const limit_speed_host_management_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_host_management_t &$obj)
    {
        acl::json_node *table = $node["table"];
        acl::json_node *filter = $node["filter"];
        acl::json_node *para = $node["para"];
        std::pair<bool, std::string> $result;

        if(!table ||!($result = gson(*table, &$obj.table), $result.first))
            return std::make_pair(false, "required [limit_speed_host_management_t.table] failed:{"+$result.second+"}");
     
        if(!filter ||!filter->get_obj()||!($result = gson(*filter->get_obj(), &$obj.filter), $result.first))
            return std::make_pair(false, "required [limit_speed_host_management_t.filter] failed:{"+$result.second+"}");
     
        if(!para ||!para->get_obj()||!($result = gson(*para->get_obj(), &$obj.para), $result.first))
            return std::make_pair(false, "required [limit_speed_host_management_t.para] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_host_management_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, limit_speed_host_management_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const limit_speed_para_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.host_save))
            $node.add_null("host_save");
        else
            $node.add_text("host_save", acl::get_value($obj.host_save));

        if (check_nullptr($obj.type))
            $node.add_null("type");
        else
            $node.add_text("type", acl::get_value($obj.type));

        if (check_nullptr($obj.origin_hostname))
            $node.add_null("origin_hostname");
        else
            $node.add_text("origin_hostname", acl::get_value($obj.origin_hostname));

        if (check_nullptr($obj.hostname))
            $node.add_null("hostname");
        else
            $node.add_text("hostname", acl::get_value($obj.hostname));

        if (check_nullptr($obj.ssid))
            $node.add_null("ssid");
        else
            $node.add_text("ssid", acl::get_value($obj.ssid));

        if (check_nullptr($obj.ip))
            $node.add_null("ip");
        else
            $node.add_text("ip", acl::get_value($obj.ip));

        if (check_nullptr($obj.mac))
            $node.add_null("mac");
        else
            $node.add_text("mac", acl::get_value($obj.mac));

        if (check_nullptr($obj.limit))
            $node.add_null("limit");
        else
            $node.add_text("limit", acl::get_value($obj.limit));

        if (check_nullptr($obj.up_limit))
            $node.add_null("up_limit");
        else
            $node.add_text("up_limit", acl::get_value($obj.up_limit));

        if (check_nullptr($obj.down_limit))
            $node.add_null("down_limit");
        else
            $node.add_text("down_limit", acl::get_value($obj.down_limit));

        if (check_nullptr($obj.time_obj))
            $node.add_null("time_obj");
        else
            $node.add_text("time_obj", acl::get_value($obj.time_obj));

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.time_mode))
            $node.add_null("time_mode");
        else
            $node.add_text("time_mode", acl::get_value($obj.time_mode));

        if (check_nullptr($obj.is_cur_host))
            $node.add_null("is_cur_host");
        else
            $node.add_text("is_cur_host", acl::get_value($obj.is_cur_host));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const limit_speed_para_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const limit_speed_para_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_para_t &$obj)
    {
        acl::json_node *host_save = $node["host_save"];
        acl::json_node *type = $node["type"];
        acl::json_node *origin_hostname = $node["origin_hostname"];
        acl::json_node *hostname = $node["hostname"];
        acl::json_node *ssid = $node["ssid"];
        acl::json_node *ip = $node["ip"];
        acl::json_node *mac = $node["mac"];
        acl::json_node *limit = $node["limit"];
        acl::json_node *up_limit = $node["up_limit"];
        acl::json_node *down_limit = $node["down_limit"];
        acl::json_node *time_obj = $node["time_obj"];
        acl::json_node *name = $node["name"];
        acl::json_node *time_mode = $node["time_mode"];
        acl::json_node *is_cur_host = $node["is_cur_host"];
        std::pair<bool, std::string> $result;

        if(!host_save ||!($result = gson(*host_save, &$obj.host_save), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.host_save] failed:{"+$result.second+"}");
     
        if(!type ||!($result = gson(*type, &$obj.type), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.type] failed:{"+$result.second+"}");
     
        if(!origin_hostname ||!($result = gson(*origin_hostname, &$obj.origin_hostname), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.origin_hostname] failed:{"+$result.second+"}");
     
        if(!hostname ||!($result = gson(*hostname, &$obj.hostname), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.hostname] failed:{"+$result.second+"}");
     
        if(!ssid ||!($result = gson(*ssid, &$obj.ssid), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.ssid] failed:{"+$result.second+"}");
     
        if(!ip ||!($result = gson(*ip, &$obj.ip), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.ip] failed:{"+$result.second+"}");
     
        if(!mac ||!($result = gson(*mac, &$obj.mac), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.mac] failed:{"+$result.second+"}");
     
        if(!limit ||!($result = gson(*limit, &$obj.limit), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.limit] failed:{"+$result.second+"}");
     
        if(!up_limit ||!($result = gson(*up_limit, &$obj.up_limit), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.up_limit] failed:{"+$result.second+"}");
     
        if(!down_limit ||!($result = gson(*down_limit, &$obj.down_limit), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.down_limit] failed:{"+$result.second+"}");
     
        if(!time_obj ||!($result = gson(*time_obj, &$obj.time_obj), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.time_obj] failed:{"+$result.second+"}");
     
        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.name] failed:{"+$result.second+"}");
     
        if(!time_mode ||!($result = gson(*time_mode, &$obj.time_mode), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.time_mode] failed:{"+$result.second+"}");
     
        if(!is_cur_host ||!($result = gson(*is_cur_host, &$obj.is_cur_host), $result.first))
            return std::make_pair(false, "required [limit_speed_para_t.is_cur_host] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_para_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, limit_speed_para_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const limit_speed_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.method))
            $node.add_null("method");
        else
            $node.add_text("method", acl::get_value($obj.method));

        if (check_nullptr($obj.host_management))
            $node.add_null("host_management");
        else
            $node.add_child("host_management", acl::gson($json, $obj.host_management));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const limit_speed_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const limit_speed_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_req_t &$obj)
    {
        acl::json_node *method = $node["method"];
        acl::json_node *host_management = $node["host_management"];
        std::pair<bool, std::string> $result;

        if(!method ||!($result = gson(*method, &$obj.method), $result.first))
            return std::make_pair(false, "required [limit_speed_req_t.method] failed:{"+$result.second+"}");
     
        if(!host_management ||!host_management->get_obj()||!($result = gson(*host_management->get_obj(), &$obj.host_management), $result.first))
            return std::make_pair(false, "required [limit_speed_req_t.host_management] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, limit_speed_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, limit_speed_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const login_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.method))
            $node.add_null("method");
        else
            $node.add_text("method", acl::get_value($obj.method));

        if (check_nullptr($obj.login))
            $node.add_null("login");
        else
            $node.add_child("login", acl::gson($json, $obj.login));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const login_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const login_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, login_req_t &$obj)
    {
        acl::json_node *method = $node["method"];
        acl::json_node *login = $node["login"];
        std::pair<bool, std::string> $result;

        if(!method ||!($result = gson(*method, &$obj.method), $result.first))
            return std::make_pair(false, "required [login_req_t.method] failed:{"+$result.second+"}");
     
        if(!login ||!login->get_obj()||!($result = gson(*login->get_obj(), &$obj.login), $result.first))
            return std::make_pair(false, "required [login_req_t.login] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, login_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, login_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const login_res_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.error_code))
            $node.add_null("error_code");
        else
            $node.add_number("error_code", acl::get_value($obj.error_code));

        if (check_nullptr($obj.stok))
            $node.add_null("stok");
        else
            $node.add_text("stok", acl::get_value($obj.stok));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const login_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const login_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, login_res_t &$obj)
    {
        acl::json_node *error_code = $node["error_code"];
        acl::json_node *stok = $node["stok"];
        std::pair<bool, std::string> $result;

        if(!error_code ||!($result = gson(*error_code, &$obj.error_code), $result.first))
            return std::make_pair(false, "required [login_res_t.error_code] failed:{"+$result.second+"}");
     
        if(!stok ||!($result = gson(*stok, &$obj.stok), $result.first))
            return std::make_pair(false, "required [login_res_t.stok] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, login_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, login_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const login_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.username))
            $node.add_null("username");
        else
            $node.add_text("username", acl::get_value($obj.username));

        if (check_nullptr($obj.password))
            $node.add_null("password");
        else
            $node.add_text("password", acl::get_value($obj.password));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const login_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const login_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, login_t &$obj)
    {
        acl::json_node *username = $node["username"];
        acl::json_node *password = $node["password"];
        std::pair<bool, std::string> $result;

        if(!username ||!($result = gson(*username, &$obj.username), $result.first))
            return std::make_pair(false, "required [login_t.username] failed:{"+$result.second+"}");
     
        if(!password ||!($result = gson(*password, &$obj.password), $result.first))
            return std::make_pair(false, "required [login_t.password] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, login_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, login_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const req_global_config_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const req_global_config_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const req_global_config_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_global_config_t &$obj)
    {
        acl::json_node *name = $node["name"];
        std::pair<bool, std::string> $result;

        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [req_global_config_t.name] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_global_config_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, req_global_config_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const req_host_management_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.table))
            $node.add_null("table");
        else
            $node.add_text("table", acl::get_value($obj.table));

        if (check_nullptr($obj.para))
            $node.add_null("para");
        else
            $node.add_child("para", acl::gson($json, $obj.para));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const req_host_management_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const req_host_management_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_host_management_t &$obj)
    {
        acl::json_node *table = $node["table"];
        acl::json_node *para = $node["para"];
        std::pair<bool, std::string> $result;

        if(!table ||!($result = gson(*table, &$obj.table), $result.first))
            return std::make_pair(false, "required [req_host_management_t.table] failed:{"+$result.second+"}");
     
        if(!para ||!para->get_obj()||!($result = gson(*para->get_obj(), &$obj.para), $result.first))
            return std::make_pair(false, "required [req_host_management_t.para] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_host_management_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, req_host_management_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const req_para_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.start))
            $node.add_null("start");
        else
            $node.add_number("start", acl::get_value($obj.start));

        if (check_nullptr($obj.end))
            $node.add_null("end");
        else
            $node.add_number("end", acl::get_value($obj.end));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const req_para_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const req_para_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_para_t &$obj)
    {
        acl::json_node *start = $node["start"];
        acl::json_node *end = $node["end"];
        std::pair<bool, std::string> $result;

        if(!start ||!($result = gson(*start, &$obj.start), $result.first))
            return std::make_pair(false, "required [req_para_t.start] failed:{"+$result.second+"}");
     
        if(!end ||!($result = gson(*end, &$obj.end), $result.first))
            return std::make_pair(false, "required [req_para_t.end] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_para_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, req_para_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const req_wireless_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.table))
            $node.add_null("table");
        else
            $node.add_text("table", acl::get_value($obj.table));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const req_wireless_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const req_wireless_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_wireless_t &$obj)
    {
        acl::json_node *table = $node["table"];
        std::pair<bool, std::string> $result;

        if(!table ||!($result = gson(*table, &$obj.table), $result.first))
            return std::make_pair(false, "required [req_wireless_t.table] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_wireless_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, req_wireless_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const request_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.method))
            $node.add_null("method");
        else
            $node.add_text("method", acl::get_value($obj.method));

        if (check_nullptr($obj.global_config))
            $node.add_null("global_config");
        else
            $node.add_child("global_config", acl::gson($json, $obj.global_config));

        if (check_nullptr($obj.host_management))
            $node.add_null("host_management");
        else
            $node.add_child("host_management", acl::gson($json, $obj.host_management));

        if (check_nullptr($obj.wireless))
            $node.add_null("wireless");
        else
            $node.add_child("wireless", acl::gson($json, $obj.wireless));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const request_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const request_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, request_t &$obj)
    {
        acl::json_node *method = $node["method"];
        acl::json_node *global_config = $node["global_config"];
        acl::json_node *host_management = $node["host_management"];
        acl::json_node *wireless = $node["wireless"];
        std::pair<bool, std::string> $result;

        if(!method ||!($result = gson(*method, &$obj.method), $result.first))
            return std::make_pair(false, "required [request_t.method] failed:{"+$result.second+"}");
     
        if(!global_config ||!global_config->get_obj()||!($result = gson(*global_config->get_obj(), &$obj.global_config), $result.first))
            return std::make_pair(false, "required [request_t.global_config] failed:{"+$result.second+"}");
     
        if(!host_management ||!host_management->get_obj()||!($result = gson(*host_management->get_obj(), &$obj.host_management), $result.first))
            return std::make_pair(false, "required [request_t.host_management] failed:{"+$result.second+"}");
     
        if(!wireless ||!wireless->get_obj()||!($result = gson(*wireless->get_obj(), &$obj.wireless), $result.first))
            return std::make_pair(false, "required [request_t.wireless] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, request_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, request_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const res_count_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.host_info))
            $node.add_null("host_info");
        else
            $node.add_number("host_info", acl::get_value($obj.host_info));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const res_count_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const res_count_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_count_t &$obj)
    {
        acl::json_node *host_info = $node["host_info"];
        std::pair<bool, std::string> $result;

        if(!host_info ||!($result = gson(*host_info, &$obj.host_info), $result.first))
            return std::make_pair(false, "required [res_count_t.host_info] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_count_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, res_count_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const res_host_management_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.host_info))
            $node.add_null("host_info");
        else
            $node.add_child("host_info", acl::gson($json, $obj.host_info));

        if (check_nullptr($obj.count))
            $node.add_null("count");
        else
            $node.add_child("count", acl::gson($json, $obj.count));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const res_host_management_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const res_host_management_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_host_management_t &$obj)
    {
        acl::json_node *host_info = $node["host_info"];
        acl::json_node *count = $node["count"];
        std::pair<bool, std::string> $result;

        if(!host_info ||!host_info->get_obj()||!($result = gson(*host_info->get_obj(), &$obj.host_info), $result.first))
            return std::make_pair(false, "required [res_host_management_t.host_info] failed:{"+$result.second+"}");
     
        if(!count ||!count->get_obj()||!($result = gson(*count->get_obj(), &$obj.count), $result.first))
            return std::make_pair(false, "required [res_host_management_t.count] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_host_management_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, res_host_management_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const res_wireless_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.sta_list))
            $node.add_null("sta_list");
        else
            $node.add_child("sta_list", acl::gson($json, $obj.sta_list));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const res_wireless_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const res_wireless_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_wireless_t &$obj)
    {
        acl::json_node *sta_list = $node["sta_list"];
        std::pair<bool, std::string> $result;

        if(!sta_list ||!sta_list->get_obj()||!($result = gson(*sta_list->get_obj(), &$obj.sta_list), $result.first))
            return std::make_pair(false, "required [res_wireless_t.sta_list] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_wireless_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, res_wireless_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const response_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.host_management))
            $node.add_null("host_management");
        else
            $node.add_child("host_management", acl::gson($json, $obj.host_management));

        if (check_nullptr($obj.wireless))
            $node.add_null("wireless");
        else
            $node.add_child("wireless", acl::gson($json, $obj.wireless));

        if (check_nullptr($obj.error_code))
            $node.add_null("error_code");
        else
            $node.add_number("error_code", acl::get_value($obj.error_code));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const response_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const response_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, response_t &$obj)
    {
        acl::json_node *host_management = $node["host_management"];
        acl::json_node *wireless = $node["wireless"];
        acl::json_node *error_code = $node["error_code"];
        std::pair<bool, std::string> $result;

        if(!host_management ||!host_management->get_obj()||!($result = gson(*host_management->get_obj(), &$obj.host_management), $result.first))
            return std::make_pair(false, "required [response_t.host_management] failed:{"+$result.second+"}");
     
        if(!wireless ||!wireless->get_obj()||!($result = gson(*wireless->get_obj(), &$obj.wireless), $result.first))
            return std::make_pair(false, "required [response_t.wireless] failed:{"+$result.second+"}");
     
        if(!error_code ||!($result = gson(*error_code, &$obj.error_code), $result.first))
            return std::make_pair(false, "required [response_t.error_code] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, response_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, response_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stat_list_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.mac))
            $node.add_null("mac");
        else
            $node.add_text("mac", acl::get_value($obj.mac));

        if (check_nullptr($obj.ip))
            $node.add_null("ip");
        else
            $node.add_text("ip", acl::get_value($obj.ip));

        if (check_nullptr($obj.tx_rate))
            $node.add_null("tx_rate");
        else
            $node.add_text("tx_rate", acl::get_value($obj.tx_rate));

        if (check_nullptr($obj.rx_rate))
            $node.add_null("rx_rate");
        else
            $node.add_text("rx_rate", acl::get_value($obj.rx_rate));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const stat_list_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stat_list_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_list_t &$obj)
    {
        acl::json_node *name = $node["name"];
        acl::json_node *mac = $node["mac"];
        acl::json_node *ip = $node["ip"];
        acl::json_node *tx_rate = $node["tx_rate"];
        acl::json_node *rx_rate = $node["rx_rate"];
        std::pair<bool, std::string> $result;

        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [stat_list_t.name] failed:{"+$result.second+"}");
     
        if(!mac ||!($result = gson(*mac, &$obj.mac), $result.first))
            return std::make_pair(false, "required [stat_list_t.mac] failed:{"+$result.second+"}");
     
        if(!ip ||!($result = gson(*ip, &$obj.ip), $result.first))
            return std::make_pair(false, "required [stat_list_t.ip] failed:{"+$result.second+"}");
     
        if(!tx_rate ||!($result = gson(*tx_rate, &$obj.tx_rate), $result.first))
            return std::make_pair(false, "required [stat_list_t.tx_rate] failed:{"+$result.second+"}");
     
        if(!rx_rate ||!($result = gson(*rx_rate, &$obj.rx_rate), $result.first))
            return std::make_pair(false, "required [stat_list_t.rx_rate] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_list_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stat_list_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


}///end of acl.
