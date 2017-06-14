#include "stdafx.h"
#include "struct.h"
#include "gson.h"
#include "acl_cpp/serialize/gson_helper.ipp"
namespace acl
{
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
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [list_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [list_res_t.msg] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, list_res_t *$obj)
    {
        return gson($node, *$obj);
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


    acl::json_node& gson(acl::json &$json, const start_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));


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
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [start_req_t.cmd] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_req_t *$obj)
    {
        return gson($node, *$obj);
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
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [start_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [start_res_t.msg] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, start_res_t *$obj)
    {
        return gson($node, *$obj);
    }


    acl::json_node& gson(acl::json &$json, const stat_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));


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
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [stat_req_t.cmd] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_req_t *$obj)
    {
        return gson($node, *$obj);
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
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [stat_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [stat_res_t.msg] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stat_res_t *$obj)
    {
        return gson($node, *$obj);
    }


    acl::json_node& gson(acl::json &$json, const stop_req_t &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));


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
        std::pair<bool, std::string> $result;

        if(!cmd ||!($result = gson(*cmd, &$obj.cmd), $result.first))
            return std::make_pair(false, "required [stop_req_t.cmd] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_req_t *$obj)
    {
        return gson($node, *$obj);
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
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [stop_res_t.status] failed:{"+$result.second+"}");
     
        if(!msg ||!($result = gson(*msg, &$obj.msg), $result.first))
            return std::make_pair(false, "required [stop_res_t.msg] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, stop_res_t *$obj)
    {
        return gson($node, *$obj);
    }


}///end of acl.
