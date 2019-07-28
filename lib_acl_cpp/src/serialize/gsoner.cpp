/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   E-mail: niukey@qq.com
 * 
 * VERSION
 *   Sat 08 Oct 2016 09:08:14 PM CST
 */

#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/json.hpp"
#endif

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

#ifndef ACL_CLIENT_ONLY

namespace acl
{

struct syntax_error : std::exception
{
	syntax_error(void) {}
	syntax_error(const char* s) : exception()
	{
		std::cout << s << std::endl;
	}
};

struct unsupported_type : std::exception
{
	unsupported_type(void) {}
	unsupported_type(const char* s) : exception()
	{
		std::cout << s << std::endl;
	}
};

struct parent_level_error : std::exception
{
	parent_level_error(void) {}
	parent_level_error(const char* s) : exception()
	{
		std::cout << s << std::endl;
	}
};

gsoner::field_t::field_t(type_t t, const std::string &name, bool required)
: type_(t)
, name_(name)
, required_(required)
{
}

gsoner::field_t::field_t(void)
{
}

gsoner::field_t::~field_t(void)
{
}

void gsoner::object_t::reset(void)
{
	fields_.clear();
	name_.clear();
}

void gsoner::set_default_required(void)
{
	default_ = true;
}

void gsoner::set_default_optional(void)
{
	default_ = false;
}

void gsoner::set_header_filename(const std::string &filename)
{
	gen_header_filename_ = filename;
}

void gsoner::set_source_filename(const std::string &filename)
{
	gen_source_filename_ = filename;
}

gsoner::gsoner(void)
{
	status_ = e_uninit;
	gen_header_ = NULL;
	gen_source_ = NULL;
	default_ = true;
	skip_ = false;
	required_ = default_;
	gen_header_filename_ = "gson.h";
	gen_source_filename_ = "gson.cpp";
	default_delimiters_ = "\\\r\n\t ";
	pos_ = 0;
	tab_ = "    ";
}

gsoner::parent_obj_t::level_t gsoner::get_level(std::string str)
{
	if(str == "public") {
		return parent_obj_t::e_public;
	} else if(str == "protect") {
		return parent_obj_t::e_protect;
	} else if(str == "private") {
		return parent_obj_t::e_private;
	} else {
		throw parent_level_error();
	}

	// unreached!
	return parent_obj_t::e_error;
}

std::string gsoner::get_node_func(const field_t &field)
{
	string code;

	switch(field.type_) {
	case gsoner::field_t::e_bool:
	case field_t::e_bool_ptr:
		return "add_bool";
	case gsoner::field_t::e_number:
		return "add_number";
	case gsoner::field_t::e_double:
		return "add_double";
	case gsoner::field_t::e_string:
	case gsoner::field_t::e_cstr:
	case gsoner::field_t::e_ccstr:
		return "add_text";
	case gsoner::field_t::e_list:
	case gsoner::field_t::e_vector:
	case gsoner::field_t::e_map:
	case gsoner::field_t::e_object:
	case gsoner::field_t::e_set:
		return "add_child";
	default:
		break;
	}

	return "error_type";
}

std::string gsoner::get_gson_func_laber(const field_t &field)
{
	string code;

	switch(field.type_) {
	case gsoner::field_t::e_list:
	case gsoner::field_t::e_vector:
	case gsoner::field_t::e_map:
	case gsoner::field_t::e_set:
	case gsoner::field_t::e_object:
		return "acl::gson($json, ";
	default:
		return "acl::get_value(";
	}

	return "error_type";
}

gsoner::function_code_t gsoner::gen_pack_code(const object_t &obj)
{
	function_code_t code;
	std::string str;
	str += "acl::json_node& gson(acl::json &$json, const ";
	str += obj.name_;

	code.declare_ptr_ = str;
	code.declare_ptr_ += " *$obj);";
	code.declare2_ = "acl::string gson(const ";
	code.declare2_ += obj.name_;
	code.declare2_ += " &$obj);";
	code.definition2_ = code.declare2_.substr(0, code.declare2_.find(";"));
	code.definition2_ += "\n{\n"
		"    acl::json $json;\n"
		"    acl::json_node &$node = acl::gson ($json, $obj);\n"
		"    return $node.to_string ();\n}\n\n";

	code.definition_ptr_ = "\n";
	code.definition_ptr_ += str;
	code.definition_ptr_ += " *$obj)\n"
		"{\n"
		"    return gson ($json, *$obj);\n"
		"}\n\n";
	str += " &$obj)";

	code.declare_ = str;
	code.declare_ += ";";

	str += "\n{\n";
	str += tab_;
	str += "acl::json_node &$node = $json.create_node();\n\n";

	/*
	if (check_nullptr(obj.string_ptr))
		node.add_null("string_ptr");
	else
		node.add_text("string_ptr", acl::get_value(obj.string_ptr));
	*/
	for (object_t::fields_t::const_iterator itr = obj.fields_.begin();
		itr != obj.fields_.end(); ++itr) {

		str += tab_;
		str += "if (check_nullptr($obj."+itr->name_+"))\n";
		str += tab_;
		str += tab_;
		str += "$node.add_null(\""+itr->name_+"\");\n";
		str += tab_;
		str += "else\n";
		str += tab_;
		str += tab_;
		str += "$node.";
		str += get_node_func(*itr);
		str += "(\"";
		str += itr->name_;
		str += "\", ";
		str += get_gson_func_laber(*itr);
		str += "$obj.";
		str += itr->name_;
		str += "));\n\n";
	}

	str += "\n";

	if (obj.fields_.empty()) {
		str += tab_;
		str += "(void) $obj;\n";
	}
	str += tab_;
	str += "return $node;\n}";

	code.definition_ = str;

	return code;
}

std::string gsoner::get_unpack_code(const std::string &obj_name,
	const field_t &field)const
{
	if (field.type_ == field_t::e_bool ||
		field.type_ == field_t::e_bool_ptr ||
		field.type_ == field_t::e_number ||
		field.type_ == field_t::e_ccstr ||
		field.type_ == field_t::e_cstr ||
		field.type_ == field_t::e_double ||
		field.type_ == field_t::e_string) {

		if (field.required_) {
			return tab_
				+ "if(!" + field.name_ + " ||"
				+ "!($result = gson(*" + field.name_ + ", &$obj."
				+ field.name_ + "), $result.first))\n" + tab_ + tab_
				+ "return std::make_pair(false, \"required ["
				+ obj_name + "." + field.name_
				+ "] failed:{\"+$result.second+\"}\");";
		} else {
			return tab_ +
				"if(" + field.name_ + ")\n" + tab_ + tab_ +
				"gson(*" + field.name_ + ", &$obj." + field.name_ + ");";
		}
	}
	if (field.required_) {
		return tab_
			+ "if(!" + field.name_ + " ||" + "!" + field.name_
			+ "->get_obj()||" + "!($result = gson(*" + field.name_
			+ "->get_obj(), &$obj." + field.name_ + "), $result.first))\n"
			+ tab_ + tab_ + "return std::make_pair(false, \"required ["
			+ obj_name + "." + field.name_
			+ "] failed:{\"+$result.second+\"}\");";
	}
	return tab_
		+ "if(" + field.name_ + "&& " + field.name_
		+ "->get_obj())\n" + tab_ + tab_ + " gson(*" + field.name_
		+ "->get_obj(), &$obj." + field.name_ + ");";
}
std::string gsoner::get_node_name(const std::string &name)
{
	return std::string(tab_ + "acl::json_node *")
		+ name + " = $node[\"" + name + "\"];";
}

gsoner::function_code_t gsoner::gen_unpack_code(const object_t &obj)
{
	std::list<std::string >node_names;
	std::list<std::string >unpack_codes;
	for (std::list<field_t>::const_iterator itr = obj.fields_.begin();
		itr != obj.fields_.end(); ++itr) {

		node_names.push_back(get_node_name(itr->name_));
		unpack_codes.push_back(get_unpack_code(obj.name_, *itr));
	}

	function_code_t code;
	std::string prefix =
		"std::pair<bool,std::string> gson(acl::json_node &$node, ";
	code.declare_ = prefix + obj.name_ + " &$obj);";
	code.declare_ptr_ = prefix + obj.name_ + " *$obj);";

	code.definition_ += prefix;
	code.definition_ += obj.name_ + " &$obj)\n{\n";
	for (std::list<std::string>::iterator itr = node_names.begin();
		itr != node_names.end(); ++itr) {

		code.definition_ += *itr;
		code.definition_ += "\n";
	}
	code.definition_ += tab_ + "std::pair<bool, std::string> $result;\n\n";
	for (std::list<std::string>::iterator itr = unpack_codes.begin();
		itr != unpack_codes.end(); ++itr) {

		code.definition_ += *itr;
		code.definition_ += "\n \n";
	}
	if (unpack_codes.empty()) {
		code.definition_ += tab_ + "(void) $node;\n";
		code.definition_ += tab_ + "(void) $obj;\n";
	}
	code.definition_ += tab_ + "return std::make_pair(true,\"\");\n}\n\n";

	code.definition_ptr_ += prefix + obj.name_ + " *$obj)";
	code.definition_ptr_ += "\n{\n" + tab_ + "return gson($node, *$obj);\n}\n\n";

	code.declare2_ = "std::pair<bool,std::string> "
	"gson(const acl::string &str, "+ obj.name_ +" &$obj);";
	code.definition2_ = " std::pair<bool,std::string> "
	"gson(const acl::string &$str, " + obj.name_ + " &$obj)\n"
		"{\n" 
		+ tab_ + "acl::json _json;\n"
		+ tab_ + "_json.update($str.c_str());\n"
		+ tab_ + "if (!_json.finish())\n"
		+ tab_ + "{\n" 
		+ tab_ + tab_ + "return std::make_pair(false, \"json not finish error\");\n"
		+ tab_ + "}\n"
		+ tab_ + "return gson(_json.get_root(), $obj);\n"
		"}\n\n";
	return code;
}

bool gsoner::check_use_namespace(void)
{
	//using namespace xxx;
	int pos = pos_;
	std::string token = codes_.substr(pos_, strlen("using"));
	if (token == "using") {
		pos_ += (int)strlen("using");
		if(next_token(default_delimiters_) == "namespace") {
			token = next_token(default_delimiters_ + ";");
			pos_++;//skip ;
			std::cout << "find:using namespace " << token << std::endl;;
			return true;
		}
	}
	pos_ = pos;
	return false;
}

bool gsoner::check_namespace(void)
{
	std::string temp = codes_.substr(pos_, strlen("namespace"));
	if (temp == "namespace") {
		pos_ += (int) strlen("namespace");
		std::string name = next_token(default_delimiters_+"{");
		namespaces_.push_back(name);
		pos_++; //skip {
		return true;
	}

	return false;
}

bool gsoner::check_namespace_end(void)
{
	if (namespaces_.size()) {
		namespaces_.pop_back();
		pos_++;
		return true;
	}

	return false;
}

std::string gsoner::next_token(std::string delimiters)
{
	std::string token;
	skip_space_comment();

	while (delimiters.find(codes_[pos_]) == std::string::npos) {
		if (codes_[pos_] == '/') {
check_again:
			skip_space();
			if(codes_[pos_] == '/') {
				if(!check_comment()) {
					throw syntax_error();
				}
				goto check_again;	
			}

			if(token.size()) {
				return token;
			}
		}

		token.push_back(codes_[pos_]);
		pos_++;
	}

	skip_space_comment();

	return token;
}

std::string gsoner::get_namespace(void)
{
	std::string result;
	for (std::list<std::string>::iterator itr = namespaces_.begin();
		itr != namespaces_.end(); ++itr) {

		result += *itr;
		result += "::";
	}

	return result;
}

bool gsoner::check_struct_begin(void)
{
	if(status_ != e_uninit) {
		return false;
	}

	std::string struct_laber = codes_.substr(pos_, strlen("struct"));
	std::string class_laber = codes_.substr(pos_, strlen("class"));

	//struct user_t
	if(struct_laber == "struct") {
		pos_ += (int) strlen("struct");
	} else if(class_laber == "class") {
		pos_ += (int) strlen("class");
	} else {
		return false;
	}

	status_ = e_struct_begin;
	std::string obj_name = next_token(default_delimiters_+ ":;{");
	current_obj_.name_ = get_namespace()+obj_name;

	if (codes_[pos_] == '{') {
		pos_++;
		return true;
	}

	if (codes_[pos_] == ';') {
		pos_++;
		//struct user_t ; end of struct;
		status_ = e_uninit;
		return true;
	}

	if (codes_[pos_] != ':') {
		return true;
	}

	pos_++;
	std::string token ;
	std::string level;

	while ((token = next_token(" \\\r\n\t,")) != "{") {
		if(codes_[pos_] == ',') {
			pos_++;
		}

		if (token == "public") {
			level = "public";
			continue;
		}

		if (token == "protect") {
			level = "protect";
			continue;
		}

		if (token == "private") {
			level = "private";
			continue;
		}

		level = "public";

		parent_obj_t parent;
		parent.level_ = get_level(level);
		parent.name_ = token;

		if (parent.level_ != parent_obj_t::e_protect &&
			parent.level_ != parent_obj_t::e_public) {

			continue;
		}

		std::map<std::string, object_t>::const_iterator
			itr = objs_.find(token);
		if (itr == objs_.end()) {
			itr = objs_.find(get_namespace()+token);
		}

		if (itr != objs_.end()) {
			for (std::list<field_t>::const_iterator 
				ii = itr->second.fields_.begin();
				ii != itr->second.fields_.end();
				++ii) {

				current_obj_.fields_.push_back(*ii);
			}

			current_obj_.parent_obj_.push_back(parent);
		}
	}

	return true;
}

bool gsoner::check_struct_end(void)
{
	if (status_ != e_struct_begin) {
		return false;
	}

	pos_++;
	skip_space_comment();
	if (codes_[pos_] == ';') {
		pos_++;
		if (current_obj_.name_.size()) {
			objs_.insert(std::make_pair(
				current_obj_.name_, current_obj_));
			current_obj_.reset();
		}

		status_ = e_uninit;
	}

	return true;
}

bool gsoner::skip_space_comment(void)
{
	bool result = false;
again:
	if (skip_space ()) {
		result = true;
	}

	char ch = codes_[pos_];
	if(ch == '/') {
		if (check_comment()) {
			result = true;
			goto again;
		} else {
			throw syntax_error();
		}
	}

	return result;
}

bool gsoner::check_include(void)
{
	std::string tmp = codes_.substr(pos_, strlen("#include"));
	if (tmp == "#include") {
		pos_ += (int) strlen("#include");
		skip_space_comment();
		char sym = codes_[pos_++];
		if(sym == '<') {
			sym = '>';
		}
		std::string include;
		while (codes_[pos_] != sym) {
			include.push_back(codes_[pos_]);
			pos_++;
		}
		pos_++;
		includes_.push_back(include);

		return true;
	}

	return false;
}

bool gsoner::check_comment(void)
{
	std::string commemt;
	bool result = false;
	if (codes_[pos_] == '/' && codes_[pos_ + 1] == '/') {
		result = true;
		pos_++;
		pos_++;
		//skip a line
		while (codes_[pos_] != '\n') {
			commemt.push_back(codes_[pos_]);
			pos_++;
		}
	} else if (codes_[pos_] == '/' && codes_[pos_ + 1] == '*') {
		result = true;
		//skip /**/comment
		pos_++;
		pos_++;
		while (codes_[pos_] != '*' || codes_[pos_ + 1] != '/') {
			commemt.push_back(codes_[pos_]);
			pos_++;
		}
		pos_++;
		pos_++;
	}

	if (!result) {
		return result;
	}

	if (commemt.find("Gson@optional") != std::string::npos) {
		required_ = false;
	}
	if (commemt.find("Gson@required") != std::string::npos) {
		required_ = true;
	}
	if (commemt.find("Gson@skip") != std::string::npos) {
		skip_ = true;
	}
	if (commemt.find("Gson@rename:") != std::string::npos) {
		std::size_t pos = commemt.find("Gson@rename:") + strlen("Gson@rename:");
		std::size_t n = 0;
		if (commemt[commemt.size() - 1] == '\n') {
			n++;
		}
		if (commemt[commemt.size() - 2] == '\r') {
			n++;
		}
		newname_ = commemt.substr( pos ,commemt.size() - pos - n);
		if (newname_.empty()) {
			std::cout << "Gson@rename:{} error, new name empty" << std::endl;
			throw syntax_error();
		}
	}
	return result;
}

std::string gsoner::get_static_string(const std::string &str, int &index)
{
	if (str[index] != '"') {
		return "";
	}

	index++;
	//int sym = 1;
	std::string lines;

	while (true) {
		if (str[index] == '\\') {
			if (str[index + 1] == '"') {
				lines.push_back('\\');
				lines.push_back('\"');
				index += 2;
				continue;
			}
		} else if (str[index] == '\"') {
			index++;
			skip_space_comment();
			if (str[index] == ';') {
				index++;
				break;
			} else if (str[index] == '\"') {
				index++;
				continue;
			} else {
				break;
			}
		}

		lines.push_back(str[index]);
		index++;
	}

	return lines;
}

bool gsoner::skip_space(void)
{
	bool result = false;
	while (codes_[pos_] == ' ' ||
		codes_[pos_] == '\r' ||
		codes_[pos_] == '\n' ||
		codes_[pos_] == '\t' ||
		codes_[pos_] == '\\') {

		pos_++;
		result = true;
	}

	return result;
}

std::pair<bool, std::string> gsoner::get_function_declare(void)
{
	if (status_ != e_struct_begin) {
		return std::make_pair(false, "");
	}

	int j = pos_;
	std::string lines;
	skip_space();

	while (true) {
		if (codes_[j] == '/') {
			if (!check_comment()) {
				throw syntax_error();
			}
			continue;
		}
		if (codes_[j] == '"') {
			std::string str = get_static_string(codes_,j);
		}
		if (codes_[j] == '=') {
			break;
		}
		if (codes_[j] == ';') {
			break;
		}
		if (codes_[j] == '(') {
			break;
		}
		lines.push_back(codes_[j]);
		j++;
	}

	if (codes_[j] != '(') {
		//not function, maybe member field
		return std::make_pair(false, "");
	}

	lines.push_back('(');
	j++;
	int syn = 1;

	while (true) {
		if (codes_[j] == '/') {
			if (!check_comment()) {
				throw syntax_error();
			}
			continue;
		}
		if (codes_[j] == ')') {
			syn--;
		}
		if (syn == 0) {
			lines.push_back(codes_[j]);
			break;
		}
		if (codes_[j] == '(') {
			syn++;
		}
		lines.push_back(codes_[j]);
		j++;
	}

	j++;
	pos_ = j;

	return std::make_pair(true, lines);
}

std::list<std::string> gsoner::get_initializelist(void)
{
	std::list<std::string> initialize_list;
	std::list<char> syms;
	std::string line;

	if (codes_[pos_] != ':') {
		return initialize_list;
	}

	pos_++;//skip ':'

	while (true) {
		skip_space_comment();
		if (codes_[pos_] == '(') {
			syms.push_back('(');
		} else if (codes_[pos_] == ')') {
			if (!syms.size() || syms.back() != '(') {
				throw syntax_error();
			}

			line.push_back (')');
			pos_++;

			syms.pop_back ();
			if (!syms.empty()) {
				continue;
			}

			initialize_list.push_back(line);
			line.clear();

			while (codes_[pos_] != ',' && codes_[pos_] != '{') {
				if (!skip_space_comment ()) {
					pos_++;
				}
			}

			if (codes_[pos_] == ',') {
				pos_++;
			}
			continue;
		} else if (codes_[pos_] == '{') {
			if (line.empty()) {
				break;
			}
			syms.push_back ('{');
		} else if (codes_[pos_] == '}') {
			if (!syms.size() || syms.back() != '{') {
				throw syntax_error();
			}

			line.push_back('}');
			pos_++;

			syms.pop_back();
			if (!syms.empty()) {
				continue;
			}

			initialize_list.push_back(line);
			line.clear();
			while (codes_[pos_] != ',' && codes_[pos_] != '{') {
				if (!skip_space_comment()) {
					pos_++;
				}
			}

			if (codes_[pos_] == ',') {
				pos_++;
			}

			continue;
		}

		line.push_back(codes_[pos_]);
		pos_++;
	}

	return initialize_list;
}

bool gsoner::check_function(void)
{
	if (status_ != e_struct_begin) {
		return false;
	}
	
	int function_begin = pos_;
	while (function_begin > 1 &&
		(codes_[function_begin-1] == '\r' ||
		codes_[function_begin-1] == '\n'||
		codes_[function_begin-1] == '\t'||
		codes_[function_begin-1] == ' ')) {

		function_begin --;
	}

	std::pair<bool, std::string> res = get_function_declare();
	if (!res.first) {
		return false;
	}

	skip_space_comment();
	//not find function define.just declare only.
	if (codes_[pos_] == ';') {
		pos_++;//skip ';' sym
		return true;
	}
	if (codes_[pos_] == ':') {
		std::list<std::string> initializelist = get_initializelist();
		if (initializelist.empty()) {
			throw syntax_error();
		}
		//what to do with this code.
	}
	if (codes_[pos_] == 'c' &&
		codes_[pos_+1] == 'o'&&
		codes_[pos_+2] == 'n'&&
		codes_[pos_+3] == 's'&&
		codes_[pos_+4] == 't') {

		pos_ += 5;
		res.second.append(" const");
		skip_space_comment();
	}
	pos_++;
	int sym = 1;
	std::string lines("{");

	while (true) {
		if (codes_[pos_] == '/') {
			if (!check_comment()) {
				throw syntax_error();
			}
			continue;
		}
		if (codes_[pos_] == '{') {
			sym++;
		}
		if (codes_[pos_] == '"') {
			std::string str = get_static_string(codes_, pos_);
			lines.push_back('"');
			lines += str;
			lines.push_back('"');
			continue;
		} else if (codes_[pos_] == '}') {
			sym--;
			if (sym == 0) {
				pos_++;
				lines.push_back('}');
				break;
			}
		}
		lines.push_back(codes_[pos_]);
		pos_++;
	}
	int function_end = pos_;

	std::cout <<current_obj_.name_ << std::endl;
	std::cout << codes_.substr(function_begin, function_end - function_begin) << std::endl;
	return true;
}

bool gsoner::check_member(void)
{
	//struct user_t{int id;  

	if (status_ != e_struct_begin) {
		return false;
	}

	std::string lines;
	skip_space();

	while (true) {
		if(codes_[pos_] == '/') {
			if(!check_comment()) {
				throw syntax_error();
			}
			continue;
		}

		if (codes_[pos_] == '"') {
			std::string str = get_static_string(codes_, pos_);
			lines.push_back('"');
			lines += str;
			lines.push_back('"');
		}

		if(codes_[pos_] == ';') {
			break;
		}
		lines.push_back(codes_[pos_]);
		pos_++;
	}
	std::cout << current_obj_.name_ << "     ";
	std::cout << lines << std::endl;
	//skip ;
	pos_++;

	//skip current field;
	if (skip_) {
		skip_ = false;
		return true;
	}
	std::string name;
	std::string types;
	// remove assignment =,
	if (lines.find('=') != std::string::npos) {
		lines = lines.substr(0, lines.find('='));
	}

	int e = (int) lines.size() - 1;
	while (lines[e] == ' ' ||
		lines[e] == '\r' ||
		lines[e] == '\n' ||
		lines[e] == '\t') {

		e--;
	}

	while (lines[e] != ' ' &&
		lines[e] != '\r' &&
		lines[e] != '\n' &&
		lines[e] != '\t' &&
		lines[e] != '*' &&
		lines[e] != '&' &&
		lines[e] != '=') {

		name.push_back(lines[e]);
		e--;
	}

	//get name
	std::reverse(name.begin(), name.end());

	types = lines.substr(0, e + 1);
	std::list<std::string> tokens;
	std::string token;
	for (std::string::iterator itr = types.begin();
		itr != types.end(); ++itr) {

		if (*itr == ' ' || *itr == '\r' || *itr == '\n' || *itr == '\t') {
			if (token.size()) {
				tokens.push_back(token);
				token.clear();
			}
		} else if (*itr == '*' || *itr == '&') {
			if (token.size()) {
				tokens.push_back(token);
				token.clear();
			}
			if (*itr == '*') {
				tokens.push_back("*");
			} else {
				tokens.push_back("&");
			}
		} else if (*itr == ':') {
			if (token.size()) {
				//back
				if (token[token.size() -1] == ':') {
					tokens.push_back("::");
					token.clear();
					continue;
				} else if (token.size()) {
					tokens.push_back(token);
					token.clear();
				}
			}

			token.push_back(':');
		} else if (*itr == '<') {
			if (token.size()) {
				tokens.push_back(token);
				token.clear();
			}
			tokens.push_back("<");
		} else if (*itr == '>') {
			if (token.size()) {
				tokens.push_back(token);
				token.clear();
			}
			tokens.push_back(">");
		} else if (*itr == ',') {
			if (token.size()) {
				tokens.push_back(token);
				token.clear();
			}
			tokens.push_back(",");
		} else {
			token.push_back(*itr);
		}
	}

	if (token.size()) {
		tokens.push_back(token);
	}

	if (tokens.size() == 0) {
		printf("\"%s\"[syntax error]", name.c_str());
		assert(false);
	}

	//check newname
	if (newname_.size()) {
		name = newname_;
	}

	newname_.clear();
	//std    :: list <int> a;
	std::string first = tokens.front();
	if (first == "const") {
		tokens.pop_front();
		//std::string first = tokens.front();
		first = tokens.front();
		if (tokens.empty()) {
			throw unsupported_type(("unsupported \"" 
				+ lines + "\"").c_str());
		}
	}

	if (first.find("char") != std::string::npos) {
		if (first == "char*") {
			field_t f;
			f.name_ = name;
			f.required_ = required_;
			f.type_ = field_t::e_cstr;
			current_obj_.fields_.push_back(f);

			return true;
		}

		tokens.pop_front();
		if (!tokens.size()) {
			throw unsupported_type("unsupported 'char' type");
		}

		first = tokens.front();
		if (first == "*") {
			field_t f;
			f.name_ = name;
			f.required_ = required_;
			f.type_ = field_t::e_cstr;
			current_obj_.fields_.push_back(f);

			return true;
		}
	}

	if (first == "std") {
		for (std::list<std::string>::iterator itr = tokens.begin();
			itr != tokens.end(); ++itr) {

			if (itr->find("string") != std::string::npos) {
				field_t f;
				f.name_ = name;
				f.required_ = required_;
				f.type_ = field_t::e_string;
				current_obj_.fields_.push_back(f);

				return true;
			} else if (itr->find("list") != std::string::npos) {
				field_t f;
				f.name_ = name;
				f.required_ = required_;
				f.type_ = field_t::e_list;
				current_obj_.fields_.push_back(f);

				return true;;
			} else if (itr->find("vector") != std::string::npos) {
				field_t f;
				f.name_ = name;
				f.required_ = required_;
				f.type_ = field_t::e_vector;
				current_obj_.fields_.push_back(f);

				return true;;
			} else if (itr->find("map") != std::string::npos) {
				field_t f;
				f.name_ = name;
				f.required_ = required_;
				f.type_ = field_t::e_map;
				current_obj_.fields_.push_back(f);

				return true;;
			} else if(itr->find("set") != std::string::npos) {
				field_t f;
				f.name_ = name;
				f.required_ = required_;
				f.type_ = field_t::e_set;
				current_obj_.fields_.push_back(f);

				return true;;
			}
		}
	} else if (first.find("acl") != std::string::npos) {
		for (std::list<std::string>::iterator itr = tokens.begin();
			itr != tokens.end(); ++itr) {

			if (itr->find("string") != std::string::npos) {
				field_t f;
				f.name_ = name;
				f.required_ = required_;
				f.type_ = field_t::e_string;
				current_obj_.fields_.push_back(f);

				return true;
			}
		}

		throw syntax_error();
	} else if (first == "unsigned"||
		first == "signed" ||
		first == "int" ||
		first == "long" ||
		first == "size_t" ||
		first == "ssize_t" ||
		first == "short" ||
		first == "int16_t"||
		first == "uint16_t" ||
		first == "uint32_t" ||
		first == "int32_t" ||
		first == "int64_t" ||
		first == "uint64_t") {

		field_t f;
		f.type_ = field_t::e_number;
		f.name_ = name;
		f.required_ = required_;
		current_obj_.fields_.push_back(f);

		return true;
	} else if (first == "bool") {
		field_t f;
		f.type_ = field_t::e_bool;
		f.name_ = name;
		f.required_ = required_;
		current_obj_.fields_.push_back(f);

		return true;
	}
	else if (first == "float" || first == "double") {
		field_t f;
		f.type_ = field_t::e_double;
		f.name_ = name;
		f.required_ = required_;
		current_obj_.fields_.push_back(f);

		return true;
	} else {
		// user define class ,struct.
		field_t f;
		f.name_ = name;
		f.required_ = required_;
		f.type_ = field_t::e_object;
		current_obj_.fields_.push_back(f);

		return true;
	}

	return true;
}

bool gsoner::read_file(const char *filepath)
{
	std::ifstream is(filepath, std::ifstream::binary);
	if (!is) {
		return false;
	}

	std::string str((std::istreambuf_iterator<char>(is)),
			std::istreambuf_iterator<char>());
	codes_.append(str);

	files_.push_back(get_filename(filepath));

	return true;
}

std::string gsoner::get_filename(const char *filepath)
{
	std::string  filename;
	int i = (int) strlen(filepath) - 1;

	while (i >= 0 && (filepath[i] != '\\' || filepath[i] != '/')) {
		filename.push_back(filepath[i]);
		i--;
	}

	std::reverse(filename.begin(), filename.end());

	return filename;
}

bool gsoner::read_multi_file(const std::vector<std::string>& files)
{
	for (std::vector<std::string>::const_iterator itr = files.begin();
		itr != files.end(); itr++) {

		if (!read_file(itr->c_str())) {
			std::cout << "read_file:" \
				<< itr->c_str() << " error" << std::endl;
			return false;
		}
	}

	return true;
}

void gsoner::parse_code(void)
{
	//char c = '\n';
	max_pos_ = (int) codes_.size();
	try
	{
		do
		{
			skip_space_comment();
			if(pos_ == max_pos_) {
				break;
			}

			char ch = codes_[pos_];
			if (ch == ';') {
				pos_++;
				continue;
			}
			if (ch == '/') {
				if (check_comment()) {
					continue;
				}
			}
			if (ch == '}') {
				if(check_struct_end()) {
					continue;
				}
				if(check_namespace_end()) {
					continue;
				}
			}
			if (ch == 'n') {
				if(check_namespace()) {
					continue;
				}
			}
			if (ch == '#') {
				if(check_include()) {
					continue;
				}
				if(check_define()) {
					continue;
				}
				if(check_pragma()) {
					continue;
				}
			}
			if (ch == 's') {
				if(check_struct_begin()) {
					continue;
				}
			}
			if (ch == 'u') {
				if (check_use_namespace()) {
					continue;
				}
			}
			if(check_function()) {
				continue;
			}
			if(check_member()) {
				required_ = default_;
				continue;
			}

			printf("%c", codes_[pos_]);
			pos_++;
		} while (pos_ < max_pos_);
	}
	catch (syntax_error &e) {
		(void) e;

		int count = 2;
		std::size_t ii = pos_;
		int start = 0;

		while (ii > 0) {
			if (codes_[ii] == '\n') {
				count--;
				if (count == 0) {
					break;;
				}
			}
			ii--;
		}
		ii++;
		start = (int) ii;
		ii = pos_;
		count = 2;
		while (ii < codes_.size()) {
			if (codes_[ii] == '\n') {
				count--;
				if (count == 0) {
					break;;
				}
			}
			ii++;
		}

		std::cout << codes_.substr(start, ii - start).c_str() << std::endl;
		ii = 0;
		count = 0;
		while (ii < (std::size_t) pos_) {
			if (codes_[ii] == '\n') {
				count++;
			}
			ii++;
		}

		std::cout << "line:" << count << std::endl;
		return;
	} catch (std::exception & e) {
		std::cout << e.what() << std::endl << std::endl;
		return;
	}
}

std::string gsoner::get_include_files(void)
{
	std::string str;
	for (std::list<std::string>::const_iterator itr = files_.begin();
		itr != files_.end(); ++itr) {

		str += "#include \"";
		str += *itr;
		str += "\"\n";
	}

	return str;
}
static std::string get_filename_without_ext(std::string filename)
{
	std::string result;

	if (filename.empty()) {
		return std::string(); 
	}
	
	std::size_t pos = filename.find_last_of('.');
	if (pos != filename.npos) {
		filename = filename.substr(0, pos);
	}
	
	for(int i = (int)filename.size() -1; i >= 0; --i) {
		char ch = filename[i];
		if (ch == '/' || ch == '\\') {
			break;
		}
		result += filename[i];
	}
	if (result.empty()) {
		return std::string();
	}
	std::reverse(result.begin(), result.end());
	return result;
}
void gsoner::gen_gson(void)
{
	const char *namespace_start = "namespace acl\n{";
	const char *namespace_end = "\n}///end of acl.\n";

	if(files_.size() == 1) {
		std::string filename = get_filename_without_ext(files_.front());
		gen_header_filename_ = filename + ".gson.h";
		gen_source_filename_ = filename + ".gson.cpp";
	}
	write_source("#include \"stdafx.h\"\n");
	write_source(get_include_files());
	write_source("#include \"" + gen_header_filename_ + "\"\n");
//	write_source("#include \"acl_cpp/serialize/gson_helper.ipp\"\n");

	write_header(namespace_start);
	write_source(namespace_start);

	for (std::map<std::string, object_t>::iterator itr = objs_.begin();
		itr != objs_.end(); ++itr) {

		function_code_t pack = gen_pack_code(itr->second);
		function_code_t unpack = gen_unpack_code(itr->second);

		write_header(('\n' + tab_ + "//" + itr->second.name_));
		write_header(('\n' + tab_ + pack.declare2_));
		write_header(('\n' + tab_ + pack.declare_));
		write_header(('\n' + tab_ + pack.declare_ptr_));
		write_header('\n'  + tab_ + unpack.declare_);
		write_header('\n'  + tab_ + unpack.declare_ptr_);
		write_header('\n'  + tab_ + unpack.declare2_+"\n");

		write_source(add_4space(pack.definition_));
		write_source(add_4space(pack.definition_ptr_));
		write_source(add_4space(pack.definition2_));
		write_source(add_4space(unpack.definition_));
		write_source(add_4space(unpack.definition_ptr_));
		write_source(add_4space(unpack.definition2_));
	}

	write_header(namespace_end);
	write_source(namespace_end);
	flush();
}

std::string gsoner::add_4space(const std::string &code)
{
	std::string result;
	result += '\n';
	result += tab_;
	std::string tmp;

	int len = (int) code.size();
	int i = 0;
	bool end = false;
	int syn = 0;

	while (i < len) {
		if (code[i] == '{') {
			syn++;
		}

		if (code[i] == '}') {
			syn--;
			if (syn == 0) {
				end = true;
			}
		}
		result.push_back(code[i]);

		if (end == false &&
			code[i] == '\n'&&
			code[i + 1] != '\n'&&
			code[i + 1] != '\r') {

			result += tab_;
		}

		i++;
	}

	return result;
}

void gsoner::flush(void)
{
	gen_header_->flush();
	gen_source_->flush();
	delete gen_header_;
	delete gen_source_;
}

void gsoner::write_header(const std::string &data)
{
	if (gen_header_ == NULL) {
		gen_header_ = new std::ofstream(gen_header_filename_.c_str());
	}
	gen_header_->write(data.c_str(), data.size());
}

void gsoner::write_source(const std::string &data)
{
	if (gen_source_ == NULL) {
		gen_source_ = new std::ofstream(gen_source_filename_.c_str());
	}
	gen_source_->write(data.c_str(), data.size());
}

bool gsoner::check_define(void)
{
	std::string lines;
	std::string tmp = codes_.substr(pos_, strlen("#define "));
	if (tmp != "#define ") {
		return false;
	}

	pos_ += (int) strlen("#define ");
	bool skip = false;
	while (true) {
		if (codes_[pos_] == '\r' && codes_[pos_+1] == '\n') {
			pos_++;
			pos_++;
			if (skip == true) {
				skip = false;
			} else {
				return true;
			}
		} else if (codes_[pos_] == '\n') {
			pos_++;
			if (skip == true) {
				skip = false;
			} else {
				return true;
			}
		} else if (codes_[pos_] == '\\') {
			pos_++;
			skip = true;
		} else {
			lines.push_back(codes_[pos_]);
			pos_++;
		}
	}

	return false;
}

bool gsoner::check_pragma(void)
{
	std::string lines;
	std::string tmp = codes_.substr(pos_, strlen("#pragma "));
	if (tmp != "#pragma ") {
		return false;
	}

	pos_ += (int) strlen("#pragma ");
	bool skip = false;
	while (true) {
		if (codes_[pos_] == '\r' && codes_[pos_ + 1] == '\n') {
			pos_++;
			pos_++;
			if (skip == true) {
				skip = false;
			} else {
				return true;
			}
		}
		if (codes_[pos_] == '\n') {
			pos_++;
			if (skip == true) {
				skip = false;
			} else {
				return true;
			}
		}
		if (codes_[pos_] == '\\') {
			pos_++;
			skip = true;
		}
		lines.push_back(codes_[pos_]);
		pos_++;
	}

	return false;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
