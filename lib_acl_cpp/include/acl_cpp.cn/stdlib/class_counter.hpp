#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <string>
#include "singleton.hpp"

#define	ACL_COUNTER_INIT(thread_safe)                                         \
	acl::class_counter::get_instance().init(thread_safe)

// 某个字符标记增加一次, flagName 为自己定义的唯一字符对象
#define ACL_COUNTER_INC(flagName)                                             \
	acl::class_counter::get_instance().inc(#flagName)

// 某个字符标记减少一次
#define ACL_COUNTER_DEC(flagName)                                             \
	acl::class_counter::get_instance().dec(#flagName)

#define	ACL_COUNTER_COUNT(flagName)                                           \
	acl::class_counter::get_instance().count(#flagName)

#define	ACL_COUNTER_PRINT()                                                   \
	acl::class_counter::get_instance().print()

namespace acl {

class thread_mutex;

class ACL_CPP_API class_counter : public singleton<class_counter> {
public:
	/**
	 * 构造方法
	 * @param clean {bool} 是否自动清除计数为 0 的计数对象.
	 */
	class_counter(bool clean = true);
	~class_counter();

	/**
	 * 可以在进程初始化时调用本方法(非必须)进行初始化,指定内部是否需要加锁,
	 * 如果不调用本方法,内部自动加线程锁.
	 */
	void init(bool thread_safe = true);

	/**
	 * @brief 将name对应的计数加1,当没有name时设置为1
	 */
	void inc(const char *name);

	/**
	 * @brief 将name对应的计数减1,当没有name时将输出错误日志
	 */
	void dec(const char *name);

	/**
	 * @brief 获取name对象的统计个数
	 */
	long long count(const char *name);

	/**
	 * @brief 输出计数统计
	 * @param flag 调用者标记
	 */
	void print(const char *flag = NULL);

private:
	std::map<std::string, long long> names_;
	thread_mutex* lock_;
	bool clean_;
};

}  // namespace acl
