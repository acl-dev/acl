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
class identity {
public:
	typedef T me;
};

template<typename TDerive, typename TProvider>
class final_tpl_base {
	friend class identity<TDerive>::me;
	friend class identity<TProvider>::me;
private:
	final_tpl_base() {}
	~final_tpl_base() {}
};
#endif

/*
 * Provides functionality to prevent derivation. Classes that need this functionality can derive from final_tpl,
 * and pass the class name as a template parameter
 * @example:
 * class my_final_class : public acl::final_tpl <my_final_class>
 * {
 * public:
 *   my_final_class() {}
 *   ~my_final_class() {}
 * }
 * This ensures that my_final_class cannot be inherited
 */
template<typename TFinalClass>
class final_tpl : virtual public final_tpl_base<TFinalClass,
	final_tpl<TFinalClass> > {
public:
	final_tpl() {}
	~final_tpl() {}
};

}

