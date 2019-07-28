#pragma once
#include "noncopyable.hpp"

struct ACL_SCAN_DIR;

namespace acl
{

class string;

class ACL_CPP_API scan_dir : public noncopyable
{
public:
	scan_dir(void);
	virtual ~scan_dir(void);

	/**
	 * 打开目录
	 * @param path {const char*} 目录路径，非空指针
	 * @param recursive {bool} 是否允许递归扫描目录
	 * @param rmdir_on {bool} 当目录为空时，是否需要删除该空目录
	 * @return {bool} 打开目录是否成功
	 */
	bool open(const char* path, bool recursive = true, bool rmdir_on = false);

	/**
	 * 虚方法，当需要删除空目录时，子类可以实现此虚方法来删除传入的目录，
	 * 本虚方法内部会自动调用 rmdir 删除空目录
	 * @param path {const char*} 需要被删除的空目录
	 * @return {bool} 删除目录是否成功
	 */
	virtual bool rmdir_callback(const char* path);

	/**
	 * 关闭目录，同时释放内部资源
	 */
	void close(void);

	/**
	 * 扫描下一个文件(遇到目录会自动跳过)，当在 open 指定了允许递归扫描选项
	 * (即 recursive = true)，则该函数会递归扫描所打开目录的所有子目录
	 * @param full {bool} 是否需要返回文件全路径
	 * @return {const char*} 非 NULL 表示所扫描到的文件名，否则表示扫描完毕
	 *  或目录还未打开
	 */
	const char* next_file(bool full = false);

	/**
	 * 扫描下一个目录(遇到文件或 "." 或 ".." 会跳过)，当在 open 指定允许了
	 * 允许递归扫描项(即 recursive = true)，则该函数会递归扫描所开目录的所
	 * 有子目录
	 * @param full {bool} 是否需要返回目录全路径
	 * @return {const char*} 非 NULL 表示所扫描到的目录名，否则表示扫描完
	 *  毕或目录还未打开
	 */
	const char* next_dir(bool full = false);

	/**
	 * 扫描下一个目录或文件，当在 open 指定了允许递归扫描项(即 resursive
	 * = true)，则该函数会递归扫描所打开目录的所有子目录及文件
	 * @param full {bool} 是否需要返回目录或文件的全路径，如果为 true 则返
	 *  回全路径，否则只返回文件名或目录名且都不含路径
	 * @param is_file {bool*} 当返回结果非空时，该地址存储的值表示所扫描到
	 *  的是否是文件，如果为 true 则为文件，否则为目录
	 * @return {const char*} 非 NULL 表示所扫描到的目录名或文件名，否则表
	 *  示扫描完毕或目录还未打开 
	 */
	const char* next(bool full = false, bool* is_file = NULL);

	/**
	 * 获得当前扫描过程所在的目录路径，返回的路径尾部不包含路径分隔符 '/'
	 * 或 '\\' (win32)，如对于路径：/home/zsx/，则会返回 /home/zsx，如果
	 * 路径为根路径：/ 则该 '/' 将会保留；在 _WIN32 下，返回类似于
	 * C:\Users\zsx 的路径
	 * @return {const char*} 当目录打开时该函数返回非空指针，否则返回 NULL
	 */
	const char* curr_path();

	/**
	 * 获得当前程序扫描过程所扫到的文件名
	 * @param full {bool} 是否需要同时返回文件全路径
	 * @return {bool} 如果目录未找开或当前扫描的不是文件，则返回 NULL
	 */
	const char* curr_file(bool full = false);

	/**
	 * 返回当前已经扫描的目录的个数
	 * @return {size_t}
	 */
	size_t dir_count() const;

	/**
	 * 返回当前已经扫描的文件的个数
	 * @return {size_t}
	 */
	size_t file_count() const;

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 获得当前已经扫描的文件及目录大小的总和
	 * @return {acl_uint64}
	 */
	unsigned __int64 scaned_size() const;

	/**
	 * open 成功后调用本函数统计根目录下所有文件及目录大小的总和
	 * @param nfiles {int*} 非空时存储被删除的文件个数
	 * @param ndirs {int*} 非空时存储被删除的目录个数
	 * @return {acl_uint64} 根目录下所有文件及目录大小的总和
	 */
	unsigned __int64 all_size(int* nfiles = NULL, int* ndirs = NULL) const;

	/**
	 * 根目录下所有文件及目录大小的总和
	 * @param path {const char*} 需要被删除的根目录
	 * @param recursive {bool} 是否递归扫描所有目录
	 * @param nfiles {int*} 非空时存储被删除的文件个数
	 * @param ndirs {int*} 非空时存储被删除的目录个数
	 * @return {acl_uint64} 根目录下所有文件及目录大小的总和
	 */
	static unsigned __int64 all_size(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);

	/**
	 * open 成功后调用本函数删除根目录下的所有文件及目录
	 * @param nfiles {int*} 非空时存储被删除的文件个数
	 * @param ndirs {int*} 非空时存储被删除的目录个数
	 * @return {acl_unint64} 删除的所有文件及目录大小总和(字节单位)
	 */
	unsigned __int64 remove_all(int* nfiles = NULL, int* ndirs = NULL) const;

	/**
	 * 删除根目录下的所有文件及目录
	 * @param path {const char*} 需要被删除的根目录
	 * @param recursive {bool} 是否递归扫描所有目录
	 * @param nfiles {int*} 非空时存储被删除的文件个数
	 * @param ndirs {int*} 非空时存储被删除的目录个数
	 * @return {acl_unint64} 删除的所有文件及目录大小总和(字节单位)
	 */
	static unsigned __int64 remove_all(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);
#else
	unsigned long long scaned_size() const;
	unsigned long long all_size(int* nfiles = NULL, int* ndirs = NULL) const;
	static unsigned long long all_size(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);
	unsigned long long remove_all(int* nfiles = NULL, int* ndirs = NULL) const;
	static unsigned long long remove_all(const char* path, bool recursive = true,
		int* nfiles = NULL, int* ndirs = NULL);
#endif

	/**
	 * 获得当前程序运行的路径
	 * @param out {string&} 存储结果
	 * @return {bool} 是否成功获得当前程序运行路径
	 */
	static bool get_cwd(string& out);

public:
	ACL_SCAN_DIR* get_scan_dir(void) const
	{
		return scan_;
	}

	/**
	 * 设置回调方法，用来删除空目录
	 * @param fn {int (*)(ACL_SCAN_DIR*, const char*, void*)}
	 * @param ctx {void*}
	 */
	void set_rmdir_callback(int (*fn)(ACL_SCAN_DIR*, const char*, void*), void* ctx);

private:
	char* path_;
	ACL_SCAN_DIR* scan_;
	string* path_buf_;
	string* file_buf_;

	static int rmdir_def(ACL_SCAN_DIR* scan, const char* path, void* ctx);
};

}  // namespace acl
