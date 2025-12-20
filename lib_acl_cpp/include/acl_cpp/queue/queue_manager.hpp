#pragma once
#include <map>
#include "../stdlib/string.hpp"
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"
#include "queue_file.hpp"

typedef struct ACL_SCAN_DIR ACL_SCAN_DIR;

namespace acl {

class queue_file;

class ACL_CPP_API queue_manager : public noncopyable
{
public:
	/**
	 * Queue object constructor
	 * @param home {const char*} Root directory of queue
	 * @param queueName {const char*} Queue name of this queue object
	 */
	queue_manager(const char* home, const char* queueName,
		unsigned sub_width = 2);
	~queue_manager();

	/**
	 * Get queue name
	 * @return {const char*}
	 */
	const char* get_queueName() const;

	/**
	 * Get queue root directory
	 * @return {const char*}
	 */
	const char* get_home() const;

	/**
	 * Create queue file
	 * @param extName {const char*} Queue file extension name
	 * @return {queue_file*} Queue file object. Always non-NULL. This return value
	 *  is dynamically created, so need to delete after use to release its memory
	 */
	queue_file* create_file(const char* extName);

	/**
	 * Open existing queue file on disk for read/write
	 * @param path {const char*} Queue filename
	 * @param no_cache {bool} When true, requires that KEY corresponding to this
	 * file in cache
	 * must not exist. If exists, returns NULL indicating this file is locked. When
	 * this parameter is
	 *  false, can directly use object in cache
	 * @return {queue_file*} Queue file object. Returns NULL if error or does not
	 * exist
	 */
	queue_file* open_file(const char* path, bool no_cache = true);

	/**
	 * Close queue file handle and release this file object, does not delete file
	 * @param fp {queue_file*} Queue file object
	 * @return {bool} Whether closing was successful
	 */
	bool close_file(queue_file* fp);

	/**
	 * Delete queue file from disk and release this file object
	 * @param fp {queue_file*} Queue file object
	 * @return {bool} Whether file deletion was successful
	 */
	bool delete_file(queue_file* fp);

	/**
	 * Modify file's extension name
	 * @param fp {queue_file*} Queue file object
	 * @param extName {const char*} New extension name
	 * @return {bool} Whether modifying file extension name was successful
	 */
	bool rename_extname(queue_file* fp, const char* extName);

	/**
	 * Move queue file to target queue. After move succeeds, file object's internal
	 * content will change
	 * @param fp {queue_file*} Queue file object
	 * @param queueName {const char*} Target queue name
	 * @param extName {const char*} File extension name
	 * @return {bool} Whether moving queue file was successful. If move fails,
	 * caller should call
	 * close_file to close this queue file object. This file will be moved by
	 * scheduled scanning task
	 */
	bool move_file(queue_file* fp, const char* queueName, const char* extName);

	/**
	 * Move a queue file object to target queue object
	 * @param fp {queue_file*} Queue file object
	 * @param toQueue {queue_manager*} Target queue object
	 * @param extName {const char*} File extension name
	 * @return {bool} Whether moving queue file was successful. If move fails,
	 * caller should call
	 * close_file to close this queue file object. This file will be moved by
	 * scheduled scanning task
	 */
	bool move_file(queue_file* fp, queue_manager* toQueue, const char* extName);

	/**
	 * Delete this queue file from disk. After deletion succeeds, this queue file
	 * handle has been deleted and cannot be used again.
	 * Even if file deletion fails, this queue file object is also released, only
	 * file deletion from disk failed.
	 * So after calling this function, fp cannot be used again
	 * @param fp {queue_file*}
	 * @return {bool} Whether deletion was successful
	 */
	bool remove(queue_file* fp);

	/**
	* Check whether given filename is currently being used
	* @param fileName {const char*} Filename
	* @return {bool} Whether it is being used
	*/
	bool busy(const char* fileName);

	/**
	* Find a queue file object in queue object's cache
	* @param key {const char*} Partial filename of queue file (excluding path and
	* extension)
	* @return {queue_file*} Returns NULL indicates not found
	*/
	queue_file* cache_find(const char* key);

	/**
	* Add a queue file object to queue object's cache
	* @param fp {queue_file*} Queue file object
	* @return {bool} Whether adding was successful. If fails, it indicates this
	* object or its corresponding key value
	*  already exists in cache
	*/
	bool cache_add(queue_file* fp);

	/**
	* Delete a queue file object from queue object's cache
	* @param key {const char*} Key value of queue file object
	* @return {bool} Whether deletion was successful. If fails, it indicates this
	* queue file object does not exist
	*/
	bool cache_del(const char* key);

	/*-------------------- Functions related to queue scanning ------------------------*/

	/**
	* Open queue for disk scanning
	* @param scanSub {bool} Whether to recursively scan subdirectories
	* @return {bool} Whether opening queue was successful
	*/
	bool scan_open(bool scanSub = true);

	/**
	* Close scanning queue
	*/
 	void scan_close();

	/**
	* Get next queue file in disk queue. Returns empty if scanning is complete
	* @return {queue_file*} Scanned queue file object. Returns empty indicates
	* scanning complete
	* or error occurred. Non-empty object must be deleted after use to release
	* internally allocated resources
	*/
	queue_file* scan_next(void);

	/**
	* Parse queue name, filename (excluding path and extension parts), file
	* extension name from file path
	* @param filePath {const char*} Full file path name
	* @param home {acl::string*} Store root directory where file is located
	* @param queueName {acl::string*} Store queue name where file is located
	* @param queueSub {acl::string*} Store queue subdirectory of file
	* @param partName {acl::string*} Store filename part of file (excluding path
	* and extension)
	* @param extName {acl::string*} Store extension name part of file
	*/
	static bool parse_filePath(const char* filePath, acl::string* home,
		string* queueName, string* queueSub,
		string* partName, string* extName);

	/**
	* Parse filename (excluding path and extension), file extension name from file
	* name (including extension but excluding path)
	*/
	static bool parse_fileName(const char* fileName, acl::string* partName,
		string* extName);

	/**
	* Parse path, extract queue name from it
	*/
	static bool parse_path(const char* path, acl::string* home,
		string* queueName, acl::string* queueSub);

	/**
	* Calculate queue subdirectory path (represented as number) based on partial
	* filename (excluding directory and extension)
	* @param partName {const char*} Partial filename
	* @param width {unsigned} Number of queue second-level directories
	* @return {unsigned int} Queue subdirectory path (represented as number)
	*/
	static unsigned int hash_queueSub(const char* partName, unsigned width);

protected:
private:
	bool cache_check(queue_file* fp);

	//typedef struct ACL_SCAN_DIR ACL_SCAN_DIR;

	// Handle for scanning directory
	ACL_SCAN_DIR* m_scanDir;
	string m_home;
	string m_queueName;
	unsigned sub_width_;

	std::map<string, queue_file*> m_queueList;
	locker m_queueLocker;
};

} // namespace acl

