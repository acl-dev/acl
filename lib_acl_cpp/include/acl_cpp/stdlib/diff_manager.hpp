#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <vector>
#include "noncopyable.hpp"

namespace acl
{

class diff_object;

/**
 * 求两个集合的差集的管理器
 */
class diff_manager : public noncopyable
{
public:
	diff_manager(long long range_from = -1, long long range_to = -1);
	~diff_manager(void);

	/**
	 * 获得内部创建的内存池对象
	 * @return {dbuf_guard&} 返回内建内存池对象，使用该内存池创建的
	 *  对象必须在 diff_manager 对象销毁前销毁，因为 diff_manager 销毁时
	 *  该内建内存池会自动销毁
	 */
	dbuf_guard& get_dbuf(void);

	/**
	 * 比较两个集合的差集，从而获得两个集合新增的对象集合、删除的对象集合
	 * 以及变化的对象集合
	 * @param curr_objs {const std::vector<diff_object*>&} 当前对象的集合
	 * @param old_olds {const std::list<diff_object*>&} 旧元素集合，内部
	 *  用该集合生成哈希表，使当前集合与该旧集合进行差异化比较
	 */
	void diff_changes(const std::vector<diff_object*>& curr_objs,
		const std::vector<diff_object*>& old_olds);

	/**
	 * 获得新增的对象集合
	 * @return {std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_new(void) const
	{
		return objs_new_;
	}

	/**
	 * 当 diff_changes 进行差异化比较成功后，本函数用于返回相对于旧集合，
	 * 在当前集合中不存在的（即被删除的）元素集合
	 * @return {std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_deleted(void) const
	{
		return objs_del_;
	}

	/**
	 * 当 diff_changes 进行差异化比较成功后，本函数用于返回相对于旧集合，
	 * 在当前集合中对象值发生变化的变化集合对象
	 * @return {std::vector<std::pair<diff_object*, diff_object*> >&}
	 *  返回产生变化的对象的集合，其中 pair 中的 first 为新对象，second
	 *  为旧对象
	 * @sample
	 *  const std::vector<std::pair<diff_object*, diff_object*> >&
	 *  	results = manager.get_updated();
	 *  std::vector<std::pair<diff_object*, diff_object*> >::const_iterator
	 *  	cit = results.begin();
	 *  for (; cit != results.end(); ++cit)
	 *  	printf(">> key: %s, curr value:%s, old value: %s\r\n",
	 *  		(*cit).first->get_key(),
	 *  		(*cit).first->get_val(),
	 *  		(*cit).second->get_val());
	 */
	const std::vector<std::pair<diff_object*, diff_object*> >&
		get_updated(void) const
	{
		return objs_upd_;
	}

	/**
	 * 当 diff_manger 进行差异华比较成功后，本函数用于返回相同对象的集合
	 * @return {std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_same(void) const
	{
		return objs_equ_;
	}

	/**
	 * 获得新增的不在指定区间范围内的对象集合
	 * @return {const std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_extra_added(void) const
	{
		return objs_new_extra_;
	}

	/**
	 * 获得删除的不在指定区间范围内的对象集合
	 * @return {const std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_extra_deleted(void) const
	{
		return objs_del_extra_;
	}

	/**
	 * 获得修改的不在指定区间范围内的对象集合
	 * @return {const std::vector<diff_object*>&}
	 */
	const std::vector<std::pair<diff_object*, diff_object*> >&
		get_extra_updated(void) const
	{
		return objs_upd_extra_;
	}

	/**
	 * 当重复使用本 diff_manager 进行差异化比较时，需要调用本方法来清空
	 * 上一次比较过程中产生的临时内存及对象
	 */
	void reset(void);

private:
	dbuf_guard dbuf_;
	long long  range_from_;
	long long  range_to_;

	// 相同的对象集合
	std::vector<diff_object*> objs_equ_;

	// 变化的对象集合

	// 新增的对象集合
	std::vector<diff_object*> objs_new_;
	// 删除的对象集合
	std::vector<diff_object*> objs_del_;
	// 修改的对象集合
	std::vector<std::pair<diff_object*, diff_object*> > objs_upd_;

	// 多余的对象集合

	// 新增的多余对象集合
	std::vector<diff_object*> objs_new_extra_;
	// 删除的多余对象集合
	std::vector<diff_object*> objs_del_extra_;
	// 修改的多余对象集合
	std::vector<std::pair<diff_object*, diff_object*> > objs_upd_extra_;
};

} // namespace acl
