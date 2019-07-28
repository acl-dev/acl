#pragma once
#include "../acl_cpp_define.hpp"
#include "noncopyable.hpp"

namespace acl {

class istream;

class ACL_CPP_API md5 : public noncopyable
{
public:
	md5(void);
	~md5(void);

	/**
	 * 可以循环调用此函数添加需要被 md5 的数据
	 * @param dat {const void*} 数据地址
	 * @param len {size_t} dat 数据长度
	 * @return {md5&}
	 */
	md5& update(const void* dat, size_t len);

	/**
	 * 必须调用本函数表示 md5 过程结束
	 * @return {md5&}
	 */
	md5& finish(void);

	/**
	 * 重置 md5 算法器的状态，从而允许重复使用同一个 md5 对象
	 * @return {md5&}
	 */
	md5& reset(void);

	/**
	 * 获得二进制格式的 md5 结果值
	 * @return {const char*} 返回值永远非空，且缓冲区长度为 16 字节
	 */
	const char* get_digest() const;

	/**
	 * 获得以字符串形式表示的 m5 结果值
	 * @return {const char*} 返回值永远非空，且以 \0 结尾，且字符串
	 *  长度为 32 字节
	 */
	const char* get_string() const;

	/**
	 * 将数据用 md5 算法计算签名值，取得 128 位 (即 16 字节) 二进制结果
	 * @param dat {const void*} 源数据
	 * @param dlen {size_t} dat 数据长度
	 * @param key {const char*} 非空时做为键数据
	 * @param klen {size_t} key 非空时表示 key 的长度
	 * @param out {void*} 存储 md5 结果
	 * @param size {size_t} out 大小，至少应该为 16 字节
	 * @return {const char*} 返回存储结果的地址(即 out 地址)
	 */
	static const char* md5_digest(const void *dat, size_t dlen,
		const void *key, size_t klen, void* out, size_t size);

	/**
	 * 将数据用 md5 算法计算签名值，取得字符串形式的结果
	 * @param dat {const void*} 源数据
	 * @param dlen {size_t} dat 数据长度
	 * @param key {const char*} 非空时做为键数据
	 * @param klen {size_t} key 非空时表示 key 的长度
	 * @param out {void*} 存储 md5 结果
	 * @param size {size_t} out 大小，至少应该为 33 字节
	 * @return {const char*} 返回存储结果的地址(即 out 地址)，
	 *  且返回值为以 \0 结尾的 32 字节长度(不含 \0)字符串
	 */
	static const char* md5_string(const void *dat, size_t dlen,
		const void *key, size_t klen, char* out, size_t size);

	/**
	 * 将文件中的内容用 md5 算法计算签名值，并取得字符串形式结果
	 * @param path {const char*} 文件全路径
	 * @param key {const char*} 非空时做为键数据
	 * @param klen {size_t} key 非空时表示 key 的长度
	 * @param out {void*} 存储 md5 结果
	 * @param size {size_t} out 大小，至少应该为 33 字节
	 * @return {int64) 返回所读取的文件数据的长度，下列情况下返回 -1
	 *  1) 打开文件失败
	 *  2) 未从文件中读到数据
	 *  3) out 缓冲区大小 size 小于 33 字节长度
	 */
#if defined(_WIN32) || defined(_WIN64)
	static __int64 md5_file(const char* path, const void *key,
		size_t klen, char* out, size_t size);
#else
	static long long int md5_file(const char* path, const void *key,
		size_t klen, char* out, size_t size);
#endif

	/**
	 * 将文件中的内容用 md5 算法计算签名值，并取得字符串形式结果
	 * @param in {istream&} 输入文件流
	 * @param key {const char*} 非空时做为键数据
	 * @param klen {size_t} key 非空时表示 key 的长度
	 * @param out {void*} 存储 md5 结果
	 * @param size {size_t} out 大小，至少应该为 33 字节
	 * @return {int64) 返回所读取的文件数据的长度，下列情况下返回 -1:
	 *  1) 未从输入流中读取数据时
	 *  2) out 缓冲区大小 size 小于 33 字节长度
	 */
#if defined(_WIN32) || defined(_WIN64)
	static __int64 md5_file(istream& in, const void *key,
		size_t klen, char* out, size_t size);
#else
	static long long int md5_file(istream& in, const void *key,
		size_t klen, char* out, size_t size);
#endif

	/**
	 * 将 16 字节长度的 MD5 二进制结果转换为 32 字节长度的字符串
	 * @param in {const void*} 128 位(即 16 字节)的 md5 值，即 in 的数据长度
	 *  至少应该 >= 16，否则会引起内存起越界
	 * @param out {char*} 存储字符串形式的结果
	 * @param size {size_t} out 内存大小，至少为 33 字节，否则内部产生断言
	 * @return {const char*} 返回存储结果的地址(即 out 地址)，
	 *  且返回值为以 \0 结尾的 32 字节长度(不含 \0)字符串
	 */
	static const char* hex_encode(const void *in, char* out, size_t size);

private:
	unsigned int buf_[4];
	unsigned int bytes_[2];
	unsigned int in_[16];

	unsigned char digest_[16];
	unsigned char digest_s_[33];
};

}  // namespace acl
