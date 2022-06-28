#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/diff_object.hpp"
#include "acl_cpp/stdlib/diff_manager.hpp"
#endif

namespace acl
{

diff_manager::diff_manager(long long range_from /* = -1 */,
	long long range_to /* = -1 */)
: dbuf_(4, 10000)
{
	dbuf_.set_increment(10000);

	if (range_from > 0 && range_to >= range_from) {
		range_from_ = range_from;
		range_to_   = range_to;
	} else {
		range_from_ = -1;
		range_to_   = -1;
	}
}

diff_manager::~diff_manager(void)
{
}

acl::dbuf_guard& diff_manager::get_dbuf(void)
{
	return dbuf_;
}

void diff_manager::reset(void)
{
	objs_new_.clear();
	objs_del_.clear();
	objs_upd_.clear();
	objs_equ_.clear();

	dbuf_.dbuf_reset();
}

void diff_manager::diff_changes(const std::vector<diff_object*>& curr_objs,
	const std::vector<diff_object*>& old_objs)
{
	size_t size = curr_objs.size() + old_objs.size();

	// 一次性预分配数组空间，以免在添加过程中内存重新分配
	objs_equ_.reserve(size + 1);
	objs_new_.reserve(size + 1);
	objs_del_.reserve(size + 1);
	objs_upd_.reserve(size + 1);
	objs_new_extra_.reserve(size + 1);
	objs_del_extra_.reserve(size + 1);
	objs_upd_extra_.reserve(size + 1);

	const char* key;

	// 创建哈希表，将旧数组中的元素添加进哈希表中

	ACL_HTABLE *htable = acl_htable_create((int) size * 2 + 1,
			ACL_HTABLE_FLAG_KEY_REUSE);

	for (std::vector<diff_object*>::const_iterator cit = old_objs.begin();
		cit != old_objs.end(); ++cit) {

		key = (const char* ) (*cit)->get_key();
		(void) acl_htable_enter(htable, key, (void*) *cit);
	}

	ACL_HTABLE_INFO* entry;
	diff_object* obj;

	// 遍历当前集合中的对象，将之与旧对象集合进行比较，找出新增的，旧的
	// 以及变化的对象集合
	for (std::vector<diff_object*>::const_iterator cit = curr_objs.begin();
		cit != curr_objs.end(); ++cit) {

		key = (*cit)->get_key();

		// 在旧集合中查询当前对象是否存在
		// obj = (diff_object*) acl_htable_find(htable, key);
		entry = acl_htable_locate(htable, key);

		// 如果不存在，则说明该对象为新对象，则将该对象做为新添加对象
		// 添加进新对象集合中
		if (entry == NULL) {
			if ((*cit)->check_range(range_from_, range_to_)) {
				objs_new_extra_.push_back(*cit);
			} else {
				objs_new_.push_back(*cit);
			}
			continue;
		}

		obj = (diff_object*) entry->value;

		// 如果当前对象与旧对象相同，则将该对象放至相同对象集合中，
		// 同时将之从当前对象集合及旧对象集合（哈希集合）中删除
		if (*obj == **cit) {
			acl_htable_delete_entry(htable, entry, NULL);
			objs_equ_.push_back(*cit);
			continue;
		}

		// 说明是 KEY 相等，但对象中的内容并不相等，则需要从当前集合
		// 及哈希集合中删除，并加入至变化的集合中
		if ((*cit)->check_range(range_from_, range_to_)) {
			objs_upd_extra_.push_back(std::make_pair(*cit, obj));
		} else {
			objs_upd_.push_back(std::make_pair(*cit, obj));
		}
		(void) acl_htable_delete(htable, key, NULL);
	}

	// printf("objs_new_extra_: %d, %d\r\n",
	// 	(int) objs_new_extra_.size(), (int) objs_new_.size());
	// 遍历存有旧对象的集合（哈希表对象），其中剩余的元素为已删除的对象
	ACL_ITER iter;
	acl_foreach(iter, htable) {
		obj = (diff_object*) iter.data;
		if (obj->check_range(range_from_, range_to_)) {
			objs_del_extra_.push_back(obj);
		} else {
			objs_del_.push_back(obj);
		}
	}

	// 销毁临时哈希表对象
	acl_htable_free(htable, NULL);
}

} // namespace diff
