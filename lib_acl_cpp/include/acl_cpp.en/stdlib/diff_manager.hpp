#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include <vector>
#include "noncopyable.hpp"

namespace acl {

class diff_object;

/**
 * Manager for finding the difference set of two collections
 */
class diff_manager : public noncopyable {
public:
	diff_manager(long long range_from = -1, long long range_to = -1);
	~diff_manager();

	/**
	 * Get internally created memory pool object
	 * @return {dbuf_guard&} Returns built-in memory pool object. Objects created using this memory pool
	 *  must be destroyed before diff_manager object is destroyed, because when diff_manager is destroyed,
	 *  this built-in memory pool will be automatically destroyed
	 */
	dbuf_guard& get_dbuf();

	/**
	 * Compare the difference set of two collections to get the set of newly added objects, deleted objects
	 * and changed objects of the two collections
	 * @param curr_objs {const std::vector<diff_object*>&} Collection of current objects
	 * @param old_olds {const std::list<diff_object*>&} Collection of old elements. Internally
	 *  uses this collection to generate hash table, so that current collection can be compared with this old collection for differences
	 */
	void diff_changes(const std::vector<diff_object*>& curr_objs,
		const std::vector<diff_object*>& old_olds);

	/**
	 * Get collection of newly added objects
	 * @return {std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_new() const {
		return objs_new_;
	}

	/**
	 * After diff_changes successfully performs difference comparison, this function is used to return, relative to old collection,
	 * the collection of elements that do not exist in current collection (i.e., deleted)
	 * @return {std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_deleted() const {
		return objs_del_;
	}

	/**
	 * After diff_changes successfully performs difference comparison, this function is used to return, relative to old collection,
	 * the collection of objects in current collection whose values have changed
	 * @return {std::vector<std::pair<diff_object*, diff_object*> >&}
	 *  Returns collection of objects that have changed, where first in pair is new object, second
	 *  is old object
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
		get_updated() const {
		return objs_upd_;
	}

	/**
	 * After diff_manger successfully performs difference comparison, this function is used to return collection of same objects
	 * @return {std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_same() const {
		return objs_equ_;
	}

	/**
	 * Get collection of newly added objects outside the specified range
	 * @return {const std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_extra_added() const {
		return objs_new_extra_;
	}

	/**
	 * Get collection of deleted objects outside the specified range
	 * @return {const std::vector<diff_object*>&}
	 */
	const std::vector<diff_object*>& get_extra_deleted() const {
		return objs_del_extra_;
	}

	/**
	 * Get collection of modified objects outside the specified range
	 * @return {const std::vector<std::pair<diff_object*, diff_object*> >&}
	 */
	const std::vector<std::pair<diff_object*, diff_object*> >&
		get_extra_updated() const {
		return objs_upd_extra_;
	}

	/**
	 * When repeatedly using this diff_manager for difference comparison, need to call this method to clear
	 * temporary memory and objects generated during the previous comparison process
	 */
	void reset();

private:
	dbuf_guard dbuf_;
	long long  range_from_;
	long long  range_to_;

	// Collection of same objects
	std::vector<diff_object*> objs_equ_;

	// Collection of changed objects

	// Collection of newly added objects
	std::vector<diff_object*> objs_new_;
	// Collection of deleted objects
	std::vector<diff_object*> objs_del_;
	// Collection of modified objects
	std::vector<std::pair<diff_object*, diff_object*> > objs_upd_;

	// Collection of redundant objects

	// Collection of newly added redundant objects
	std::vector<diff_object*> objs_new_extra_;
	// Collection of deleted redundant objects
	std::vector<diff_object*> objs_del_extra_;
	// Collection of modified redundant objects
	std::vector<std::pair<diff_object*, diff_object*> > objs_upd_extra_;
};

} // namespace acl

