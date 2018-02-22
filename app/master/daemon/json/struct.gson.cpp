#include "stdafx.h"
#include "struct.h"
#include "struct.gson.h"
#include "acl_cpp/serialize/gson_helper.ipp"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const kill_req_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const kill_req_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const kill_req_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_data_t &$obj)
    {
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [kill_req_data_t.path] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, kill_req_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const kill_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));

        if (check_nullptr($obj.data))
            $node.add_null("data");
        else
            $node.add_child("data", acl::gson($json, $obj.data));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const kill_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const kill_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [kill_req_t.cmd] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [kill_req_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, kill_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const kill_res_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const kill_res_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const kill_res_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_data_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [kill_res_data_t.status] failed:{"+$result.second+"}");
     
        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [kill_res_data_t.path] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, kill_res_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const kill_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const kill_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const kill_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [kill_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [kill_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [kill_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, kill_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, kill_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const list_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const list_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const list_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, list_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [list_req_t.cmd] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, list_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, list_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const list_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const list_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const list_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, list_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [list_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [list_res_t.msg] failed:{"+$result.second+"}");
     
        if(data&& data->get_obj())
             gson(*data->get_obj(), &$obj.data);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, list_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, list_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const master_config_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const master_config_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const master_config_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [master_config_req_t.cmd] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, master_config_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const master_config_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const master_config_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const master_config_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [master_config_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [master_config_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [master_config_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, master_config_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, master_config_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const proc_info_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.pid))
            $node.add_null("pid");
        else
            $node.add_number("pid", acl::get_value($obj.pid));

        if (check_nullptr($obj.start))
            $node.add_null("start");
        else
            $node.add_number("start", acl::get_value($obj.start));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const proc_info_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const proc_info_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, proc_info_t &$obj)
    {
        acl::json_node *pid = $node["pid"];
        acl::json_node *start = $node["start"];
        std::pair<bool, std::string> $result;

        if(!pid ||!($result = gson(*pid, &$obj.pid), $result.first))
            return std::make_pair(false, "required [proc_info_t.pid] failed:{"+$result.second+"}");
     
        if(!start ||!($result = gson(*start, &$obj.start), $result.first))
            return std::make_pair(false, "required [proc_info_t.start] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, proc_info_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, proc_info_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const reload_req_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const reload_req_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const reload_req_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_data_t &$obj)
    {
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [reload_req_data_t.path] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, reload_req_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const reload_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));

        if (check_nullptr($obj.timeout))
            $node.add_null("timeout");
        else
            $node.add_number("timeout", acl::get_value($obj.timeout));

        if (check_nullptr($obj.data))
            $node.add_null("data");
        else
            $node.add_child("data", acl::gson($json, $obj.data));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const reload_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const reload_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        acl::json_node *timeout = $node["timeout"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [reload_req_t.cmd] failed:{"+$result.second+"}");
     
        if(timeout)
            gson(*timeout, &$obj.timeout);
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [reload_req_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, reload_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const reload_res_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.proc_count))
            $node.add_null("proc_count");
        else
            $node.add_number("proc_count", acl::get_value($obj.proc_count));

        if (check_nullptr($obj.proc_signaled))
            $node.add_null("proc_signaled");
        else
            $node.add_number("proc_signaled", acl::get_value($obj.proc_signaled));

        if (check_nullptr($obj.proc_ok))
            $node.add_null("proc_ok");
        else
            $node.add_number("proc_ok", acl::get_value($obj.proc_ok));

        if (check_nullptr($obj.proc_err))
            $node.add_null("proc_err");
        else
            $node.add_number("proc_err", acl::get_value($obj.proc_err));

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const reload_res_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const reload_res_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_data_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *proc_count = $node["proc_count"];
        acl::json_node *proc_signaled = $node["proc_signaled"];
        acl::json_node *proc_ok = $node["proc_ok"];
        acl::json_node *proc_err = $node["proc_err"];
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [reload_res_data_t.status] failed:{"+$result.second+"}");
     
        if(!proc_count ||!($result = gson(*proc_count, &$obj.proc_count), $result.first))
            return std::make_pair(false, "required [reload_res_data_t.proc_count] failed:{"+$result.second+"}");
     
        if(!proc_signaled ||!($result = gson(*proc_signaled, &$obj.proc_signaled), $result.first))
            return std::make_pair(false, "required [reload_res_data_t.proc_signaled] failed:{"+$result.second+"}");
     
        if(!proc_ok ||!($result = gson(*proc_ok, &$obj.proc_ok), $result.first))
            return std::make_pair(false, "required [reload_res_data_t.proc_ok] failed:{"+$result.second+"}");
     
        if(!proc_err ||!($result = gson(*proc_err, &$obj.proc_err), $result.first))
            return std::make_pair(false, "required [reload_res_data_t.proc_err] failed:{"+$result.second+"}");
     
        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [reload_res_data_t.path] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, reload_res_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const reload_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const reload_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const reload_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [reload_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [reload_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [reload_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, reload_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, reload_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [req_t.cmd] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const res_t &$obj)
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


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [res_t.msg] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const restart_req_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));

        if (check_nullptr($obj.ext))
            $node.add_null("ext");
        else
            $node.add_text("ext", acl::get_value($obj.ext));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const restart_req_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const restart_req_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_data_t &$obj)
    {
        acl::json_node *path = $node["path"];
        acl::json_node *ext = $node["ext"];
        std::pair<bool, std::string> $result;

        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [restart_req_data_t.path] failed:{"+$result.second+"}");
     
        if(ext)
            gson(*ext, &$obj.ext);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, restart_req_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const restart_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));

        if (check_nullptr($obj.data))
            $node.add_null("data");
        else
            $node.add_child("data", acl::gson($json, $obj.data));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const restart_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const restart_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [restart_req_t.cmd] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [restart_req_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, restart_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const restart_res_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const restart_res_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const restart_res_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_data_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *name = $node["name"];
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [restart_res_data_t.status] failed:{"+$result.second+"}");
     
        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [restart_res_data_t.name] failed:{"+$result.second+"}");
     
        if(path)
            gson(*path, &$obj.path);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, restart_res_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const restart_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const restart_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const restart_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [restart_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [restart_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [restart_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, restart_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, restart_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const serv_info_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.type))
            $node.add_null("type");
        else
            $node.add_number("type", acl::get_value($obj.type));

        if (check_nullptr($obj.start))
            $node.add_null("start");
        else
            $node.add_number("start", acl::get_value($obj.start));

        if (check_nullptr($obj.owner))
            $node.add_null("owner");
        else
            $node.add_text("owner", acl::get_value($obj.owner));

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));

        if (check_nullptr($obj.conf))
            $node.add_null("conf");
        else
            $node.add_text("conf", acl::get_value($obj.conf));

        if (check_nullptr($obj.proc_max))
            $node.add_null("proc_max");
        else
            $node.add_number("proc_max", acl::get_value($obj.proc_max));

        if (check_nullptr($obj.proc_prefork))
            $node.add_null("proc_prefork");
        else
            $node.add_number("proc_prefork", acl::get_value($obj.proc_prefork));

        if (check_nullptr($obj.proc_total))
            $node.add_null("proc_total");
        else
            $node.add_number("proc_total", acl::get_value($obj.proc_total));

        if (check_nullptr($obj.proc_avail))
            $node.add_null("proc_avail");
        else
            $node.add_number("proc_avail", acl::get_value($obj.proc_avail));

        if (check_nullptr($obj.throttle_delay))
            $node.add_null("throttle_delay");
        else
            $node.add_number("throttle_delay", acl::get_value($obj.throttle_delay));

        if (check_nullptr($obj.listen_fd_count))
            $node.add_null("listen_fd_count");
        else
            $node.add_number("listen_fd_count", acl::get_value($obj.listen_fd_count));

        if (check_nullptr($obj.notify_addr))
            $node.add_null("notify_addr");
        else
            $node.add_text("notify_addr", acl::get_value($obj.notify_addr));

        if (check_nullptr($obj.notify_recipients))
            $node.add_null("notify_recipients");
        else
            $node.add_text("notify_recipients", acl::get_value($obj.notify_recipients));

        if (check_nullptr($obj.version))
            $node.add_null("version");
        else
            $node.add_text("version", acl::get_value($obj.version));

        if (check_nullptr($obj.env))
            $node.add_null("env");
        else
            $node.add_child("env", acl::gson($json, $obj.env));

        if (check_nullptr($obj.procs))
            $node.add_null("procs");
        else
            $node.add_child("procs", acl::gson($json, $obj.procs));

        if (check_nullptr($obj.check_fds))
            $node.add_null("check_fds");
        else
            $node.add_bool("check_fds", acl::get_value($obj.check_fds));

        if (check_nullptr($obj.check_mem))
            $node.add_null("check_mem");
        else
            $node.add_bool("check_mem", acl::get_value($obj.check_mem));

        if (check_nullptr($obj.check_cpu))
            $node.add_null("check_cpu");
        else
            $node.add_bool("check_cpu", acl::get_value($obj.check_cpu));

        if (check_nullptr($obj.check_io))
            $node.add_null("check_io");
        else
            $node.add_bool("check_io", acl::get_value($obj.check_io));

        if (check_nullptr($obj.check_limits))
            $node.add_null("check_limits");
        else
            $node.add_bool("check_limits", acl::get_value($obj.check_limits));

        if (check_nullptr($obj.check_net))
            $node.add_null("check_net");
        else
            $node.add_bool("check_net", acl::get_value($obj.check_net));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const serv_info_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const serv_info_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, serv_info_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *name = $node["name"];
        acl::json_node *type = $node["type"];
        acl::json_node *start = $node["start"];
        acl::json_node *owner = $node["owner"];
        acl::json_node *path = $node["path"];
        acl::json_node *conf = $node["conf"];
        acl::json_node *proc_max = $node["proc_max"];
        acl::json_node *proc_prefork = $node["proc_prefork"];
        acl::json_node *proc_total = $node["proc_total"];
        acl::json_node *proc_avail = $node["proc_avail"];
        acl::json_node *throttle_delay = $node["throttle_delay"];
        acl::json_node *listen_fd_count = $node["listen_fd_count"];
        acl::json_node *notify_addr = $node["notify_addr"];
        acl::json_node *notify_recipients = $node["notify_recipients"];
        acl::json_node *version = $node["version"];
        acl::json_node *env = $node["env"];
        acl::json_node *procs = $node["procs"];
        acl::json_node *check_fds = $node["check_fds"];
        acl::json_node *check_mem = $node["check_mem"];
        acl::json_node *check_cpu = $node["check_cpu"];
        acl::json_node *check_io = $node["check_io"];
        acl::json_node *check_limits = $node["check_limits"];
        acl::json_node *check_net = $node["check_net"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [serv_info_t.status] failed:{"+$result.second+"}");
     
        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [serv_info_t.name] failed:{"+$result.second+"}");
     
        if(!type ||!($result = gson(*type, &$obj.type), $result.first))
            return std::make_pair(false, "required [serv_info_t.type] failed:{"+$result.second+"}");
     
        if(!start ||!($result = gson(*start, &$obj.start), $result.first))
            return std::make_pair(false, "required [serv_info_t.start] failed:{"+$result.second+"}");
     
        if(owner)
            gson(*owner, &$obj.owner);
     
        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [serv_info_t.path] failed:{"+$result.second+"}");
     
        if(!conf ||!($result = gson(*conf, &$obj.conf), $result.first))
            return std::make_pair(false, "required [serv_info_t.conf] failed:{"+$result.second+"}");
     
        if(!proc_max ||!($result = gson(*proc_max, &$obj.proc_max), $result.first))
            return std::make_pair(false, "required [serv_info_t.proc_max] failed:{"+$result.second+"}");
     
        if(!proc_prefork ||!($result = gson(*proc_prefork, &$obj.proc_prefork), $result.first))
            return std::make_pair(false, "required [serv_info_t.proc_prefork] failed:{"+$result.second+"}");
     
        if(!proc_total ||!($result = gson(*proc_total, &$obj.proc_total), $result.first))
            return std::make_pair(false, "required [serv_info_t.proc_total] failed:{"+$result.second+"}");
     
        if(!proc_avail ||!($result = gson(*proc_avail, &$obj.proc_avail), $result.first))
            return std::make_pair(false, "required [serv_info_t.proc_avail] failed:{"+$result.second+"}");
     
        if(!throttle_delay ||!($result = gson(*throttle_delay, &$obj.throttle_delay), $result.first))
            return std::make_pair(false, "required [serv_info_t.throttle_delay] failed:{"+$result.second+"}");
     
        if(!listen_fd_count ||!($result = gson(*listen_fd_count, &$obj.listen_fd_count), $result.first))
            return std::make_pair(false, "required [serv_info_t.listen_fd_count] failed:{"+$result.second+"}");
     
        if(notify_addr)
            gson(*notify_addr, &$obj.notify_addr);
     
        if(notify_recipients)
            gson(*notify_recipients, &$obj.notify_recipients);
     
        if(version)
            gson(*version, &$obj.version);
     
        if(env&& env->get_obj())
             gson(*env->get_obj(), &$obj.env);
     
        if(procs&& procs->get_obj())
             gson(*procs->get_obj(), &$obj.procs);
     
        if(check_fds)
            gson(*check_fds, &$obj.check_fds);
     
        if(check_mem)
            gson(*check_mem, &$obj.check_mem);
     
        if(check_cpu)
            gson(*check_cpu, &$obj.check_cpu);
     
        if(check_io)
            gson(*check_io, &$obj.check_io);
     
        if(check_limits)
            gson(*check_limits, &$obj.check_limits);
     
        if(check_net)
            gson(*check_net, &$obj.check_net);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, serv_info_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, serv_info_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const start_req_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));

        if (check_nullptr($obj.ext))
            $node.add_null("ext");
        else
            $node.add_text("ext", acl::get_value($obj.ext));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const start_req_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const start_req_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_data_t &$obj)
    {
        acl::json_node *path = $node["path"];
        acl::json_node *ext = $node["ext"];
        std::pair<bool, std::string> $result;

        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [start_req_data_t.path] failed:{"+$result.second+"}");
     
        if(ext)
            gson(*ext, &$obj.ext);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, start_req_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const start_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));

        if (check_nullptr($obj.timeout))
            $node.add_null("timeout");
        else
            $node.add_number("timeout", acl::get_value($obj.timeout));

        if (check_nullptr($obj.data))
            $node.add_null("data");
        else
            $node.add_child("data", acl::gson($json, $obj.data));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const start_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const start_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        acl::json_node *timeout = $node["timeout"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [start_req_t.cmd] failed:{"+$result.second+"}");
     
        if(timeout)
            gson(*timeout, &$obj.timeout);
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [start_req_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, start_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const start_res_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.proc_count))
            $node.add_null("proc_count");
        else
            $node.add_number("proc_count", acl::get_value($obj.proc_count));

        if (check_nullptr($obj.proc_signaled))
            $node.add_null("proc_signaled");
        else
            $node.add_number("proc_signaled", acl::get_value($obj.proc_signaled));

        if (check_nullptr($obj.proc_ok))
            $node.add_null("proc_ok");
        else
            $node.add_number("proc_ok", acl::get_value($obj.proc_ok));

        if (check_nullptr($obj.proc_err))
            $node.add_null("proc_err");
        else
            $node.add_number("proc_err", acl::get_value($obj.proc_err));

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const start_res_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const start_res_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_data_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *proc_count = $node["proc_count"];
        acl::json_node *proc_signaled = $node["proc_signaled"];
        acl::json_node *proc_ok = $node["proc_ok"];
        acl::json_node *proc_err = $node["proc_err"];
        acl::json_node *name = $node["name"];
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [start_res_data_t.status] failed:{"+$result.second+"}");
     
        if(!proc_count ||!($result = gson(*proc_count, &$obj.proc_count), $result.first))
            return std::make_pair(false, "required [start_res_data_t.proc_count] failed:{"+$result.second+"}");
     
        if(!proc_signaled ||!($result = gson(*proc_signaled, &$obj.proc_signaled), $result.first))
            return std::make_pair(false, "required [start_res_data_t.proc_signaled] failed:{"+$result.second+"}");
     
        if(!proc_ok ||!($result = gson(*proc_ok, &$obj.proc_ok), $result.first))
            return std::make_pair(false, "required [start_res_data_t.proc_ok] failed:{"+$result.second+"}");
     
        if(!proc_err ||!($result = gson(*proc_err, &$obj.proc_err), $result.first))
            return std::make_pair(false, "required [start_res_data_t.proc_err] failed:{"+$result.second+"}");
     
        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [start_res_data_t.name] failed:{"+$result.second+"}");
     
        if(path)
            gson(*path, &$obj.path);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, start_res_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const start_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const start_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const start_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [start_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [start_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [start_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, start_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stat_req_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const stat_req_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stat_req_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_data_t &$obj)
    {
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [stat_req_data_t.path] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stat_req_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stat_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));

        if (check_nullptr($obj.data))
            $node.add_null("data");
        else
            $node.add_child("data", acl::gson($json, $obj.data));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const stat_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stat_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [stat_req_t.cmd] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [stat_req_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stat_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stat_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const stat_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stat_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [stat_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [stat_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [stat_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stat_res_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stop_req_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const stop_req_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stop_req_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_data_t &$obj)
    {
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [stop_req_data_t.path] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stop_req_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stop_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));

        if (check_nullptr($obj.data))
            $node.add_null("data");
        else
            $node.add_child("data", acl::gson($json, $obj.data));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const stop_req_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stop_req_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_t &$obj)
    {
        acl::json_node *cmd = $node["cmd"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [stop_req_t.cmd] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [stop_req_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stop_req_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stop_res_data_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_number("status", acl::get_value($obj.status));

        if (check_nullptr($obj.path))
            $node.add_null("path");
        else
            $node.add_text("path", acl::get_value($obj.path));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const stop_res_data_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stop_res_data_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_data_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *path = $node["path"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [stop_res_data_t.status] failed:{"+$result.second+"}");
     
        if(!path ||!($result = gson(*path, &$obj.path), $result.first))
            return std::make_pair(false, "required [stop_res_data_t.path] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_data_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stop_res_data_t &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const stop_res_t &$obj)
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
    
    acl::json_node& gson(acl::json &$json, const stop_res_t *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const stop_res_t &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_t &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *msg = $node["msg"];
        acl::json_node *data = $node["data"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [stop_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [stop_res_t.msg] failed:{"+$result.second+"}");
     
        if(!data ||!data->get_obj()||!($result = gson(*data->get_obj(), &$obj.data), $result.first))
            return std::make_pair(false, "required [stop_res_t.data] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_t *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, stop_res_t &$obj)
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
