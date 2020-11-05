#pragma once
#include "../acl_cpp_define.hpp"
#include "pipe_stream.hpp"

typedef struct z_stream_s z_stream;

namespace acl {

/**
 * 压缩级别类型定义，该集合定义了压缩速度及压缩比的一个可选方式
 * 所选的压缩值越高，则压缩比会更大，但压缩速度会越低
 */
typedef enum
{
	zlib_default = -1,      // 缺省的压缩比
	zlib_level0 = 0,        // 最低的压缩比，其实就是不压缩
	zlib_best_speed = 1,    // 压缩速度最快的压缩比
	zlib_level1 = zlib_best_speed,
	zlib_level2 = 2,
	zlib_level3 = 3,
	zlib_level4 = 4,
	zlib_level5 = 5,
	zlib_level6 = 6,
	zlib_level7 = 7,
	zlib_level8 = 8,
	zlib_best_compress = 9, // 最高的压缩比，最低的压缩速度
	zlib_level9 = zlib_best_compress
} zlib_level_t;

/**
 * 压缩过程中的压缩窗口参数类型，值越大则压缩效果越好且占用内存越多，
 * 针对 HTTP 压缩传输，需要设置这些值的负值：-zlib_wbits_t
 */
enum
{
	zlib_wbits_8  = 8,
	zlib_wbits_9  = 9,
	zlib_wbits_10 = 10,
	zlib_wbits_11 = 11,
	zlib_wbits_12 = 12,
	zlib_wbits_13 = 13,
	zlib_wbits_14 = 14,
	zlib_wbits_15 = 15,
};

/**
 * 压缩过程中的内存分配策略，值越大使用内存越多
 */
typedef enum
{
	zlib_mlevel_1 = 1,
	zlib_mlevel_2 = 2,
	zlib_mlevel_3 = 3,
	zlib_mlevel_4 = 4,
	zlib_mlevel_5 = 5,
	zlib_mlevel_6 = 6,
	zlib_mlevel_7 = 7,
	zlib_mlevel_8 = 8,
	zlib_mlevel_9 = 9,
} zlib_mlevel_t;

/**
 * 压缩或解压过程中的缓存模式，即在压缩或解压过程中是否立刻刷新
 * 到缓冲区为了获得比较高的压缩比，应该选择 zlib_flush_off 方式
 */
typedef enum
{
	zlib_flush_off = 0,     // 不立即刷新至用户缓存
	zlib_flush_partial = 1, // 刷新部分至用户缓存
	zlib_flush_sync = 2,    // 同步刷新
	zlib_flush_full = 3,    // 完全刷新
	zlib_flush_finish = 4   // 完全刷新并停止压缩或解压过程
} zlib_flush_t;

class string;

class ACL_CPP_API zlib_stream : public pipe_stream
{
public:
	zlib_stream(void);
	~zlib_stream(void);

	/**
	 * 非流式压缩
	 * @param in {const char*} 源数据
	 * @param len {int} 源数据长度
	 * @param out {string*} 存储压缩结果的用户缓冲区
	 * @param level {zlib_level_t} 压缩级别，级别越高则压缩比越高，
	 *  但压缩速度越低
	 * @return {bool} 压缩过程是否成功
	 */
	bool zlib_compress(const char* in, int len, string* out,
		zlib_level_t level = zlib_default);

	/**
	 * 非流式解压缩
	 * @param in {const char*} 源压缩数据
	 * @param len {int} 源数据长度
	 * @param out {string*} 存储解压缩结果的用户缓冲区
	 * @param have_zlib_header {bool} 是否有 zlib_header 头，对
	 *  HTTP 传输协议而言应该将此值设为 false
	 * @param wsize {int} 解压过程中的滑动窗口大小
	 * @return {bool} 解压缩过程是否成功
	 */
	bool zlib_uncompress(const char* in, int len, string* out,
		bool have_zlib_header = true, int wsize = 15);

	///////////////////////////////////////////////////////////////
	//
	//           以下为流式压缩和流式解压缩过程
	//
	///////////////////////////////////////////////////////////////

	/**
	 * 开始压缩过程，如果采用流式压缩方式，则调用顺序必须是：
	 * zip_begin->zip_update->zip_finish，如果中间任何一个
	 * 过程失败，则应该调用 zip_reset
	 * @param level {zlib_level_t} 压缩级别，级别越高，则压缩比
	 *  越高，但压缩速度越低
	 * @param wbits {zlib_wbits_t} 压缩过程中的滑动窗口级别，值越大，则
	 *  压缩效率越高且使用内存越多，针对 HTTP 数据压缩传输，应该采用该
	 *  值的负值，如：-zlib_wbits_15
	 * @param mlevel {zlib_mlevel_t} 压缩过程中的内存分配策略，值越大，
	 *  则压缩效率越高且内存使用越多
	 * @return {bool} 压缩初始化过程是否成功，失败的原因一般
	 *  应该是输入的参数非法
	 */
	bool zip_begin(zlib_level_t level = zlib_default,
		int wbits = zlib_wbits_15,
		zlib_mlevel_t mlevel = zlib_mlevel_9);

	/**
	 * 循环调用此函数对源数据进行压缩
	 * @param in {const char*} 源数据
	 * @param len {int} 源数据长度
	 * @param out {string*} 用户缓冲区，该函数以添加方式往用户
	 *  提供的缓冲区中添加压缩的结果，用户应该自行判断调用本函数
	 *  前后的缓冲区长度，以确定由该函数添加的数据长度，由于所
	 *  选择的 zlib_flush_t 的不同，该缓冲区中数据可能未必存取
	 *  所有的结果
	 * @param flag {zlib_flush_t} 压缩过程中的数据缓冲方式
	 *  zlib_flush_off: 数据结果可能不会立即刷新至用户缓冲区，
	 *    zlib 库本身决定刷新的方式，从而可能会获得较高的压缩比
	 *  zlib_flush_partial: 数据结果可能会部分刷新至用户缓冲区
	 *  zlib_flush_sync: 数据数据同步刷新至用户缓冲区
	 *  zlib_flush_full: 将 zlib 库缓冲数据结果全部刷新至用户缓冲区
	 *  zlib_flush_finish: 调用本参数后表明压缩过程结束，同时会将
	 *    所有结果数据刷新至用户缓冲区，一般该参数不需要调用，因为在
	 *    调用 zip_finish 后，会自动将所有的缓冲数据刷新至用户缓冲区
	 * @return {bool} 压缩过程是否失败
	 */
	bool zip_update(const char* in, int len, string* out,
		zlib_flush_t flag = zlib_flush_off);

	/**
	 * 调用本函数表示压缩过程结束
	 * @param out {string} 用户缓冲区，该函数会将 zlib 库缓冲区中
	 *  的数据以添加的方式都刷新至用户缓冲区
	 * @return {bool} 是否成功
	 */
	bool zip_finish(string* out);

	/**
	 * 重置压缩器状态，一般只有当压缩过程出错时才会调用本函数
	 * @return {bool} 是否成功
	 */
	bool zip_reset(void);

	/**
	 * 在压缩过程中可使用此函数计算数据的 crc32 校验值
	 * @param n {unsigned} 上次计算的校验和值，第一次时可写 0
	 * @param buf {const void*} 需要校验的数据地址，第一次使用时写 NULL
	 * @param dlen {size_t} buf 数据的长度，第一次使用时写 0
	 * @return {unsinged} 本次计算的校验和值
	 */
	unsigned crc32_update(unsigned n, const void* buf, size_t dlen);

	/**
	 * 开始解压缩过程，如果采用流式解压缩方式，则调用顺序必须是：
	 * unzip_begin->unzip_update->unzip_finish，如果中间任何一个
	 * 过程失败，则应该调用 unzip_reset
	 * @param have_zlib_header {bool} 是否有 zlib_header 头，对
	 *  HTTP 传输协议而言应该将此值设为 false
	 * @param wsize {int} 解压过程中的滑动窗口大小
	 * @return {bool} 解压缩初始化过程是否成功，失败的原因一般
	 *  应该是输入的参数非法
	 */
	bool unzip_begin(bool have_zlib_header = true, int wsize = 15);

	/**
	 * 循环调用此函数对源数据进行解压缩
	 * @param in {const char*} 压缩的源数据
	 * @param len {int} 源数据长度
	 * @param out {string*} 用户缓冲区，该函数以添加方式往用户
	 *  提供的缓冲区中添加解压的结果，用户应该自行判断调用本函数
	 *  前后的缓冲区长度，以确定由该函数添加的数据长度，由于所
	 *  选择的 zlib_flush_t 的不同，该缓冲区中数据可能未必存取
	 *  所有的结果
	 * @param flag {zlib_flush_t} 解压缩过程中的数据缓冲方式
	 *  zlib_flush_off: 数据结果可能不会立即刷新至用户缓冲区，
	 *    zlib 库本身决定刷新的方式
	 *  zlib_flush_partial: 数据结果可能会部分刷新至用户缓冲区
	 *  zlib_flush_sync: 数据数据同步刷新至用户缓冲区
	 *  zlib_flush_full: 将 zlib 库缓冲数据结果全部刷新至用户缓冲区
	 *  zlib_flush_finish: 调用本参数后表明解压缩过程结束，同时会将
	 *    所有结果数据刷新至用户缓冲区，一般该参数不需要调用，因为在
	 *    调用 zip_finish 后，会自动将所有的缓冲数据刷新至用户缓冲区
	 * @return {bool} 解压缩过程是否失败
	 */
	bool unzip_update(const char* in, int len, string* out,
		zlib_flush_t flag = zlib_flush_off);

	/**
	 * 调用本函数表示解压缩过程结束
	 * @param out {string} 用户缓冲区，该函数会将 zlib 库缓冲区中
	 *  的数据以添加的方式都刷新至用户缓冲区
	 * @return {bool} 是否成功
	 */
	bool unzip_finish(string* out);

	/**
	 * 重置解压缩器状态，一般只有当解压缩过程出错时才会调用本函数
	 * @return {bool} 是否成功
	 */
	bool unzip_reset(void);

	/**
	 * 获得当前的 zstream 对象
	 * @return {z_stream*}
	 */
	z_stream* get_zstream(void) const
	{
		return zstream_;
	}

	/**
	 * 当采用动态加载方式加载动态库时，可以使用此函数设置动态库的加载全路径
	 */
	static void set_loadpath(const char* path);

	/**
	 * 当设置了动态库的动态加载全路径时，可以通过本函数获得动态库加载全路径
	 * @return {const char*} 当未设置时则返回 NULL
	 */
	static const char* get_loadpath(void);

	/**
	 * 手动调用动态加载 zlib 库方法，如果为静态链接，则无需调用本方法，
	 * 此外，如果 zlib 动态库不在程序运行目录下时，需要先调用上面的
	 * set_loadpath() 方法设置 zlib 动态库的全路径；
	 * 该方法内部通过 pthread_once() 保证互斥调用，所以即使同时被多个
	 * 线程调用也是安全的
	 * @return {bool} 加载是否成功
	 */
	static bool zlib_load_once(void);

	///////////////////////////////////////////////////////////////

	bool pipe_zip_begin(zlib_level_t level = zlib_default,
		zlib_flush_t flag = zlib_flush_off);
	bool pipe_unzip_begin(zlib_flush_t flag = zlib_flush_off);

	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);

private:
	z_stream* zstream_;
	bool finished_;
	bool is_compress_;
	zlib_flush_t flush_;

	bool update(int (*func)(z_stream*, int), zlib_flush_t flag,
		const char* in, int len, string* out);
	bool flush_out(int (*func)(z_stream*, int),
		zlib_flush_t flag, string* out);
};

} // namespace acl
