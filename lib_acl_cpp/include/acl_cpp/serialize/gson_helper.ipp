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
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/json.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

///////////////////////////////////type_traits////////////////////////

struct true_type
{
	static const bool value = true;
};

struct false_type
{
	static const bool value = false;
};

template<class T>
struct remove_const
{
	typedef T type;
};

template<class T>
struct remove_const<const T>
{
	typedef T type;
};

template<class T>
struct remove_volatile
{
	typedef T type;
};

template<class T>
struct remove_volatile<volatile T>
{
	typedef T type;
};

template<class T>
struct remove_cv
{
	typedef typename remove_const
		<typename remove_volatile<T>::type>::type type;
};

//remove_pointer
template<class T>
struct remove_pointer
{
	typedef T type;
};

template<class T>
struct remove_pointer<T *>
{
	typedef T type;
};

template<class T>
struct remove_pointer<T *const>
{
	typedef T type;
};

template<class T>
struct remove_pointer<T *volatile>
{
	typedef T type;
};

template<class T>
struct remove_pointer<T *const volatile>
{
	typedef T type;
};

template<bool ,
	class T = void>
	struct enable_if
{
};

template<class T>
struct enable_if<true, T>
{	
	typedef T type;
};

//_Is_number
template<class T>
struct _Is_number : false_type
{
};

template<>
struct _Is_number<unsigned short> : true_type
{
};

template<>
struct _Is_number<signed short> : true_type
{
};

template<>
struct _Is_number<unsigned int> : true_type
{
};

template<>
struct _Is_number<signed int> : true_type
{
};

template<>
struct _Is_number<unsigned long> : true_type
{
};

template<>
struct _Is_number<signed long> : true_type
{
};

template<>
struct _Is_number<long long> : true_type
{
};

template<>
struct _Is_number<unsigned long long> : true_type
{
};

template<class T>
struct is_number : _Is_number<typename remove_cv<T>::type>
{
};

//string
template<class T>
struct _Is_string : false_type
{
};

template<>
struct _Is_string<std::string> : true_type
{
};

template<>
struct _Is_string<acl::string> : true_type
{
};

template<class T>
struct is_string : _Is_string<typename remove_cv<T>::type>
{
};

//double
template<class T>
struct _Is_double : false_type
{
};

template<>
struct _Is_double<float> : true_type
{
};

template<>
struct _Is_double<double> : true_type
{
};

template<>
struct _Is_double<long double> : true_type
{
};

template <class T>
struct is_double : _Is_double<typename remove_const<T>::type>
{
};

//char ptr. c string in cpp
template<class T>
struct _Is_char_ptr : false_type
{
};

template<>
struct _Is_char_ptr<char*> : true_type
{
};

template<class T>
struct is_char_ptr :_Is_char_ptr<typename remove_cv<T>::type>
{
};

template<class T>
struct _Is_bool : false_type
{
};

template<>
struct _Is_bool<bool> : true_type
{
};

template<class T>
struct is_bool : _Is_bool<typename remove_cv<T>::type>
{
};

template<bool T>
struct _Is_object :false_type
{
};

template<>
struct _Is_object<true> : true_type
{
};

template<class T>
struct is_object : _Is_object<
	!is_string<T>::value &&
	!is_double<T>::value &&
	!is_number<T>::value &&
	!is_bool<T>::value &&
	!is_char_ptr<T>::value>
{
};

template <class T>
static inline bool check_nullptr(T&)
{
	return false;
}

template<class T>
static inline bool check_nullptr(T *t)
{
	if (t == NULL)
		return true;
	return false;
}

//acl::string ,std::string
template<class T>
typename enable_if<is_string<T>::value, const char *>::type
static inline get_value(const T &value)
{
	return value.c_str();
}

template<class T>
typename enable_if<is_string<T>::value, const char *>::type
static inline get_value(const T *value)
{
	return value->c_str();
}

//char *,const char *
static inline const char *get_value(const char *value)
{
	return value;
}

//bool 
static inline bool get_value(const bool value)
{
	return value;
}

static inline bool get_value(const bool* value)
{
	return *value;
}

//number
template <class T>
typename enable_if<is_number<T>::value, T>::type
static inline get_value(const T t)
{
	return t;
}

// number pointor -> number
// eg: int * -> int .
template <class T>
typename enable_if<is_number<T>::value, T>::type
static inline get_value(const T *t)
{
	return *t;
}

template <class T>
typename enable_if<is_double<T>::value, T>::type
static inline get_value(const T &t)
{
	return t;
}

template <class T>
typename enable_if<is_double<T>::value, T>::type
static inline get_value(const T *t)
{
	return *t;
}

// obj
template<class T>
typename enable_if<is_object<T>::value, void>::type
static inline add_item(acl::json &json, acl::json_node &node, const T &obj)
{
	if(check_nullptr(obj))
		node.add_array_null();
	else
		node.add_child(gson(json, obj));
}

// number
template<class T>
typename enable_if<is_number<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T value)
{
	node.add_array_number(get_value(value));
}

template<class T>
typename enable_if<is_number<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T *value)
{
	if (check_nullptr(value))
		node.add_array_null();
	else
		node.add_array_number(get_value(value));
}

template<class T>
typename enable_if<is_double<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T value)
{
	node.add_array_double(get_value(value));
}

template<class T>
typename enable_if<is_double<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T* value)
{
	if(check_nullptr(value))
		node.add_array_null();
	else
		node.add_array_double(get_value(value));
}

//bool 
template<class T>
typename enable_if<is_bool<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T *value)
{
	if (check_nullptr(value))
		node.add_array_null();
	else
		node.add_array_bool(get_value(value));
}

template<class T>
typename enable_if<is_bool<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T value)
{
	node.add_array_bool(get_value(value));
}

template<class T>
typename enable_if<is_string<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T value)
{
	node.add_array_text(get_value(value));
}

template<class T>
typename enable_if<is_string<T>::value, void>::type
static inline add_item(acl::json &, acl::json_node &node, T *value)
{
	if(check_nullptr(value))
		node.add_array_null();
	else
		node.add_array_text(get_value(value));
}

static inline void add_item(acl::json &, acl::json_node &node, char *value)
{
	if(check_nullptr(value))
		node.add_array_null();
	else
		node.add_array_text(value);
}

template<class V>
static inline acl::json_node &gson(acl::json &json, const std::list<V> &objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::list<V>::const_iterator
		itr = objects.begin(); itr != objects.end(); ++itr)
	{
		add_item(json, node, *itr);
	}

	return node;
}

template<class T>
static inline acl::json_node &gson(acl::json &json, const std::list<T> *objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::list<T>::const_iterator
		itr = objects->begin(); itr != objects->end(); ++itr)
	{
		add_item(json, node, *itr);
	}

	return node;
}

template<class T>
static inline acl::json_node &gson(acl::json &json,
	const std::vector<T> &objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::vector<T>::const_iterator
		itr = objects.begin(); itr != objects.end(); ++itr)
	{
		add_item(json, node, *itr);
	}

	return node;
}

template<class T>
static inline acl::json_node &gson(acl::json &json,
	const std::vector<T> *objects)
{
	return gson(json, *objects);
}

//define number map
template<class K, class V>
typename enable_if<is_number<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<K, V> &objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<K, V>::const_iterator
		itr = objects.begin(); itr != objects.end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

//define number map
template<class K, class V>
typename enable_if< is_number<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<K, V> *objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<K, V>::const_iterator
		itr = objects->begin(); itr != objects->end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

//define floating map
template<class K, class V>
typename enable_if<is_double<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<K, V> &objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<K, V>::const_iterator
		itr = objects.begin(); itr != objects.end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

template<class K, class V>
typename enable_if<is_double<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<K, V> *objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<K, V>::const_iterator
		itr = objects->begin(); itr != objects->end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

//define bool map
template<class K, class V>
typename enable_if<is_bool<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<K, V> &objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<K, V>::const_iterator itr = objects;
		itr != objects.end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

template<class K, class V>
typename enable_if<is_string<V>::value
	|| is_char_ptr<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<K, V> & objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<K, V>::const_iterator
		itr = objects.begin(); itr != objects.end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

template<class T, class V>
typename enable_if<is_object<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<T, V> &objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<T, V>::const_iterator
		itr = objects.begin(); itr != objects.end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

template<class T, class V>
typename enable_if<is_object<V>::value, acl::json_node &>::type
static inline gson(acl::json &json, const std::map<T, V> *objects)
{
	acl::json_node &node = json.create_array();
	for (typename std::map<T, V>::const_iterator
		itr = objects->begin(); itr != objects->end(); ++itr)
	{
		const char *tag = get_value(itr->first);
		if (check_nullptr(itr->second))
			node.add_child(json.create_node().add_null(tag));
		else
		{
			acl::json_node &item = gson(json, itr->second);
			node.add_child(json.create_node().add_child(tag, item));
		}
	}

	return node;
}

//////////////////////////////////////////////////////////////////////////////

template <class T>
typename enable_if<is_object<T>::value,
	std::pair<bool, std::string> >::type
static inline gson(acl::json_node &node, T **obj);

template<class T>
static inline void del(T **obj)
{
	delete *obj;
	*obj = NULL;
}

template<class T>
static inline void del(T *obj)
{
	(void) obj;
}

//bool
static inline std::pair<bool, std::string> gson(acl::json_node &node, bool *obj)
{
	if (node.is_bool() == false)
		return std::make_pair(false, "get bool failed");

	*obj = *node.get_bool();
	return std::make_pair(true, "");
}

static inline std::pair<bool, std::string> gson(acl::json_node &node, bool **obj)
{
	*obj = NULL;
	if (node.is_bool() == false)
		return std::make_pair(false, "get bool failed");

	*obj = new bool;
	**obj = *node.get_bool();

	return std::make_pair(true, "");
}

//double
template <class T>
typename enable_if<is_double<T>::value,
	std::pair<bool, std::string> >::type
static inline gson(acl::json_node &node, T *obj)
{
	if (node.is_double() == false)
		return std::make_pair(false, "get double failed");

	*obj = static_cast<T>(*node.get_double());
	return std::make_pair(true, "");
}

template <class T>
typename enable_if<is_double<T>::value,
	std::pair<bool, std::string> >::type
static inline gson(acl::json_node &node, T **obj)
{
	*obj = NULL;
	if (node.is_double() == false)
		return std::make_pair(false, "get double failed");;

	*obj = new T;
	**obj = static_cast<T>(*node.get_double());

	return std::make_pair(true, "");
}

//intergral
template <class T>
typename enable_if<is_number<T>::value,
	 std::pair<bool, std::string> >::type
static inline gson(acl::json_node &node, T *obj)
{
	if (node.is_number() == false)
		return std::make_pair(false, "get number failed");

	*obj = static_cast<T>(*node.get_int64());
	return std::make_pair(true, "");
}

template <class T>
typename enable_if<is_number<T>::value,
	 std::pair<bool, std::string> >::type
static inline gson(acl::json_node &node, T **obj)
{
	*obj = NULL;
	if (node.is_number() == false)
		return std::make_pair(false, "get number failed");;

	*obj = new T;
	**obj = static_cast<T>(*node.get_int64());

	return std::make_pair(true, "");
}

//string
static inline std::pair<bool, std::string> gson(acl::json_node &node, char **obj)
{
	*obj = NULL;
	if (node.is_string() == false)
		return std::make_pair(false, "get char * string failed");

	int len = strlen(node.get_string());
	*obj = new char[len + 1];
	memcpy(*obj, node.get_string(), len);
	(*obj)[len] = 0;

	return std::make_pair(true, "");
}

static inline std::pair<bool, std::string> 
	gson(acl::json_node &node, acl::string *obj)
{
	if (node.is_string() == false)
		return std::make_pair(false, "get string failed");

	obj->append(node.get_string());
	return std::make_pair(true, "");
}

static inline std::pair<bool, std::string>
	gson(acl::json_node &node, acl::string **obj)
{
	*obj = NULL;
	if (node.is_string() == false)
		return std::make_pair(false, "get string failed");

	*obj = new acl::string;
	(*obj)->append(node.get_string());

	return std::make_pair(true, "");
}

static inline std::pair<bool, std::string>
	gson(acl::json_node &node, std::string *obj)
{
	if (node.is_string() == false)
		return std::make_pair(false, "get string failed");

	obj->append(node.get_string());
	return std::make_pair(true, "");
}

static inline std::pair<bool, std::string>
	gson(acl::json_node &node, std::string **obj)
{
	*obj = NULL;
	if (node.is_string() == false)
		return std::make_pair(false, "get string failed");

	*obj = new std::string;
	(*obj)->append(node.get_string());

	return std::make_pair(true, "");
}

template<class T>
static inline std::pair<bool, std::string>
	gson(acl::json_node &node, std::list<T> *objs)
{
	std::pair<bool, std::string> result;
	std::string error_string;
	acl::json_node *itr = node.first_child();

	while (itr)
	{
		T obj;
		result = gson(*itr, &obj);
		if (result.first)
			objs->push_back(obj);
		else
			error_string.append(result.second);
		itr = node.next_child();
	}

	return std::make_pair(!!!objs->empty(), error_string);
}

//vector
template<class T>
std::pair<bool, std::string>
static inline gson(acl::json_node &node, std::vector<T> *objs)
{
	std::pair<bool, std::string> result;
	acl::json_node *itr = node.first_child();
	std::string error_string;

	while (itr)
	{
		T obj;
		result = gson(*itr, &obj);
		if (result.first)
			objs->push_back(obj);
		else
			error_string.append(result.second);
		itr = node.next_child();
		//todo delete obj when failed
	}

	return std::make_pair(!!!objs->empty(), error_string);
}

template <class T>
typename enable_if<is_object<T>::value,
	std::pair<bool, std::string> >::type
static inline gson(acl::json_node &node, T **obj)
{
	*obj = new T();
	std::pair<bool, std::string> result = gson(node, *obj);
	if (result.first == false)
	{
		delete *obj;
		*obj = NULL;
	}

	return result;
}

///////////////////////////////////////////map////////////////////////////////

//int map

template<class K, class T>
typename enable_if<
	is_string<T>::value||
	is_bool<T>::value||
	is_number<T>::value||
	is_double<T>::value||
	is_char_ptr<T>::value,
	std::pair<bool, std::string> >::type
static inline expand(acl::json_node &node, std::map<K, T> *objs)
{
	std::pair<bool, std::string> result;
	acl::json_node *itr = node.first_child();
	while (itr)
	{
		T obj;
		result = gson(*itr, &obj);
		if (!result.first)
			break;

		objs->insert(std::make_pair(K(itr->tag_name()), obj));
		itr = node.next_child();
	}

	if (result.first)
		return std::make_pair(true, "");

	for (typename std::map<K, T>::iterator it = objs->begin();
		it != objs->end(); ++it)
	{
		del(&it->second);
	}

	objs->clear();
	return result;
}

template<class K, class T>
typename enable_if <is_object<T>::value ,
	std::pair<bool, std::string > > ::type
static inline expand(acl::json_node &node, std::map<K, T> *objs)
{
	std::pair<bool, std::string> result;
	acl::json_node *itr = node.first_child();

	while (itr && itr->get_obj())
	{
		T obj;
		result = gson(*(itr->get_obj()), &obj);
		if (!result.first)
			break;

		objs->insert(std::make_pair(K(itr->tag_name()), obj));
		itr = node.next_child();
	}

	if (result.first)
		return std::make_pair(true, "");

	for (typename std::map<K, T>::iterator itr2 = objs->begin();
		itr2 != objs->end(); ++itr2)
	{
		del(&itr2->second);
	}

	objs->clear();
	return result;
}

//map
template<class K, class V>
std::pair<bool, std::string>
static inline gson(acl::json_node &node, std::map<K, V> *objs)
{
	std::pair<bool, std::string> result;
	acl::json_node *itr = node.first_child();
	std::string error_string;

	while (itr)
	{
		result = expand(*itr, objs);
		if (result.first == false)
			error_string.append(result.second);
		itr = node.next_child();
	}

	return std::make_pair(!!!objs->empty(), error_string);
}

} // namespace acl
