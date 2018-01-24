#include "stdafx.h"
#include "service_struct.h"
#include "service_struct.gson.h"
#include "acl_cpp/serialize/gson_helper.ipp"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const service_info_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.start))
            $node.add_null("start");
        else
            $node.add_number("start", acl::get_value($obj.start));

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.conf))
            $node.add_null("conf");
        else
            $node.add_text("conf", acl::get_value($obj.conf));

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));

        if (check_nullptr($obj.version))
            $node.add_null("version");
        else
            $node.add_text("version", acl::get_value($obj.version));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const service_info_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const service_info_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, service_info_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *start = $node["start"];
        acl::json_node *name = $node["name"];
        acl::json_node *conf = $node["conf"];
        acl::json_node *path = $node["path"];
        acl::json_node *version = $node["version"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [service_info_t.status] failed:{"+$result.second+"}");
     
        if(!start ||!($result = gson(*start, &$obj.start), $result.first))
            return std::make_pair(false, "required [service_info_t.start] failed:{"+$result.second+"}");
     
        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [service_info_t.name] failed:{"+$result.second+"}");
     
        if(!conf ||!($result = gson(*conf, &$obj.conf), $result.first))
            return std::make_pair(false, "required [service_info_t.conf] failed:{"+$result.second+"}");
     
        if(path)
            gson(*path, &$obj.path);
     
        if(version)
            gson(*version, &$obj.version);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, service_info_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, service_info_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const service_list_res_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.msg))
            $node.add_null("msg");
        else
            $node.add_text("msg", acl::get_value($obj.msg));

        if (check_nullptr($obj.data))
            $node.add_null("data");
        else
            $node.add_child("data", acl::gson($json, $obj.data));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const service_list_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const service_list_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, service_list_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [service_list_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [service_list_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [service_list_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, service_list_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, service_list_res_t &$obj)
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
