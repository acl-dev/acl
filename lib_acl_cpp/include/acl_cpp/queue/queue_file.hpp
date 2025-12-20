#pragma once
#include <time.h>
#include "../stdlib/string.hpp"
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"
#include "queue_manager.hpp"

#ifndef MAXPATH255
#define MAXPATH255 255
#endif

namespace acl {

class fstream;

class ACL_CPP_API queue_file : public noncopyable
{
public:
	queue_file();

	/**
	 * Get file stream pointer
	 * @return {acl::fstream*} File stream pointer. If NULL, it indicates file has
	 * not been opened yet
	 */
	fstream* get_fstream(void) const;

	/**
	 * Get file creation time
	 * @return {time_t}, Returns seconds since 1970. If return value is (time_t)
	 * -1,
	 *  it indicates error
	 */
	time_t get_ctime(void) const;

	/**
	 * Write data to file
	 * @param data {const void*} Data address
	 * @param len {size} Data length
	 * @return {bool} Whether writing data was successful
	 */
	bool write(const void* data, size_t len);
	int format(const char* fmt, ...) ACL_CPP_PRINTF(2, 3);
	int vformat(const char* fmt, va_list ap);

	/**
	 * Read data from file
	 * @param buf {void*} Buffer address
	 * @param len {size_t} buf size
	 * @return {int} Length of data read. -1: Indicates read ended or read failed
	 * or input parameter error,
	 *  should close this file object. > 0: Indicates success
	 */
	int read(void* buf, size_t len);

	/**
	 * Get key value of this queue file. This value is part of queue file name
	 * (excluding path,
	 * extension)
	 * @return {const char*} Queue file key value
	 */
	const char* key(void) const
	{
		return m_partName;
	}

	/**
	 * Get full access path of queue file
	 * @return {const char*}
	 */
	const char* get_filePath(void) const
	{
		return m_filePath.c_str();
	}

	/**
	 * Get root path part of queue file (excluding queue directory)
	 * @return {const char*}
	 */
	const char* get_home(void) const
	{
		return m_home;
	}

	/**
	 * Get queue name of this queue file
	 * @return {const char*} Queue name
	 */
	const char* get_queueName(void) const
	{
		return m_queueName;
	}

	/**
	 * Get queue subdirectory
	 * @return {const char*} Queue subdirectory name
	 */
	const char* get_queueSub(void) const
	{
		return m_queueSub;
	}

	/**
	 * Get extension name of this queue file
	 * @return {const char*} Extension name
	 */
	const char* get_extName(void) const
	{
		return m_extName;
	}

	/**
	 * Get size of data already written
	 * @return {size_t}
	 */
	size_t get_fileSize() const
	{
		return nwriten_;
	}

private:
	friend class queue_manager;

	~queue_file();

	/**
	 * Create new queue file. After creation, will automatically create lock object
	 * for this file.
	 * Can directly call lock()/unlock() of this file
	 * @param home {const char*} Root path where queue file is located
	 * @param queueName {const char*} Queue name
	 * @param extName {const char*} Queue file extension name
	 * @param width {unsigned} Number of queue second-level directories
	 * @return {bool} Whether creating new queue file was successful. If returns
	 * false, it indicates
	 *  input path or extName is illegal
	 */
	bool create(const char* home, const char* queueName,
		const char* extName, unsigned width);

	/**
	 * Open existing queue file. After opening, will automatically create lock
	 * object for this file.
	 * Can directly call lock()/unlock() of this file
	 * @param filePath {const char*} Queue file path
	 * @return {bool} Whether opening queue file was successful
	 */
	bool open(const char* filePath);
	bool open(const char* home, const char* queueName, const char* queueSub,
		const char* partName, const char* extName);

	/**
	 * Close current file handle
	 */
	void close();

	/**
	 * Delete this queue file from disk
	 * @return {bool} Whether deletion was successful
	 */
	bool remove();

	/**
	 * Move queue file from current queue to target queue
	 * @param queueName {const char*} Target queue name
	 * @param extName {const char*} Target extension name
	 * @return {bool} Whether moving file was successful
	 */
	bool move_file(const char* queueName, const char* extName);

	/**
	 * Set queue name
	 * @param queueName {const char*} Queue name
	 */
	void set_queueName(const char* queueName);

	/**
	 * Set extension name of queue file
	 */
	void set_extName(const char* extName);

	/**
	 * Lock current queue file object (including mutex lock and file lock)
	 * @return {bool} Whether locking was successful
	 */
	bool lock(void);

	/**
	 * Unlock current queue file object (including mutex lock and file lock)
	 * @return {bool} Whether unlocking was successful
	 */
	bool unlock(void);

private:
	// File stream object
	fstream* m_fp;

	// Full path name of queue file relative to queue root directory
	string m_filePath;

	// Root path of queue file
	char  m_home[MAXPATH255];

	// Queue name
	char  m_queueName[32];

	// Subdirectory under queue
	char  m_queueSub[32];

	// Queue filename, excluding path, also excluding file extension
	char  m_partName[MAXPATH255];

	// Extension name of queue file
	char  m_extName[32];

	// Lock object
	locker m_locker;

	// Whether current file has already been locked
	bool  m_bLocked;

	// Whether file lock has already been opened
	bool  m_bLockerOpened;

	// Size of file data already written
	size_t nwriten_;
};

} // namespace acl

