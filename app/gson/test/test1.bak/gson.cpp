#include "stdafx.h"
#include "struct.h"
#include "gson.h"
#include "acl_cpp/serialize/gson_helper.ipp"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const people &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.aa))
            $node.add_null("aa");
        else
            $node.add_text("aa", acl::get_value($obj.aa));

        if (check_nullptr($obj.bb))
            $node.add_null("bb");
        else
            $node.add_text("bb", acl::get_value($obj.bb));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const people *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const people &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, people &$obj)
    {
        acl::json_node *aa = $node["aa"];
        acl::json_node *bb = $node["bb"];
        std::pair<bool, std::string> result;

        if(aa)
            gson(*aa, &$obj.aa);
     
        if(bb)
            gson(*bb, &$obj.bb);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, people *$obj)
    {
        return gson($node, *$obj);
    }


    acl::json_node& gson(acl::json &$json, const user &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.type))
            $node.add_null("type");
        else
            $node.add_child("type", acl::gson($json, $obj.type));

        if (check_nullptr($obj.cmd))
            $node.add_null("cmd");
        else
            $node.add_text("cmd", acl::get_value($obj.cmd));

        if (check_nullptr($obj.user_list))
            $node.add_null("user_list");
        else
            $node.add_child("user_list", acl::gson($json, $obj.user_list));

        if (check_nullptr($obj.user_vector))
            $node.add_null("user_vector");
        else
            $node.add_child("user_vector", acl::gson($json, $obj.user_vector));

        if (check_nullptr($obj.user_map))
            $node.add_null("user_map");
        else
            $node.add_child("user_map", acl::gson($json, $obj.user_map));

        if (check_nullptr($obj.user_list_ptr))
            $node.add_null("user_list_ptr");
        else
            $node.add_child("user_list_ptr", acl::gson($json, $obj.user_list_ptr));

        if (check_nullptr($obj.user_vector_ptr))
            $node.add_null("user_vector_ptr");
        else
            $node.add_child("user_vector_ptr", acl::gson($json, $obj.user_vector_ptr));

        if (check_nullptr($obj.user_map_ptr))
            $node.add_null("user_map_ptr");
        else
            $node.add_child("user_map_ptr", acl::gson($json, $obj.user_map_ptr));

        if (check_nullptr($obj.n))
            $node.add_null("n");
        else
            $node.add_number("n", acl::get_value($obj.n));

        if (check_nullptr($obj.n1))
            $node.add_null("n1");
        else
            $node.add_number("n1", acl::get_value($obj.n1));

        if (check_nullptr($obj.n2))
            $node.add_null("n2");
        else
            $node.add_number("n2", acl::get_value($obj.n2));

        if (check_nullptr($obj.n3))
            $node.add_null("n3");
        else
            $node.add_number("n3", acl::get_value($obj.n3));

        if (check_nullptr($obj.u))
            $node.add_null("u");
        else
            $node.add_child("u", acl::gson($json, $obj.u));


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
        acl::json_node *type = $node["type"];
        acl::json_node *cmd = $node["cmd"];
        acl::json_node *user_list = $node["user_list"];
        acl::json_node *user_vector = $node["user_vector"];
        acl::json_node *user_map = $node["user_map"];
        acl::json_node *user_list_ptr = $node["user_list_ptr"];
        acl::json_node *user_vector_ptr = $node["user_vector_ptr"];
        acl::json_node *user_map_ptr = $node["user_map_ptr"];
        acl::json_node *n = $node["n"];
        acl::json_node *n1 = $node["n1"];
        acl::json_node *n2 = $node["n2"];
        acl::json_node *n3 = $node["n3"];
        acl::json_node *u = $node["u"];
        std::pair<bool, std::string> result;

        if(!type ||!type->get_obj()||!(result = gson(*type->get_obj(), &$obj.type), result.first))
            return std::make_pair(false, "required [user.type] failed:{"+result.second+"}");
     
        if(!cmd ||!(result = gson(*cmd, &$obj.cmd), result.first))
            return std::make_pair(false, "required [user.cmd] failed:{"+result.second+"}");
     
        if(!user_list ||!user_list->get_obj()||!(result = gson(*user_list->get_obj(), &$obj.user_list), result.first))
            return std::make_pair(false, "required [user.user_list] failed:{"+result.second+"}");
     
        if(!user_vector ||!user_vector->get_obj()||!(result = gson(*user_vector->get_obj(), &$obj.user_vector), result.first))
            return std::make_pair(false, "required [user.user_vector] failed:{"+result.second+"}");
     
        if(!user_map ||!user_map->get_obj()||!(result = gson(*user_map->get_obj(), &$obj.user_map), result.first))
            return std::make_pair(false, "required [user.user_map] failed:{"+result.second+"}");
     
        if(!user_list_ptr ||!user_list_ptr->get_obj()||!(result = gson(*user_list_ptr->get_obj(), &$obj.user_list_ptr), result.first))
            return std::make_pair(false, "required [user.user_list_ptr] failed:{"+result.second+"}");
     
        if(!user_vector_ptr ||!user_vector_ptr->get_obj()||!(result = gson(*user_vector_ptr->get_obj(), &$obj.user_vector_ptr), result.first))
            return std::make_pair(false, "required [user.user_vector_ptr] failed:{"+result.second+"}");
     
        if(!user_map_ptr ||!user_map_ptr->get_obj()||!(result = gson(*user_map_ptr->get_obj(), &$obj.user_map_ptr), result.first))
            return std::make_pair(false, "required [user.user_map_ptr] failed:{"+result.second+"}");
     
        if(!n ||!(result = gson(*n, &$obj.n), result.first))
            return std::make_pair(false, "required [user.n] failed:{"+result.second+"}");
     
        if(!n1 ||!(result = gson(*n1, &$obj.n1), result.first))
            return std::make_pair(false, "required [user.n1] failed:{"+result.second+"}");
     
        if(!n2 ||!(result = gson(*n2, &$obj.n2), result.first))
            return std::make_pair(false, "required [user.n2] failed:{"+result.second+"}");
     
        if(!n3 ||!(result = gson(*n3, &$obj.n3), result.first))
            return std::make_pair(false, "required [user.n3] failed:{"+result.second+"}");
     
        if(u&& u->get_obj())
             gson(*u->get_obj(), &$obj.u);
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, user *$obj)
    {
        return gson($node, *$obj);
    }


}///end of acl.
