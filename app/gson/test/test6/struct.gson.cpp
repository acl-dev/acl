#include "stdafx.h"
#include "struct.h"
#include "struct.gson.h"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const company &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.groups))
            $node.add_null("groups");
        else
            $node.add_child("groups", acl::gson($json, $obj.groups));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const company *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const company &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, company &$obj)
    {
        acl::json_node *name = $node["name"];
        acl::json_node *groups = $node["groups"];
        std::pair<bool, std::string> $result;

        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [company.name] failed:{"+$result.second+"}");
     
        if(!groups ||!groups->get_obj()||!($result = gson(*groups->get_obj(), &$obj.groups), $result.first))
            return std::make_pair(false, "required [company.groups] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, company *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, company &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const group &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.users))
            $node.add_null("users");
        else
            $node.add_child("users", acl::gson($json, $obj.users));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const group *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const group &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, group &$obj)
    {
        acl::json_node *name = $node["name"];
        acl::json_node *users = $node["users"];
        std::pair<bool, std::string> $result;

        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [group.name] failed:{"+$result.second+"}");
     
        if(!users ||!users->get_obj()||!($result = gson(*users->get_obj(), &$obj.users), $result.first))
            return std::make_pair(false, "required [group.users] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, group *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, group &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const user &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.name))
            $node.add_null("name");
        else
            $node.add_text("name", acl::get_value($obj.name));

        if (check_nullptr($obj.age))
            $node.add_null("age");
        else
            $node.add_number("age", acl::get_value($obj.age));

        if (check_nullptr($obj.male))
            $node.add_null("male");
        else
            $node.add_bool("male", acl::get_value($obj.male));

        if (check_nullptr($obj.addr))
            $node.add_null("addr");
        else
            $node.add_text("addr", acl::get_value($obj.addr));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const user *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const user &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, user &$obj)
    {
        acl::json_node *name = $node["name"];
        acl::json_node *age = $node["age"];
        acl::json_node *male = $node["male"];
        acl::json_node *addr = $node["addr"];
        std::pair<bool, std::string> $result;

        if(!name ||!($result = gson(*name, &$obj.name), $result.first))
            return std::make_pair(false, "required [user.name] failed:{"+$result.second+"}");
     
        if(!age ||!($result = gson(*age, &$obj.age), $result.first))
            return std::make_pair(false, "required [user.age] failed:{"+$result.second+"}");
     
        if(!male ||!($result = gson(*male, &$obj.male), $result.first))
            return std::make_pair(false, "required [user.male] failed:{"+$result.second+"}");
     
        if(addr)
            gson(*addr, &$obj.addr);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, user *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, user &$obj)
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
