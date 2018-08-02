#include "stdafx.h"
#include "struct.h"
#include "struct.gson.h"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const files_outdate &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.files))
            $node.add_null("files");
        else
            $node.add_child("files", acl::gson($json, $obj.files));

        if (check_nullptr($obj.values))
            $node.add_null("values");
        else
            $node.add_child("values", acl::gson($json, $obj.values));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const files_outdate *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const files_outdate &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, files_outdate &$obj)
    {
        acl::json_node *files = $node["files"];
        acl::json_node *values = $node["values"];
        std::pair<bool, std::string> $result;

        if(!files ||!files->get_obj()||!($result = gson(*files->get_obj(), &$obj.files), $result.first))
            return std::make_pair(false, "required [files_outdate.files] failed:{"+$result.second+"}");
     
        if(!values ||!values->get_obj()||!($result = gson(*values->get_obj(), &$obj.values), $result.first))
            return std::make_pair(false, "required [files_outdate.values] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, files_outdate *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, files_outdate &$obj)
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
