#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl {

typedef enum {
	REDIS_RESULT_UNKOWN,
	REDIS_RESULT_NIL,
	REDIS_RESULT_ERROR,
	REDIS_RESULT_STATUS,
	REDIS_RESULT_INTEGER,
	REDIS_RESULT_STRING,
	REDIS_RESULT_ARRAY,
} redis_result_t;

class string;
class dbuf_pool;
class redis_client;

/**
 * Redis-server return result object class. After analyzing data returned by
 * redis-server, create
 * redis_result class object.
 * the redis result for redis-server's reply
 */
class ACL_CPP_API redis_result : public noncopyable {
public:
	explicit redis_result(dbuf_pool* dbuf);

	/**
	 * Overloaded new/delete operators. When creating new object, makes memory
	 * allocation
	 * performed in memory pool
	 * override new/delete operator, when the new object was created,
	 * memory was alloc in dbuf_pool, which is a memroy pool allocator
	 */
	void *operator new(size_t size, dbuf_pool* pool);
	void operator delete(void* ptr, dbuf_pool* pool);

	/**
	 * Get data type of current result node
	 * get the data type of the reply from redis-server
	 * @return {redis_result_t}
	 *  defined above REDIS_RESULT_
	 */
	redis_result_t get_type() const {
		return result_type_;
	}

	/**
	 * Get number of objects stored in current result node
	 * get the number of objects from redis-server
	 * @return {size_t} Correspondence between return value and storage type is as
	 * follows:
	 *  the relation between returned value and result type show below:
	 *  REDIS_RESULT_ERROR: 1
	 *  REDIS_RESULT_STATUS: 1
	 *  REDIS_RESULT_INTEGER: 1
	 * REDIS_RESULT_STRING: > 0 indicates number of non-contiguous memory blocks
	 * this string data is split into
	 *  REDIS_RESULT_ARRAY: children_->size()
	 */
	size_t get_size() const;

	/**
	 * When return value is REDIS_RESULT_INTEGER type, this method returns
	 * corresponding 32-bit integer value
	 * get the 32 bits integer for REDIS_RESULT_INTEGER result
	 * @param success {bool*} When this pointer is not NULL, records whether
	 * operation process was successful
	 *  when not NULL, storing the status of success
	 * @return {int}
	 */
	int get_integer(bool* success = NULL) const;

	/**
	 * When return value is REDIS_RESULT_INTEGER type, this method returns
	 * corresponding 64-bit integer value
	 * get the 64 bits integer for REDIS_RESULT_INTEGER result
	 * @param success {bool*} When this pointer is not NULL, records whether
	 * operation process was successful
	 *  when not NULL, storing the status of success
	 * @return {long long int}
	 */
	long long int get_integer64(bool* success = NULL) const;

	/**
	 * When return value is REDIS_RESULT_STRING type, this method returns
	 * corresponding double type value
	 * get the double value for REDIS_RESULT_STRING result
	 * @param success {bool*} When this pointer is not NULL, records whether
	 * operation process was successful
	 *  when not NULL, storing the status of success
	 * @return {double}
	 */
	double get_double(bool* success = NULL) const;

	/**
	 * When return value is REDIS_RESULT_STATUS type, this method returns status
	 * information
	 * get operation status for REDIS_RESULT_STATUS result
	 * @return {const char*} Returns "" indicates error
	 *  error if empty string returned
	 */
	const char* get_status() const;

	/**
	 * When error occurs, return value is REDIS_RESULT_ERROR type, this method
	 * returns error information
	 * when some error happened, this can get the error information
	 * @return {const char*} Returns empty string "" indicates there is no error
	 * information
	 *  there was no error information if empty string returned
	 */
	const char* get_error() const;

	/**
	 * Return data corresponding to index (when data type is not
	 * REDIS_RESULT_ARRAY)
	 * get the string data of associated subscript(just for the type
	 * of no REDIS_RESULT_ARRAY)
	 * @param i {size_t} Array index
	 *  the array's subscript
	 * @param len {size_t*} When it is not NULL pointer, stores length of returned
	 * data
	 *  when not NULL, the parameter will store the length of the result
	 * @return {const char*} Returns NULL indicates index out of bounds
	 *  NULL if nothing exists or the subscript is out of bounds
	 */
	const char* get(size_t i, size_t* len = NULL) const;

	/**
	 * Return address of all data array (when data type is not REDIS_RESULT_ARRAY)
	 * return all data's array if the type isn't REDIS_RESULT_ARRAY
	 * @return {const char**}
	 */
	const char** get_argv() const {
		return (const char**) argv_;
	}

	/**
	 * Return address of all data length array (when data type is not
	 * REDIS_RESULT_ARRAY)
	 * return all length's array if the type isn't REDIS_RESULT_ARRAY
	 * @return {const size_t*}
	 */
	const size_t* get_lens() const {
		return lens_;
	}

	/**
	 * Return total length of all data (when data type is not REDIS_RESULT_ARRAY)
	 * return the total length of all data for no REDIS_RESULT_ARRAY
	 * @return {size_t}
	 */
	size_t get_length() const;

	/**
	 * When data type is REDIS_RESULT_STRING type, this function stores data stored
	 * in memory blocks
	 * into contiguous memory, but need to be careful to prevent memory overflow
	 * compose a continus data for the slicing chunk data internal
	 * @param buf {string&} Store result data. Internally will first call
	 * buf.clear()
	 *  store the result
	 * @param clear_auto {bool} if clear the buf internal.
	 * @return {int} Total length of data. Return value 0 indicates internal array
	 * is empty
	 *  return the total length of data, 0 if data array has no elements
	 */
	int argv_to_string(string& buf, bool clear_auto = true) const;
	int argv_to_string(char* buf, size_t size) const;

	/**
	 * When data type is REDIS_RESULT_ARRAY type, this function returns all array
	 * objects
	 * return the objects array when result type is REDIS_RESULT_ARRAY
	 * @param size {size_t*} When returned array is not empty, this address stores
	 * array length
	 *  store the array's length if size isn't NULL
	 * @return {const const redis_result*}
	 */
	const redis_result** get_children(size_t* size) const;

	/**
	 * When data type is REDIS_RESULT_ARRAY type, this function returns result
	 * object corresponding to index
	 * get one object of the given subscript from objects array
	 * @param i {size_t} Index value
	 *  the given subscript
	 * @return {const redis_result*} When index value is out of bounds or result
	 * does not exist, returns NULL
	 *  NULL if subscript is out of bounds or object not exist
	 */
	const redis_result* get_child(size_t i) const;

	/**
	 * Return memory pool object passed in constructor
	 * get the memory pool object set in constructor
	 * @return {dbuf_pool*}
	 */
	dbuf_pool* get_dbuf() {
		return dbuf_;
	}

	/**
	 * Convert entire object to string
	 * @param out {string&} Store result (added in append mode)
	 * @return {const string&}
	 */
	const string& to_string(string& out) const;

private:
	~redis_result();

	friend class redis_client;
	void clear();

public:
	redis_result& set_type(redis_result_t type);
	redis_result& set_size(size_t size);
	redis_result& put(const char* buf, size_t len);
	redis_result& put(const redis_result* rr, size_t idx);

private:
	redis_result_t result_type_;
	dbuf_pool* dbuf_;

	size_t  size_;
	size_t  idx_;
	const char** argv_;
	size_t* lens_;

	//std::vector<const redis_result*>* children_;
	const redis_result** children_;
	size_t  children_size_;
	size_t  children_idx_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

