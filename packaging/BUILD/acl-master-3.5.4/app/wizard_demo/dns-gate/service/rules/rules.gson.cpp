#include "stdafx.h"
#include "rules.h"
#include "rules.gson.h"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const filter_rule &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.names))
            $node.add_null("names");
        else
            $node.add_child("names", acl::gson($json, $obj.names));

        if (check_nullptr($obj.hells))
            $node.add_null("hells");
        else
            $node.add_child("hells", acl::gson($json, $obj.hells));

        if (check_nullptr($obj.enables))
            $node.add_null("enables");
        else
            $node.add_child("enables", acl::gson($json, $obj.enables));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const filter_rule *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const filter_rule &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rule &$obj)
    {
        acl::json_node *names = $node["names"];
        acl::json_node *hells = $node["hells"];
        acl::json_node *enables = $node["enables"];
        std::pair<bool, std::string> $result;

        if(!names ||!names->get_obj()||!($result = gson(*names->get_obj(), &$obj.names), $result.first))
            return std::make_pair(false, "required [filter_rule.names] failed:{"+$result.second+"}");
     
        if(!hells ||!hells->get_obj()||!($result = gson(*hells->get_obj(), &$obj.hells), $result.first))
            return std::make_pair(false, "required [filter_rule.hells] failed:{"+$result.second+"}");
     
        if(!enables ||!enables->get_obj()||!($result = gson(*enables->get_obj(), &$obj.enables), $result.first))
            return std::make_pair(false, "required [filter_rule.enables] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rule *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, filter_rule &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const filter_rules &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.rules))
            $node.add_null("rules");
        else
            $node.add_child("rules", acl::gson($json, $obj.rules));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const filter_rules *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const filter_rules &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rules &$obj)
    {
        acl::json_node *rules = $node["rules"];
        std::pair<bool, std::string> $result;

        if(!rules ||!rules->get_obj()||!($result = gson(*rules->get_obj(), &$obj.rules), $result.first))
            return std::make_pair(false, "required [filter_rules.rules] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, filter_rules *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, filter_rules &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const time_enable &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.from_hour))
            $node.add_null("from_hour");
        else
            $node.add_number("from_hour", acl::get_value($obj.from_hour));

        if (check_nullptr($obj.from_min))
            $node.add_null("from_min");
        else
            $node.add_number("from_min", acl::get_value($obj.from_min));

        if (check_nullptr($obj.to_hour))
            $node.add_null("to_hour");
        else
            $node.add_number("to_hour", acl::get_value($obj.to_hour));

        if (check_nullptr($obj.to_min))
            $node.add_null("to_min");
        else
            $node.add_number("to_min", acl::get_value($obj.to_min));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const time_enable *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const time_enable &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, time_enable &$obj)
    {
        acl::json_node *from_hour = $node["from_hour"];
        acl::json_node *from_min = $node["from_min"];
        acl::json_node *to_hour = $node["to_hour"];
        acl::json_node *to_min = $node["to_min"];
        std::pair<bool, std::string> $result;

        if(!from_hour ||!($result = gson(*from_hour, &$obj.from_hour), $result.first))
            return std::make_pair(false, "required [time_enable.from_hour] failed:{"+$result.second+"}");
     
        if(!from_min ||!($result = gson(*from_min, &$obj.from_min), $result.first))
            return std::make_pair(false, "required [time_enable.from_min] failed:{"+$result.second+"}");
     
        if(!to_hour ||!($result = gson(*to_hour, &$obj.to_hour), $result.first))
            return std::make_pair(false, "required [time_enable.to_hour] failed:{"+$result.second+"}");
     
        if(!to_min ||!($result = gson(*to_min, &$obj.to_min), $result.first))
            return std::make_pair(false, "required [time_enable.to_min] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, time_enable *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, time_enable &$obj)
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
