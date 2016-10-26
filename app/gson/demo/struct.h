#pragma once

struct base
{
	std::string string ;
	//Gson@optional
	std::string *string_ptr ;
	int a  ;
	int *a_ptr ;
	unsigned int b ;
	unsigned int *b_ptr;
	int64_t c ;
	int64_t *c_ptr ;
	unsigned long d;
	unsigned long *d_ptr ;
	unsigned long long e ;
	unsigned long long *e_ptr;
	long f ;
	long *f_ptr ;
	long long g;
	
	long long *g_ptr ;
	acl::string acl_string;
	acl::string *acl_string_ptr;

	float h;
	float *h_ptr;
	double i;
	double *i_ptr;
};

struct list1
{
	base b;
	base *b_ptr;
	std::list<base> bases_list;
	std::list<base> *bases_list_ptr;
	//Gson@optional
	std::list<base*> *bases_ptr_list_ptr;
	std::map<std::string,base> base_map;
	std::map<std::string,std::list<base> > base_list_map;
	std::vector<std::string> vector_string;
	std::vector<std::list<base> > vector_list_base;
};

namespace hello
{
struct world : list1
{
int me;
};
}
