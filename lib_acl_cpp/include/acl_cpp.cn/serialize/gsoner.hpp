/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   E-mail: niukey@qq.com
 * 
 * VERSION
 *   Sat 08 Oct 2016 09:07:14 PM CST
 */

#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <map>
#include <vector>
#include <string>

#ifndef ACL_CLIENT_ONLY

namespace acl
{

class ACL_CPP_API gsoner
{
public:
	gsoner ();
	bool read_file(const char *filepath);
	bool read_multi_file(const std::vector<std::string>& files);
	void parse_code();
	void gen_gson();
	void set_default_required();
	void set_default_optional();
	void set_header_filename(const std::string &filename);
	void set_source_filename(const std::string &filename);

private:
	enum code_parser_status_t
	{
		e_uninit,
		e_comment,
		e_struct_begin,
		e_struct_end,
	};

	struct field_t 
	{
		enum type_t
		{
			e_bool,
			e_bool_ptr,
			e_number,
			e_double,
			e_cstr,		//char *
			e_ccstr,	//const char *
			e_string,
			e_list,
			e_vector,
			e_map,
			e_set,
			e_object,
		};

		type_t type_;
		std::string name_;
		bool required_;

		field_t();
		field_t(type_t t, const std::string &name,bool required);

		virtual ~field_t();
	};

	struct parent_obj_t
	{
		enum level_t
		{
			e_error = 0,
			e_public,
			e_protect,
			e_private,
		};
		level_t level_;
		std::string name_;
	};

	struct object_t
	{
		typedef std::list<field_t> fields_t;
		fields_t fields_;
		std::string name_;
		std::list<parent_obj_t> parent_obj_;
		void reset ();
	};

	struct function_code_t
	{
		std::string declare_;
		std::string declare_ptr_;
		std::string declare2_;
		std::string definition_;
		std::string definition_ptr_;
		std::string definition2_;
	};

	parent_obj_t::level_t get_level(std::string str);
	std::string get_node_func (const field_t &field);
	std::string get_gson_func_laber (const field_t &field);
	function_code_t gen_pack_code (const object_t &obj);
	std::string get_unpack_code(const std::string &obj_name, 
		const field_t &field)const ;
	std::string get_node_name(const std::string &name);
	std::string next_token(std::string delimiters);
	std::string get_namespace();
	function_code_t gen_unpack_code(const object_t &obj);
	std::string get_static_string(const std::string &str, int &index);
	std::string get_include_files();
	std::string get_filename(const char *filepath);

	bool skip_space_comment();

	bool check_use_namespace();
	bool check_namespace();
	bool check_namespace_end();
	bool check_struct_begin ();
	bool check_struct_end ();
	bool check_include();
	bool check_comment ();
	bool check_function();
	bool check_member();
	bool skip_space ();
	std::pair<bool, std::string> get_function_declare();

	std::string add_4space(const std::string &code);
	void flush();
	void write_header(const std::string &data);
	void write_source(const std::string &data);

	bool check_define();
	bool check_pragma();
	std::list<std::string> get_initializelist ();
	//char cc;
	int pos_;
	int max_pos_;
	std::string comment_begin_;
	std::string comment_end_;
	std::string codes_;
	code_parser_status_t status_;
	std::string tab_ ;
	bool required_;
	bool skip_;
	bool default_;
	std::string newname_;
	object_t current_obj_;
	std::map<std::string,object_t> objs_;
	std::list<std::string> namespaces_;
	std::list<std::string> includes_;
	std::list<std::string> files_;
	std::ofstream *gen_header_;
	std::ofstream *gen_source_;
	std::string gen_header_filename_;
	std::string gen_source_filename_;
	std::string default_delimiters_;
};

}//end of acl

#endif // ACL_CLIENT_ONLY
