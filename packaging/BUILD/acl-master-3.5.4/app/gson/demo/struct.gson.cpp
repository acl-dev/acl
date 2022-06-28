#include "stdafx.h"
#include "struct.h"
#include "struct.gson.h"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const base &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.string))
            $node.add_null("string");
        else
            $node.add_text("string", acl::get_value($obj.string));

        if (check_nullptr($obj.string_ptr))
            $node.add_null("string_ptr");
        else
            $node.add_text("string_ptr", acl::get_value($obj.string_ptr));

        if (check_nullptr($obj.a))
            $node.add_null("a");
        else
            $node.add_number("a", acl::get_value($obj.a));

        if (check_nullptr($obj.a_ptr))
            $node.add_null("a_ptr");
        else
            $node.add_number("a_ptr", acl::get_value($obj.a_ptr));

        if (check_nullptr($obj.b))
            $node.add_null("b");
        else
            $node.add_number("b", acl::get_value($obj.b));

        if (check_nullptr($obj.b_ptr))
            $node.add_null("b_ptr");
        else
            $node.add_number("b_ptr", acl::get_value($obj.b_ptr));

        if (check_nullptr($obj.c))
            $node.add_null("c");
        else
            $node.add_number("c", acl::get_value($obj.c));

        if (check_nullptr($obj.c_ptr))
            $node.add_null("c_ptr");
        else
            $node.add_number("c_ptr", acl::get_value($obj.c_ptr));

        if (check_nullptr($obj.d))
            $node.add_null("d");
        else
            $node.add_number("d", acl::get_value($obj.d));

        if (check_nullptr($obj.d_ptr))
            $node.add_null("d_ptr");
        else
            $node.add_number("d_ptr", acl::get_value($obj.d_ptr));

        if (check_nullptr($obj.e))
            $node.add_null("e");
        else
            $node.add_number("e", acl::get_value($obj.e));

        if (check_nullptr($obj.e_ptr))
            $node.add_null("e_ptr");
        else
            $node.add_number("e_ptr", acl::get_value($obj.e_ptr));

        if (check_nullptr($obj.f))
            $node.add_null("f");
        else
            $node.add_number("f", acl::get_value($obj.f));

        if (check_nullptr($obj.f_ptr))
            $node.add_null("f_ptr");
        else
            $node.add_number("f_ptr", acl::get_value($obj.f_ptr));

        if (check_nullptr($obj.g))
            $node.add_null("g");
        else
            $node.add_number("g", acl::get_value($obj.g));

        if (check_nullptr($obj.g_ptr))
            $node.add_null("g_ptr");
        else
            $node.add_number("g_ptr", acl::get_value($obj.g_ptr));

        if (check_nullptr($obj.acl_string))
            $node.add_null("acl_string");
        else
            $node.add_text("acl_string", acl::get_value($obj.acl_string));

        if (check_nullptr($obj.acl_string_ptr))
            $node.add_null("acl_string_ptr");
        else
            $node.add_text("acl_string_ptr", acl::get_value($obj.acl_string_ptr));

        if (check_nullptr($obj.h))
            $node.add_null("h");
        else
            $node.add_double("h", acl::get_value($obj.h));

        if (check_nullptr($obj.h_ptr))
            $node.add_null("h_ptr");
        else
            $node.add_double("h_ptr", acl::get_value($obj.h_ptr));

        if (check_nullptr($obj.i))
            $node.add_null("i");
        else
            $node.add_double("i", acl::get_value($obj.i));

        if (check_nullptr($obj.i_ptr))
            $node.add_null("i_ptr");
        else
            $node.add_double("i_ptr", acl::get_value($obj.i_ptr));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const base *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const base &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, base &$obj)
    {
        acl::json_node *string = $node["string"];
        acl::json_node *string_ptr = $node["string_ptr"];
        acl::json_node *a = $node["a"];
        acl::json_node *a_ptr = $node["a_ptr"];
        acl::json_node *b = $node["b"];
        acl::json_node *b_ptr = $node["b_ptr"];
        acl::json_node *c = $node["c"];
        acl::json_node *c_ptr = $node["c_ptr"];
        acl::json_node *d = $node["d"];
        acl::json_node *d_ptr = $node["d_ptr"];
        acl::json_node *e = $node["e"];
        acl::json_node *e_ptr = $node["e_ptr"];
        acl::json_node *f = $node["f"];
        acl::json_node *f_ptr = $node["f_ptr"];
        acl::json_node *g = $node["g"];
        acl::json_node *g_ptr = $node["g_ptr"];
        acl::json_node *acl_string = $node["acl_string"];
        acl::json_node *acl_string_ptr = $node["acl_string_ptr"];
        acl::json_node *h = $node["h"];
        acl::json_node *h_ptr = $node["h_ptr"];
        acl::json_node *i = $node["i"];
        acl::json_node *i_ptr = $node["i_ptr"];
        std::pair<bool, std::string> $result;

        if(!string ||!($result = gson(*string, &$obj.string), $result.first))
            return std::make_pair(false, "required [base.string] failed:{"+$result.second+"}");
     
        if(string_ptr)
            gson(*string_ptr, &$obj.string_ptr);
     
        if(!a ||!($result = gson(*a, &$obj.a), $result.first))
            return std::make_pair(false, "required [base.a] failed:{"+$result.second+"}");
     
        if(!a_ptr ||!($result = gson(*a_ptr, &$obj.a_ptr), $result.first))
            return std::make_pair(false, "required [base.a_ptr] failed:{"+$result.second+"}");
     
        if(!b ||!($result = gson(*b, &$obj.b), $result.first))
            return std::make_pair(false, "required [base.b] failed:{"+$result.second+"}");
     
        if(!b_ptr ||!($result = gson(*b_ptr, &$obj.b_ptr), $result.first))
            return std::make_pair(false, "required [base.b_ptr] failed:{"+$result.second+"}");
     
        if(!c ||!($result = gson(*c, &$obj.c), $result.first))
            return std::make_pair(false, "required [base.c] failed:{"+$result.second+"}");
     
        if(!c_ptr ||!($result = gson(*c_ptr, &$obj.c_ptr), $result.first))
            return std::make_pair(false, "required [base.c_ptr] failed:{"+$result.second+"}");
     
        if(!d ||!($result = gson(*d, &$obj.d), $result.first))
            return std::make_pair(false, "required [base.d] failed:{"+$result.second+"}");
     
        if(!d_ptr ||!($result = gson(*d_ptr, &$obj.d_ptr), $result.first))
            return std::make_pair(false, "required [base.d_ptr] failed:{"+$result.second+"}");
     
        if(!e ||!($result = gson(*e, &$obj.e), $result.first))
            return std::make_pair(false, "required [base.e] failed:{"+$result.second+"}");
     
        if(!e_ptr ||!($result = gson(*e_ptr, &$obj.e_ptr), $result.first))
            return std::make_pair(false, "required [base.e_ptr] failed:{"+$result.second+"}");
     
        if(!f ||!($result = gson(*f, &$obj.f), $result.first))
            return std::make_pair(false, "required [base.f] failed:{"+$result.second+"}");
     
        if(!f_ptr ||!($result = gson(*f_ptr, &$obj.f_ptr), $result.first))
            return std::make_pair(false, "required [base.f_ptr] failed:{"+$result.second+"}");
     
        if(!g ||!($result = gson(*g, &$obj.g), $result.first))
            return std::make_pair(false, "required [base.g] failed:{"+$result.second+"}");
     
        if(!g_ptr ||!($result = gson(*g_ptr, &$obj.g_ptr), $result.first))
            return std::make_pair(false, "required [base.g_ptr] failed:{"+$result.second+"}");
     
        if(!acl_string ||!($result = gson(*acl_string, &$obj.acl_string), $result.first))
            return std::make_pair(false, "required [base.acl_string] failed:{"+$result.second+"}");
     
        if(!acl_string_ptr ||!($result = gson(*acl_string_ptr, &$obj.acl_string_ptr), $result.first))
            return std::make_pair(false, "required [base.acl_string_ptr] failed:{"+$result.second+"}");
     
        if(!h ||!($result = gson(*h, &$obj.h), $result.first))
            return std::make_pair(false, "required [base.h] failed:{"+$result.second+"}");
     
        if(!h_ptr ||!($result = gson(*h_ptr, &$obj.h_ptr), $result.first))
            return std::make_pair(false, "required [base.h_ptr] failed:{"+$result.second+"}");
     
        if(!i ||!($result = gson(*i, &$obj.i), $result.first))
            return std::make_pair(false, "required [base.i] failed:{"+$result.second+"}");
     
        if(!i_ptr ||!($result = gson(*i_ptr, &$obj.i_ptr), $result.first))
            return std::make_pair(false, "required [base.i_ptr] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, base *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, base &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const hello::world &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.b))
            $node.add_null("b");
        else
            $node.add_child("b", acl::gson($json, $obj.b));

        if (check_nullptr($obj.b_ptr))
            $node.add_null("b_ptr");
        else
            $node.add_child("b_ptr", acl::gson($json, $obj.b_ptr));

        if (check_nullptr($obj.bases_list))
            $node.add_null("bases_list");
        else
            $node.add_child("bases_list", acl::gson($json, $obj.bases_list));

        if (check_nullptr($obj.bases_list_ptr))
            $node.add_null("bases_list_ptr");
        else
            $node.add_child("bases_list_ptr", acl::gson($json, $obj.bases_list_ptr));

        if (check_nullptr($obj.bases_ptr_list_ptr))
            $node.add_null("bases_ptr_list_ptr");
        else
            $node.add_child("bases_ptr_list_ptr", acl::gson($json, $obj.bases_ptr_list_ptr));

        if (check_nullptr($obj.vector_string))
            $node.add_null("vector_string");
        else
            $node.add_child("vector_string", acl::gson($json, $obj.vector_string));

        if (check_nullptr($obj.vector_list_base))
            $node.add_null("vector_list_base");
        else
            $node.add_child("vector_list_base", acl::gson($json, $obj.vector_list_base));

        if (check_nullptr($obj.base_map))
            $node.add_null("base_map");
        else
            $node.add_child("base_map", acl::gson($json, $obj.base_map));

        if (check_nullptr($obj.string_map))
            $node.add_null("string_map");
        else
            $node.add_child("string_map", acl::gson($json, $obj.string_map));

        if (check_nullptr($obj.int_map))
            $node.add_null("int_map");
        else
            $node.add_child("int_map", acl::gson($json, $obj.int_map));

        if (check_nullptr($obj.bool_map))
            $node.add_null("bool_map");
        else
            $node.add_child("bool_map", acl::gson($json, $obj.bool_map));

        if (check_nullptr($obj.base_list_map))
            $node.add_null("base_list_map");
        else
            $node.add_child("base_list_map", acl::gson($json, $obj.base_list_map));

        if (check_nullptr($obj.str_set_))
            $node.add_null("str_set_");
        else
            $node.add_child("str_set_", acl::gson($json, $obj.str_set_));

        if (check_nullptr($obj.int_set_))
            $node.add_null("int_set_");
        else
            $node.add_child("int_set_", acl::gson($json, $obj.int_set_));

        if (check_nullptr($obj.bool_set_))
            $node.add_null("bool_set_");
        else
            $node.add_child("bool_set_", acl::gson($json, $obj.bool_set_));

        if (check_nullptr($obj.me))
            $node.add_null("me");
        else
            $node.add_number("me", acl::get_value($obj.me));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const hello::world *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const hello::world &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, hello::world &$obj)
    {
        acl::json_node *b = $node["b"];
        acl::json_node *b_ptr = $node["b_ptr"];
        acl::json_node *bases_list = $node["bases_list"];
        acl::json_node *bases_list_ptr = $node["bases_list_ptr"];
        acl::json_node *bases_ptr_list_ptr = $node["bases_ptr_list_ptr"];
        acl::json_node *vector_string = $node["vector_string"];
        acl::json_node *vector_list_base = $node["vector_list_base"];
        acl::json_node *base_map = $node["base_map"];
        acl::json_node *string_map = $node["string_map"];
        acl::json_node *int_map = $node["int_map"];
        acl::json_node *bool_map = $node["bool_map"];
        acl::json_node *base_list_map = $node["base_list_map"];
        acl::json_node *str_set_ = $node["str_set_"];
        acl::json_node *int_set_ = $node["int_set_"];
        acl::json_node *bool_set_ = $node["bool_set_"];
        acl::json_node *me = $node["me"];
        std::pair<bool, std::string> $result;

        if(!b ||!b->get_obj()||!($result = gson(*b->get_obj(), &$obj.b), $result.first))
            return std::make_pair(false, "required [hello::world.b] failed:{"+$result.second+"}");
     
        if(!b_ptr ||!b_ptr->get_obj()||!($result = gson(*b_ptr->get_obj(), &$obj.b_ptr), $result.first))
            return std::make_pair(false, "required [hello::world.b_ptr] failed:{"+$result.second+"}");
     
        if(!bases_list ||!bases_list->get_obj()||!($result = gson(*bases_list->get_obj(), &$obj.bases_list), $result.first))
            return std::make_pair(false, "required [hello::world.bases_list] failed:{"+$result.second+"}");
     
        if(!bases_list_ptr ||!bases_list_ptr->get_obj()||!($result = gson(*bases_list_ptr->get_obj(), &$obj.bases_list_ptr), $result.first))
            return std::make_pair(false, "required [hello::world.bases_list_ptr] failed:{"+$result.second+"}");
     
        if(bases_ptr_list_ptr&& bases_ptr_list_ptr->get_obj())
             gson(*bases_ptr_list_ptr->get_obj(), &$obj.bases_ptr_list_ptr);
     
        if(!vector_string ||!vector_string->get_obj()||!($result = gson(*vector_string->get_obj(), &$obj.vector_string), $result.first))
            return std::make_pair(false, "required [hello::world.vector_string] failed:{"+$result.second+"}");
     
        if(!vector_list_base ||!vector_list_base->get_obj()||!($result = gson(*vector_list_base->get_obj(), &$obj.vector_list_base), $result.first))
            return std::make_pair(false, "required [hello::world.vector_list_base] failed:{"+$result.second+"}");
     
        if(!base_map ||!base_map->get_obj()||!($result = gson(*base_map->get_obj(), &$obj.base_map), $result.first))
            return std::make_pair(false, "required [hello::world.base_map] failed:{"+$result.second+"}");
     
        if(!string_map ||!string_map->get_obj()||!($result = gson(*string_map->get_obj(), &$obj.string_map), $result.first))
            return std::make_pair(false, "required [hello::world.string_map] failed:{"+$result.second+"}");
     
        if(!int_map ||!int_map->get_obj()||!($result = gson(*int_map->get_obj(), &$obj.int_map), $result.first))
            return std::make_pair(false, "required [hello::world.int_map] failed:{"+$result.second+"}");
     
        if(!bool_map ||!bool_map->get_obj()||!($result = gson(*bool_map->get_obj(), &$obj.bool_map), $result.first))
            return std::make_pair(false, "required [hello::world.bool_map] failed:{"+$result.second+"}");
     
        if(!base_list_map ||!base_list_map->get_obj()||!($result = gson(*base_list_map->get_obj(), &$obj.base_list_map), $result.first))
            return std::make_pair(false, "required [hello::world.base_list_map] failed:{"+$result.second+"}");
     
        if(!str_set_ ||!str_set_->get_obj()||!($result = gson(*str_set_->get_obj(), &$obj.str_set_), $result.first))
            return std::make_pair(false, "required [hello::world.str_set_] failed:{"+$result.second+"}");
     
        if(!int_set_ ||!int_set_->get_obj()||!($result = gson(*int_set_->get_obj(), &$obj.int_set_), $result.first))
            return std::make_pair(false, "required [hello::world.int_set_] failed:{"+$result.second+"}");
     
        if(!bool_set_ ||!bool_set_->get_obj()||!($result = gson(*bool_set_->get_obj(), &$obj.bool_set_), $result.first))
            return std::make_pair(false, "required [hello::world.bool_set_] failed:{"+$result.second+"}");
     
        if(!me ||!($result = gson(*me, &$obj.me), $result.first))
            return std::make_pair(false, "required [hello::world.me] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, hello::world *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, hello::world &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const list1 &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.b))
            $node.add_null("b");
        else
            $node.add_child("b", acl::gson($json, $obj.b));

        if (check_nullptr($obj.b_ptr))
            $node.add_null("b_ptr");
        else
            $node.add_child("b_ptr", acl::gson($json, $obj.b_ptr));

        if (check_nullptr($obj.bases_list))
            $node.add_null("bases_list");
        else
            $node.add_child("bases_list", acl::gson($json, $obj.bases_list));

        if (check_nullptr($obj.bases_list_ptr))
            $node.add_null("bases_list_ptr");
        else
            $node.add_child("bases_list_ptr", acl::gson($json, $obj.bases_list_ptr));

        if (check_nullptr($obj.bases_ptr_list_ptr))
            $node.add_null("bases_ptr_list_ptr");
        else
            $node.add_child("bases_ptr_list_ptr", acl::gson($json, $obj.bases_ptr_list_ptr));

        if (check_nullptr($obj.vector_string))
            $node.add_null("vector_string");
        else
            $node.add_child("vector_string", acl::gson($json, $obj.vector_string));

        if (check_nullptr($obj.vector_list_base))
            $node.add_null("vector_list_base");
        else
            $node.add_child("vector_list_base", acl::gson($json, $obj.vector_list_base));

        if (check_nullptr($obj.base_map))
            $node.add_null("base_map");
        else
            $node.add_child("base_map", acl::gson($json, $obj.base_map));

        if (check_nullptr($obj.string_map))
            $node.add_null("string_map");
        else
            $node.add_child("string_map", acl::gson($json, $obj.string_map));

        if (check_nullptr($obj.int_map))
            $node.add_null("int_map");
        else
            $node.add_child("int_map", acl::gson($json, $obj.int_map));

        if (check_nullptr($obj.bool_map))
            $node.add_null("bool_map");
        else
            $node.add_child("bool_map", acl::gson($json, $obj.bool_map));

        if (check_nullptr($obj.base_list_map))
            $node.add_null("base_list_map");
        else
            $node.add_child("base_list_map", acl::gson($json, $obj.base_list_map));

        if (check_nullptr($obj.str_set_))
            $node.add_null("str_set_");
        else
            $node.add_child("str_set_", acl::gson($json, $obj.str_set_));

        if (check_nullptr($obj.int_set_))
            $node.add_null("int_set_");
        else
            $node.add_child("int_set_", acl::gson($json, $obj.int_set_));

        if (check_nullptr($obj.bool_set_))
            $node.add_null("bool_set_");
        else
            $node.add_child("bool_set_", acl::gson($json, $obj.bool_set_));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const list1 *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const list1 &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, list1 &$obj)
    {
        acl::json_node *b = $node["b"];
        acl::json_node *b_ptr = $node["b_ptr"];
        acl::json_node *bases_list = $node["bases_list"];
        acl::json_node *bases_list_ptr = $node["bases_list_ptr"];
        acl::json_node *bases_ptr_list_ptr = $node["bases_ptr_list_ptr"];
        acl::json_node *vector_string = $node["vector_string"];
        acl::json_node *vector_list_base = $node["vector_list_base"];
        acl::json_node *base_map = $node["base_map"];
        acl::json_node *string_map = $node["string_map"];
        acl::json_node *int_map = $node["int_map"];
        acl::json_node *bool_map = $node["bool_map"];
        acl::json_node *base_list_map = $node["base_list_map"];
        acl::json_node *str_set_ = $node["str_set_"];
        acl::json_node *int_set_ = $node["int_set_"];
        acl::json_node *bool_set_ = $node["bool_set_"];
        std::pair<bool, std::string> $result;

        if(!b ||!b->get_obj()||!($result = gson(*b->get_obj(), &$obj.b), $result.first))
            return std::make_pair(false, "required [list1.b] failed:{"+$result.second+"}");
     
        if(!b_ptr ||!b_ptr->get_obj()||!($result = gson(*b_ptr->get_obj(), &$obj.b_ptr), $result.first))
            return std::make_pair(false, "required [list1.b_ptr] failed:{"+$result.second+"}");
     
        if(!bases_list ||!bases_list->get_obj()||!($result = gson(*bases_list->get_obj(), &$obj.bases_list), $result.first))
            return std::make_pair(false, "required [list1.bases_list] failed:{"+$result.second+"}");
     
        if(!bases_list_ptr ||!bases_list_ptr->get_obj()||!($result = gson(*bases_list_ptr->get_obj(), &$obj.bases_list_ptr), $result.first))
            return std::make_pair(false, "required [list1.bases_list_ptr] failed:{"+$result.second+"}");
     
        if(bases_ptr_list_ptr&& bases_ptr_list_ptr->get_obj())
             gson(*bases_ptr_list_ptr->get_obj(), &$obj.bases_ptr_list_ptr);
     
        if(!vector_string ||!vector_string->get_obj()||!($result = gson(*vector_string->get_obj(), &$obj.vector_string), $result.first))
            return std::make_pair(false, "required [list1.vector_string] failed:{"+$result.second+"}");
     
        if(!vector_list_base ||!vector_list_base->get_obj()||!($result = gson(*vector_list_base->get_obj(), &$obj.vector_list_base), $result.first))
            return std::make_pair(false, "required [list1.vector_list_base] failed:{"+$result.second+"}");
     
        if(!base_map ||!base_map->get_obj()||!($result = gson(*base_map->get_obj(), &$obj.base_map), $result.first))
            return std::make_pair(false, "required [list1.base_map] failed:{"+$result.second+"}");
     
        if(!string_map ||!string_map->get_obj()||!($result = gson(*string_map->get_obj(), &$obj.string_map), $result.first))
            return std::make_pair(false, "required [list1.string_map] failed:{"+$result.second+"}");
     
        if(!int_map ||!int_map->get_obj()||!($result = gson(*int_map->get_obj(), &$obj.int_map), $result.first))
            return std::make_pair(false, "required [list1.int_map] failed:{"+$result.second+"}");
     
        if(!bool_map ||!bool_map->get_obj()||!($result = gson(*bool_map->get_obj(), &$obj.bool_map), $result.first))
            return std::make_pair(false, "required [list1.bool_map] failed:{"+$result.second+"}");
     
        if(!base_list_map ||!base_list_map->get_obj()||!($result = gson(*base_list_map->get_obj(), &$obj.base_list_map), $result.first))
            return std::make_pair(false, "required [list1.base_list_map] failed:{"+$result.second+"}");
     
        if(!str_set_ ||!str_set_->get_obj()||!($result = gson(*str_set_->get_obj(), &$obj.str_set_), $result.first))
            return std::make_pair(false, "required [list1.str_set_] failed:{"+$result.second+"}");
     
        if(!int_set_ ||!int_set_->get_obj()||!($result = gson(*int_set_->get_obj(), &$obj.int_set_), $result.first))
            return std::make_pair(false, "required [list1.int_set_] failed:{"+$result.second+"}");
     
        if(!bool_set_ ||!bool_set_->get_obj()||!($result = gson(*bool_set_->get_obj(), &$obj.bool_set_), $result.first))
            return std::make_pair(false, "required [list1.bool_set_] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, list1 *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, list1 &$obj)
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
