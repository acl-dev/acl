#pragma once
#include <map>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/locker.hpp"
#include "acl_cpp/queue/queue_file.hpp"

typedef struct ACL_SCAN_DIR ACL_SCAN_DIR;

namespace acl {

class queue_file;

class ACL_CPP_API queue_manager
{
public:
	/**
	 * 队列对象的构造函数
	 * @param home {const char*} 队列的根目录
	 * @param queueName {const char*} 该队列对象的队列名称
	 */
	queue_manager(const char* home, const char* queueName,
		unsigned sub_width = 2);
	~queue_manager();

	/**
	 * 获得队列名
	 * @return {const char*}
	 */
	const char* get_queueName() const;

	/**
	 * 获得队列根目录
	 * @return {const char*}
	 */
	const char* get_home() const;

	/**
	 * 创建队列文件
	 * @param extName {const char*} 队列文件扩展名
	 * @return {queue_file*} 队列文件对象, 永远非NULL, 该返回值
	 *  为动态创建的, 所以用完后需要 delete 以释放其所占内存
	 */
	queue_file* create_file(const char* extName);

	/**
	 * 打开磁盘上存在的队列文件用于读/写
	 * @param path {const char*} 队列文件名
	 * @param no_cache {bool} 为 true 时，要求在缓存中该文件对应的 KEY
	 *  必须不存在，如果存在则返回 NULL 表示该文件正被锁定; 当该参数为
	 *  false 时，则可以直接使用缓存中的对象
	 * @return {queue_file*} 队列文件对象, 出错或不存在则返回 NULL
	 */
	queue_file* open_file(const char* path, bool no_cache = true);

	/**
	 * 关闭队列文件句柄, 并释放该文件对象，并不删除文件
	 * @param fp {queue_file*} 队列文件对象
	 * @return {bool} 关闭是否成功
	 */
	bool close_file(queue_file* fp);

	/**
	 * 从磁盘上删除队列文件, 并释放该文件对象
	 * @param fp {queue_file*} 队列文件对象
	 * @return {bool} 删除文件是否成功
	 */
	bool delete_file(queue_file* fp);

	/**
	 * 修改文件的扩展名
	 * @param fp {queue_file*} 队列文件对象
	 * @param extName {const char*} 新的扩展名
	 * @return {bool} 修改文件扩展名是否成功
	 */
	bool rename_extname(queue_file* fp, const char* extName);

	/**
	 * 将队列文件移至目标队列中, 移动成功后, 文件对象内部内容将会发生改变
	 * @param fp {queue_file*} 队列文件对象
	 * @param queueName {const char*} 目标队列名
	 * @param extName {const char*} 文件扩展名
	 * @return {bool} 移动队列文件是否成功, 如果移动失败, 则调用者应用调用
	 *  close_file 关闭该队列文件对象, 该文件将会被定时扫描任务移走
	 */
	bool move_file(queue_file* fp, const char* queueName, const char* extName);

	/**
	 * 将一个队列文件对象移至目标队列对象中
	 * @param fp {queue_file*} 队列文件对象
	 * @param toQueue {queue_manager*} 目标队列对象
	 * @param extName {const char*} 文件扩展名
	 * @return {bool} 移动队列文件是否成功, 如果移动失败, 则调用者应用调用
	 *  close_file 关闭该队列文件对象, 该文件将会被定时扫描任务移走
	 */
	bool move_file(queue_file* fp, queue_manager* toQueue, const char* extName);

	/**
	 * 从磁盘上删除本队列文件, 删除成功后该队列文件句柄已经被删除, 不可再用,
	 * 即使删除文件失败, 该队列文件对象也被释放, 只是从磁盘上删除该文件失败,
	 * 所以调用此函数后 fp 不能再次使用
	 * @param fp {queue_file*}
	 * @return {bool} 删除是否成功
	 */
	bool remove(queue_file* fp);

	/**
	* 检查所给文件名是否正在被使用
	* @param fileName {const char*} 文件名
	* @return {bool} 是否被使用
	*/
	bool busy(const char* fileName);

	/**
	* 在队列对象的缓存中查找某个队列文件对象
	* @param key {const char*} 队列文件的部分文件名(不含路径及扩展名)
	* @return {queue_file*} 返回 NULL 则表示未查到
	*/
	queue_file* cache_find(const char* key);

	/**
	* 向队列对象的缓存中添加某个队列文件对象
	* @param fp {queue_file*} 队列文件对象
	* @return {bool} 添加是否成功, 若失败则说明该对象或其对应的键值
	*  已经存在于缓存中
	*/
	bool cache_add(queue_file* fp);

	/**
	* 从队列对象的缓存中删除某个队列文件对象
	* @param key {const char*} 队列文件对象的键值
	* @return {bool} 删除是否成功, 若失败则说明该队列文件对象不存在
	*/
	bool cache_del(const char* key);

	/*-------------------- 与队列扫描相关的函数 ------------------------*/

	/**
	* 打开磁盘扫描队列
	* @param scanSub {bool} 是否递归扫描子目录
	* @return {bool} 打开队列是否成功
	*/
	bool scan_open(bool scanSub = true);

	/**
	* 关闭扫描队列
	*/
 	void scan_close();

	/**
	* 获得磁盘队列中的下一个队列文件, 若扫描完毕则返回空
	* @return {queue_file*} 扫描的队列文件对象, 返回空则表示扫描完毕
	*  或出错，非空对象一定要在用完后 delete 以释放内部分配的资源
	*/
	queue_file* scan_next(void);

	/**
	* 根据文件路径分析出队列名, 文件名(不含路径及扩展名部分), 文件扩展名
	* @param filePath {const char*} 文件全路径名
	* @param home {acl::string*} 存储文件所在的根目录
	* @param queueName {acl::string*} 存储文件所在的队列名
	* @param queueSub {acl::string*} 存储文件的队列子目录
	* @param partName {acl::string*} 存储文件的文件名部分(不含路径及扩展名)
	* @param extName {acl::string*} 存储文件的扩展名部分
	*/
	static bool parse_filePath(const char* filePath, acl::string* home,
		string* queueName, string* queueSub,
		string* partName, string* extName);

	/**
	* 根据文件名称(含扩展名但不含路径), 分析出文件名(不含路径及扩展名),
	* 和文件扩展名称
	*/
	static bool parse_fileName(const char* fileName, acl::string* partName,
		string* extName);

	/**
	* 分析路径, 从中提取出队列名称
	*/
	static bool parse_path(const char* path, acl::string* home,
		string* queueName, acl::string* queueSub);

	/**
	* 根据部分文件名(不含目录及扩展名)计算出其队列子目录路径(以数字表示)
	* @param partName {const char*} 部分文件名
	* @param width {unsigned} 队列二级目录的个数
	* @return {unsigned int} 队列子目录路径(以数字表示)
	*/
	static unsigned int hash_queueSub(const char* partName, unsigned width);

protected:
private:
	bool cache_check(queue_file* fp);

	//typedef struct ACL_SCAN_DIR ACL_SCAN_DIR;

	// 扫描目录的句柄
	ACL_SCAN_DIR* m_scanDir;
	string m_home;
	string m_queueName;
	unsigned sub_width_;

	std::map<string, queue_file*> m_queueList;
	locker m_queueLocker;
};

} // namespace acl
