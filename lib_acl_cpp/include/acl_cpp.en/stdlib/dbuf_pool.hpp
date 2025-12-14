#pragma once
#include "../acl_cpp_define.hpp"
//#include "noncopyable.hpp"
#include <vector>

struct ACL_DBUF_POOL;

namespace acl {

/**
 * Session-based memory pool management class, which provides memory allocation
 * functions. When the object is destroyed, memory will
 * be released all at once. It is suitable for applications that need to
 * frequently allocate some small-sized small memory blocks.
 * This implementation is actually a wrapper for the ACL_DBUF_POOL structure in
 * lib_acl.
 */

#if 0
#ifndef ACL_DBUF_HOOK_NEW
# define ACL_DBUF_HOOK_NEW
#endif
#endif

class ACL_CPP_API dbuf_pool { // : public noncopyable
public:
	/**
	 * Constructor, can be dynamically allocated.
	 * @param nblock {size_t} Number of 4096-byte blocks.
	 * @param align {size_t} Byte alignment when allocating memory.
	 */
	dbuf_pool(size_t nblock = 2, size_t align = 8);

	/**
	 * If the object needs to be dynamically allocated, after use, you need to call
	 * destroy
	 * to release the dynamically allocated object.
	 */
	void destroy() const;

#ifdef ACL_DBUF_HOOK_NEW
	/**
	 * Override new/delete operators so that dbuf_pool objects can also be
	 * allocated on the memory pool,
	 * thereby avoiding malloc/free overhead.
	 * @param size {size_t} Variable parameter, dbuf_pool object's length size.
	 * @param nblock {size_t} Internal memory block (4096) multiplier used.
	 */
	static void *operator new(size_t size, size_t nblock = 2);

# if defined(_WIN32) || defined(_WIN64)
	static void operator delete(void* ptr, size_t);
# endif
	static void operator delete(void* ptr);
#endif // ACL_DBUF_HOOK_NEW

	/**
	 * Reset memory pool state to reuse this memory pool object.
	 * @param reserve {size_t} When value > 0, you need to specify the reserved
	 * memory block size.
	 * This size should be less than or equal to the size already allocated in this
	 * memory pool object.
	 * @return {bool} Whether reset was successful, returns false on error.
	 */
	bool dbuf_reset(size_t reserve = 0);

	/**
	 * Allocate memory of specified length.
	 * @param len {size_t} Memory length to be allocated. When memory is relatively
	 * small (less than constructor's
	 * block_size), allocated memory will be allocated on dbuf_pool object's
	 * internal memory pool.
	 *  When memory is large, malloc will be used directly for allocation.
	 * @return {void*} Newly allocated memory address.
	 */
	void* dbuf_alloc(size_t len);

	/**
	 * Allocate memory of specified length and initialize memory to zero.
	 * @param len {size_t} Memory length to be allocated.
	 * @return {void*} Newly allocated memory address.
	 */
	void* dbuf_calloc(size_t len);

	/**
	 * Dynamically allocate new memory and copy the string, similar to strdup.
	 * @param s {const char*} Source string.
	 * @return {char*} Address of newly copied string.
	 */
	char* dbuf_strdup(const char* s);

	/**
	 * Dynamically allocate new memory and copy the string, similar to strdup.
	 * @param s {const char*} Source string.
	 * @param len {size_t} Maximum length of the string to be copied.
	 * @return {char*} Address of newly copied string.
	 */
	char* dbuf_strndup(const char* s, size_t len);

	/**
	 * Dynamically allocate memory and copy memory data.
	 * @param addr {const void*} Source memory address.
	 * @param len {size_t} Source data length.
	 * @return {void*} Address of newly copied data.
	 */
	void* dbuf_memdup(const void* addr, size_t len);

	/**
	 * Return memory allocated by memory pool.
	 * @param addr {const void*} Memory address allocated by memory pool.
	 * @return {bool} Returns false if memory address is not allocated by memory
	 * pool or released multiple times.
	 */
	bool dbuf_free(const void* addr);

	/**
	 * Mark a certain address allocated by memory pool to prevent it from being
	 * released when dbuf_reset is called.
	 * @param addr {const void*} Memory address allocated by memory pool.
	 * @return {bool} Returns false if memory address is not allocated by memory
	 * pool.
	 */
	bool dbuf_keep(const void* addr);

	/**
	 * Remove the mark on a certain address allocated by memory pool, so it will be
	 * released when dbuf_reset is called.
	 * @param addr {const void*} Memory address allocated by memory pool.
	 * @return {bool} Returns false if memory address is not allocated by memory
	 * pool.
	 */
	bool dbuf_unkeep(const void* addr);

	/**
	 * Get internal ACL_DBUF_POOL handle to use C interface memory pool object
	 * internally.
	 * @return {ACL_DBUF_POOL*}
	 */
	ACL_DBUF_POOL *get_dbuf() const {
		return pool_;
	}

private:
	ACL_DBUF_POOL* pool_;
	size_t mysize_;

public:
	~dbuf_pool();
};

/**
 * sample:
 *  void test() {
 *      acl::dbuf_pool* dbuf = new acl::dbuf_pool;
 *      for (int i = 0; i < 1000; i++) {
 *          char* ptr = dbuf->dbuf_strdup("hello world!");
 *          printf("%s\r\n", p);
 *      }
 *      dbuf->destroy();
 *
 *      // When creating dbuf object, specify internal memory block multiplier.
 *      dbuf = new(8) acl::dbuf_pool;
 *      for (int i = 0; i < 1000; i++) {
 *          ptr = dbuf->dbuf_strdup("hello world!");
 *          printf("%s\r\n", p);
 *      }
 *
 *	// Release dbuf object.
 *      dbuf->destroy();
 *  }
 *
 */
//////////////////////////////////////////////////////////////////////////////

class dbuf_guard;

/**
 * Base class for objects allocated on session memory pool.
 */
class ACL_CPP_API dbuf_obj { //: public noncopyable
public:
	/**
	 * Constructor
	 * @param guard {dbuf_guard*} When this parameter is not empty, this object
	 * will be
	 * automatically managed and uniformly destroyed by dbuf_guard object. If this
	 * parameter is empty, applications should
	 * call dbuf_guard::push_back to add this object to the object collection for
	 * unified management.
	 */
	dbuf_obj(dbuf_guard* guard = NULL);

	virtual ~dbuf_obj() {}

	/**
	 * Get this object's index position in dbuf_guard's object collection.
	 * @return {int} Returns this object's index position in dbuf_guard's object
	 * collection. If
	 * this object has not been managed by dbuf_guard, returns -1. At this time,
	 * there may be a memory leak.
	 */
	int pos() const {
		return pos_;
	}

	/**
	 * Get the dbuf_guard object passed in constructor.
	 * @return {dbuf_guard*}
	 */
	dbuf_guard* get_guard() const {
		return guard_;
	}

private:
	friend class dbuf_guard;

	// Record which dbuf_guard object manages this object.
	dbuf_guard* guard_;

	// This field is used by dbuf_guard, for safety.
	int nrefer_;

	// This object's index position recorded in dbuf_guard object collection.
	int pos_;
};

/**
 * Session memory pool guard class, manages objects allocated on dbuf_pool. When
 * the object
 * is destroyed, dbuf_pool will automatically release all allocated memory.
 */
class ACL_CPP_API dbuf_guard { // : public noncopyable
public:
	/**
	 * Constructor
	 * @param dbuf {dbuf_pool*} Memory pool object. When not empty, dbuf is managed
	 * by dbuf_guard
	 * object. If empty, this constructor internally automatically creates a
	 * dbuf_pool object.
	 * @param capacity {size_t} Initial capacity of internal objs_ array.
	 */
	dbuf_guard(dbuf_pool* dbuf, size_t capacity = 500);

	/**
	 * Constructor
	 * @param nblock {size_t} When creating internal dbuf_pool object, you can
	 *  specify the memory block (4096) multiplier.
	 * @param capacity {size_t} Initial capacity of internal objs_ array.
	 * @param align {size_t} Byte alignment when allocating memory.
	 */
	dbuf_guard(size_t nblock = 2, size_t capacity = 500, size_t align = 8);

	/**
	 * Destructor. When destroying, internally automatically destroys dbuf_pool
	 * object created by constructor.
	 */
	~dbuf_guard();

	/**
	 * Call dbuf_pool::dbuf_reset
	 * @param reserve {size_t}
	 * @return {bool}
	 */
	bool dbuf_reset(size_t reserve = 0);

	/**
	 * Call dbuf_pool::dbuf_alloc
	 * @param len {size_t}
	 * @return {void*}
	 */
	void* dbuf_alloc(size_t len) {
		return dbuf_->dbuf_alloc(len);
	}

	/**
	 * Call dbuf_pool::dbuf_calloc
	 * @param len {size_t}
	 * @return {void*}
	 */
	void* dbuf_calloc(size_t len) {
		return dbuf_->dbuf_calloc(len);
	}

	/**
	 * Call dbuf_pool::dbuf_strdup
	 * @param s {const char*}
	 * @return {char*}
	 */
	char* dbuf_strdup(const char* s) {
		return dbuf_->dbuf_strdup(s);
	}

	/**
	 * Call dbuf_pool::dbuf_strndup
	 * @param s {const char*}
	 * @param len {size_t}
	 * @return {char*}
	 */
	char* dbuf_strndup(const char* s, size_t len) {
		return dbuf_->dbuf_strndup(s, len);
	}

	/**
	 * Call dbuf_pool::dbuf_memdup
	 * @param addr {const void*}
	 * @param len {size_t}
	 * @return {void*}
	 */
	void* dbuf_memdup(const void* addr, size_t len) {
		return dbuf_->dbuf_memdup(addr, len);
	}

	/**
	 * Call dbuf_pool::dbuf_free
	 * @param addr {const void*}
	 * @return {bool}
	 */
	bool dbuf_free(const void* addr) {
		return dbuf_->dbuf_free(addr);
	}

	/**
	 * Call dbuf_pool::dbuf_keep
	 * @param addr {const void*}
	 * @return {bool}
	 */
	bool dbuf_keep(const void* addr) {
		return dbuf_->dbuf_keep(addr);
	}

	/**
	 * Call dbuf_pool::dbuf_unkeep
	 * @param addr {const void*}
	 * @return {bool}
	 */
	bool dbuf_unkeep(const void* addr) {
		return dbuf_->dbuf_unkeep(addr);
	}

	/**
	 * Get dbuf_pool object.
	 * @return {acl::dbuf_pool&}
	 */
	acl::dbuf_pool& get_dbuf() const {
		return *dbuf_;
	}

	/**
	 * Add object. This method adds dbuf_obj objects allocated on dbuf_pool to
	 * dbuf_guard object for unified management and destruction. If the same
	 * dbuf_obj object is added to the same
	 * dbuf_guard object multiple times, the dbuf_guard object will have reference
	 * counting mechanism, which will
	 * cause repeated releases.
	 * @param obj {dbuf_obj*}
	 * @return {int} Returns obj object's index position in dbuf_obj object
	 * collection after adding.
	 * dbuf_guard internally has reference counting mechanism for dbuf_obj objects,
	 * so when
	 * the same dbuf_obj object is added to the same dbuf_guard object multiple
	 * times, internally only one is recorded.
	 */
	int push_back(dbuf_obj* obj);

	/**
	 * Get the number of objects currently managed in memory pool.
	 * @return {size_t}
	 */
	size_t size() const {
		return size_;
	}

	/**
	 * Get object at specified index.
	 * @param pos {size_t} Specified object index position, should not exceed
	 * bounds.
	 * @return {dbuf_obj*} Returns NULL when index position exceeds bounds.
	 */
	dbuf_obj* operator[](size_t pos) const;

	/**
	 * Get object at specified index.
	 * @param pos {size_t} Specified object index position, should not exceed
	 * bounds.
	 * @return {dbuf_obj*} Returns NULL when index position exceeds bounds.
	 */
	dbuf_obj* get(size_t pos) const;

	/**
	 * Set the increment size when expanding objs_ array each time. Internal
	 * default value is 100.
	 * @param incr {size_t}
	 */
	void set_increment(size_t incr);

public:
	template <typename T>
	T* create() {
		T* t = new (dbuf_alloc(sizeof(T))) T();
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1>
	T* create(P1 p) {
		T* t = new (dbuf_alloc(sizeof(T))) T(p);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2>
	T* create(P1 p1, P2 p2) {
		T* t = new (dbuf_alloc(sizeof(T))) T(p1, p2);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3>
	T* create(P1 p1, P2 p2, P3 p3) {
		T* t = new (dbuf_alloc(sizeof(T))) T(p1, p2, p3);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3,
		 typename P4>
	T* create(P1 p1, P2 p2, P3 p3, P4 p4) {
		T* t = new (dbuf_alloc(sizeof(T))) T(p1, p2, p3, p4);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3,
		 typename P4, typename P5>
	T* create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
		T* t = new (dbuf_alloc(sizeof(T))) T(p1, p2, p3, p4, p5);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3,
		 typename P4, typename P5, typename P6>
	T* create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) {
		T* t = new (dbuf_alloc(sizeof(T))) T(p1, p2, p3, p4, p5, p6);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3,
		 typename P4, typename P5, typename P6, typename P7>
	T* create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) {
		T* t = new (dbuf_alloc(sizeof(T)))
			T(p1, p2, p3, p4, p5, p6, p7);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3,
		 typename P4, typename P5, typename P6, typename P7,
		 typename P8>
	T* create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8) {
		T* t = new (dbuf_alloc(sizeof(T)))
			T(p1, p2, p3, p4, p5, p6, p7, p8);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3,
		 typename P4, typename P5, typename P6, typename P7,
		 typename P8,typename P9>
	T* create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7,
		P8 p8, P9 p9) {
		T* t = new (dbuf_alloc(sizeof(T)))
			T(p1, p2, p3, p4, p5, p6, p7, p8, p9);
		(void) push_back(t);
		return t;
	}

	template <typename T, typename P1, typename P2, typename P3,
		 typename P4, typename P5, typename P6, typename P7,
		 typename P8, typename P9, typename P10>
	T* create(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7,
		P8 p8, P9 p9, P10 p10) {
		T* t = new (dbuf_alloc(sizeof(T)))
			T(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
		(void) push_back(t);
		return t;
	}

private:
	size_t nblock_;			// Internal unit multiplier for dbuf_pool memory blocks
	size_t incr_;			// Capacity size when creating new dbuf_objs_link
	dbuf_pool* dbuf_;		// Memory pool object
	dbuf_pool* dbuf_internal_;	// Memory pool object

	// Here, dbuf_obj objects are used instead of std::vector.
	// One reason is that objects are also allocated on dbuf_pool memory pool,
	// which is another reason.
	// std::vector's internal memory allocation is not controllable.

	struct dbuf_objs_link {
		dbuf_obj** objs;	// Array storing dbuf_obj object pointers
		size_t size;		// Number of objects stored in objs
		size_t capacity;	// Size of objs array

		struct dbuf_objs_link* next;
	};

	dbuf_objs_link  head_;
	dbuf_objs_link* curr_;
	size_t size_;

	void init(size_t capacity);

	// Expand objs_ array space.
	void extend_objs();
};

/**
 * sample1:
 * // Inherit from acl::dbuf_obj class.
 * class myobj1 : public acl::dbuf_obj {
 * public:
 * // Pass guard object to constructor, and the object will be automatically
 * added to guard's object collection.
 * 	myobj1(acl::dbuf_guard* guard) : dbuf_obj(guard) {}
 *
 * 	void doit() {
 * 		printf("hello world!\r\n");
 * 	}
 *
 * private:
 * 	~myobj1() {}
 * };
 *
 * void test() {
 * 	acl::dbuf_guard dbuf;
 *
 *	// Dynamically create 100 myobj objects on dbuf_guard memory pool.
 * 	for (int i = 0; i < 100; i++) {
 * // Dynamically create myobj object on guard memory pool and pass guard as
 * constructor parameter.
 * 		myobj* obj = new (dbuf.dbuf_alloc(sizeof(myobj))) myobj(&dbuf);
 * 		obj->doit();
 * 	}
 *
 * // When dbuf is destroyed, all dynamically created objects will be
 * automatically destroyed.
 * }
 *
 * // sample2
 * class myobj2 : public acl::dbuf_obj {
 * public:
 * 	myobj2() {}
 *
 * 	void doit() {
 * 		printf("hello world\r\n");
 * 	}
 *
 * private:
 * 	~myobj2() {}
 * };
 *
 * void test2() {
 * 	acl::dbuf_guard dbuf;
 *
 * 	for (int i = 0; i < 100; i++) {
 * 		myobj2* obj = dbuf.create<myobj2>();
 * 		obj->doit();
 * 	}
 * }
 *
 * // sample3
 * class myobj2 : public acl::dbuf_obj {
 * public:
 * 	myobj2(int i) : i_(i) {}
 *
 * 	void doit() {
 * 		printf("hello world, i: %d\r\n", i_);
 * 	}
 *
 * private:
 * 	~myobj2() {}
 *
 * private:
 *	int i_;
 * };
 *
 * void test2() {
 * 	acl::dbuf_guard dbuf;
 *
 * 	for (int i = 0; i < 100; i++) {
 * 		myobj2* obj = dbuf.create<myobj2>(i);
 * 		obj->doit();
 * 	}
 * }
 */
} // namespace acl

