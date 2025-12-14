#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"
#include <stdlib.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <pthread.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
struct acl_pthread_mutex_t;
#else
# ifndef	acl_pthread_mutex_t
#  define	acl_pthread_mutex_t	pthread_mutex_t
# endif
#endif

namespace acl {

/**
 * Mutex lock, can create both file lock and thread lock simultaneously, or
 * create only one type of lock
 */
class ACL_CPP_API locker : public noncopyable {
public:
	/**
	 * Constructor
	 * @param use_mutex {bool} Whether to create thread lock
	 * @param use_spinlock {bool} Whether to use spinlock internally when using
	 * thread lock
	 */
	locker(bool use_mutex = true, bool use_spinlock = false);
	virtual ~locker();

	/**
	 * Create file lock based on file path
	 * @param file_path {const char*} File path, non-empty
	 * @return {bool} Whether successful
	 * Note: This function and the open function below can only be called one at a
	 * time
	 */
	bool open(const char* file_path);

	/**
	 * Create file lock based on file handle
	 * @param fh {int} File handle
	 * @return {bool} Whether successful
	 */
#if defined(_WIN32) || defined(_WIN64)
	bool open(void* fh);
#else
	bool open(int fh);
#endif

	/**
	 * Lock the already opened lock (including thread lock and file lock)
	 * @return {bool} Whether locking was successful
	 */
	bool lock();

	/**
	 * Try to lock the already opened lock (including thread lock and file lock)
	 * @return {bool} Whether locking was successful
	 */
	bool try_lock();

	/**
	 * Unlock the already opened lock (including thread lock and file lock)
	 * @return {bool} Whether unlocking was successful
	 */
	bool unlock();

private:
	acl_pthread_mutex_t* mutex_;
	char* pFile_;
#if defined(_WIN32) || defined(_WIN64)
	void* fHandle_;
#else
	int   fHandle_;
	pthread_mutexattr_t  mutex_attr_;
# if !defined(MINGW) && !defined(__APPLE__) && !defined(ANDROID)
	pthread_spinlock_t*  spinlock_;
# endif
#endif
	bool  myFHandle_;

	void init_mutex(bool use_spinlock);
};

class ACL_CPP_API lock_guard : public noncopyable
{
public:
	lock_guard(locker& lk);
	~lock_guard();

private:
	locker& lk_;
};

}  // namespace acl

