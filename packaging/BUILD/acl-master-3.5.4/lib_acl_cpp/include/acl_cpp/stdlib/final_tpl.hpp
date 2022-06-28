#pragma once
#include "../acl_cpp_define.hpp"

namespace acl {

#if defined(_WIN32) || defined(_WIN64)
template<typename TDerive, typename TProvider>
class final_tpl_base
{
	friend TDerive;
	friend TProvider;
private:
	final_tpl_base() {}
	~final_tpl_base() {}
};
#else
template <typename T>
class identity
{
public:
	typedef T me;
};

template<typename TDerive, typename TProvider>
class final_tpl_base
{
	friend class identity<TDerive>::me;
	friend class identity<TProvider>::me;
private:
	final_tpl_base() {}
	~final_tpl_base() {}
};
#endif

/*
 * 提供禁止派生的功能,需要此功能的类可以从final_tpl派生,
 * 并将类名作为模板参数传递
 * @example:
 * class my_final_class : public acl::final_tpl <my_final_class>
 * {
 * public:
 *   my_final_class() {}
 *   ~my_final_class() {}
 * }
 * 这样就保证了 my_final_class 是不能被继承的
 */
template<typename TFinalClass>
class final_tpl : virtual public final_tpl_base<TFinalClass,
	final_tpl<TFinalClass> >
{
public:
	final_tpl() {}
	~final_tpl() {}
};

}
