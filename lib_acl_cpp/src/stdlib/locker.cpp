#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/locker.hpp"
#endif

namespace acl {

locker::locker(bool use_mutex /* = true */, bool use_spinlock /* = false */)
: pFile_(NULL)
, myFHandle_(false)
{
	fHandle_ = ACL_FILE_INVALID;
	if (use_mutex)
		init_mutex(use_spinlock);
	else
	{
		mutex_ = NULL;
#ifdef	ACL_HAS_SPINLOCK
		spinlock_ = NULL;
#endif
	}
}

locker::~locker()
{
	if (pFile_)
		acl_myfree(pFile_);
	if (myFHandle_ && fHandle_ != ACL_FILE_INVALID)
		acl_file_close(fHandle_);
	if (mutex_)
	{
#ifndef	ACL_WINDOWS
		(void) pthread_mutexattr_destroy(&mutex_attr_);
#endif
		(void) acl_pthread_mutex_destroy(mutex_);
		acl_myfree(mutex_);
	}
#ifdef	ACL_HAS_SPINLOCK
	if (spinlock_)
	{
		pthread_spin_destroy(spinlock_);
		acl_myfree_fn((void*) spinlock_);
	}
#endif
}

void locker::init_mutex(bool use_spinlock acl_unused)
{

#ifdef	ACL_HAS_SPINLOCK
	if (use_spinlock)
	{
		spinlock_ = (pthread_spinlock_t*)
			acl_mycalloc(1, sizeof(pthread_spinlock_t));
		pthread_spin_init(spinlock_, PTHREAD_PROCESS_PRIVATE);
		mutex_= NULL;
		return;
	}
	else
		spinlock_ = NULL;
#endif

	mutex_ = (acl_pthread_mutex_t*)
		acl_mycalloc(1, sizeof(acl_pthread_mutex_t));
#ifdef ACL_WINDOWS
	acl_assert(acl_pthread_mutex_init(mutex_, NULL) == 0);
#else
	acl_assert(pthread_mutexattr_init(&mutex_attr_) == 0);
	acl_assert(pthread_mutexattr_settype(&mutex_attr_,
				PTHREAD_MUTEX_RECURSIVE) == 0);
	acl_assert(acl_pthread_mutex_init(mutex_, &mutex_attr_) == 0);
#endif
}

bool locker::open(const char* file_path)
{
	acl_assert(file_path && *file_path);
	acl_assert(pFile_ == NULL);
	acl_assert(fHandle_ == ACL_FILE_INVALID);

	fHandle_ = acl_file_open(file_path, O_RDWR | O_CREAT, 0600);
	if (fHandle_ == ACL_FILE_INVALID)
		return false;
	myFHandle_ = true;
	pFile_ = acl_mystrdup(file_path);
	return true;
}

bool locker::open(ACL_FILE_HANDLE fh)
{
	acl_assert(myFHandle_ == false);
	fHandle_ = fh;
	return true;
}

bool locker::lock()
{
#ifdef	ACL_HAS_SPINLOCK
	if (spinlock_)
	{
		if (pthread_spin_lock(spinlock_) != 0)
			return false;
	}
	else
#endif
	if (mutex_)
	{
		if (acl_pthread_mutex_lock(mutex_) != 0)
			return false;
	}

	if (fHandle_ == ACL_FILE_INVALID)
		return true;

	int operation = ACL_FLOCK_OP_EXCLUSIVE;
	if (acl_myflock(fHandle_, ACL_FLOCK_STYLE_FCNTL, operation) == 0)
		return true;

	if (mutex_)
		acl_assert(acl_pthread_mutex_unlock(mutex_) == 0);
	return false;
}

bool locker::try_lock()
{
#ifdef	ACL_HAS_SPINLOCK
	if (spinlock_)
	{
		if (pthread_spin_trylock(spinlock_) != 0)
			return false;
	}
	else
#endif
	if (mutex_)
	{
		if (acl_pthread_mutex_trylock(mutex_) != 0)
			return false;
	}

	if (fHandle_ == ACL_FILE_INVALID)
		return true;

	int operation = ACL_FLOCK_OP_EXCLUSIVE | ACL_FLOCK_OP_NOWAIT;
	if (acl_myflock(fHandle_, ACL_FLOCK_STYLE_FCNTL, operation) == 0)
		return true;

	if (mutex_)
		acl_assert(acl_pthread_mutex_unlock(mutex_) == 0);
	return false;
}

bool locker::unlock()
{
	bool  ret;

#ifdef	ACL_HAS_SPINLOCK
	if (spinlock_)
	{
		if (pthread_spin_unlock(spinlock_) == 0)
			ret = true;
		else
			ret = false;
	}
	else
#endif
	if (mutex_)
	{
		if (acl_pthread_mutex_unlock(mutex_) == 0)
			ret = true;
		else
			ret = false;
	}
	else
		ret = true;

	if (fHandle_ == ACL_FILE_INVALID)
		return ret;

	int operation = ACL_FLOCK_STYLE_FCNTL;
	if (acl_myflock(fHandle_, operation, ACL_FLOCK_OP_NONE) == -1)
		return false;
	return ret;
}

/////////////////////////////////////////////////////////////////////////////

lock_guard::lock_guard(locker& lk)
: lk_(lk)
{
	acl_assert(lk_.lock());
}

lock_guard::~lock_guard()
{
	acl_assert(lk_.unlock());
}

} // namespace acl
