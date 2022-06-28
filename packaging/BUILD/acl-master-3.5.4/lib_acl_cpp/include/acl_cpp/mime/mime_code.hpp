#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/pipe_stream.hpp"

#if !defined(ACL_MIME_DISABLE)

namespace acl {
	
class string;

class ACL_CPP_API mime_code : public pipe_stream
{
public:
	/**
	 * 构造函数
	 * @param addCrlf {bool} 非流式编码时是否在末尾添加 "\r\n"
	 * @param addInvalid {bool} 流式解码时是否遇到非法字符是否原样拷贝
	 * @param encoding_type {const char*} 编码类型标识符
	 */
	mime_code(bool addCrlf, bool addInvalid, const char* encoding_type);
	virtual ~mime_code(void) = 0;

	/**
	 * 获得编码类型标识符
	 * @return {const char*}
	 */
	const char* get_encoding_type(void) const
	{
		return encoding_type_;
	}

	/* 流式编码函数，使用方法： encode_update->encode_update->...->encode_finish */

	/**
	 * 编码过程, 添加源数据, 结果存于 out 中, 如果输入的
	 * 数据不满足编码时缓存条件, 则内部仅临时缓存而不做编码
	 * @param src {const char*} 源数据地址
	 * @param n {int} 源数据长度
	 * @param out {string*} 存储编码结果, 可以通过比较
	 *  调用此函数前后的 out->length() 来判断该函数是进行了
	 *  编码过程还是临时缓存过程; 如果使用了 out 中的结果数据，
	 *  则用完后应该调用 out->clear() 清空用过的数据
	 */
	virtual void encode_update(const char *src, int n,
		string* out);

	/**
	 * 编码结束后需要调用此函数来对可能存在于临时缓存中的
	 * 源数据进行最后的编码
	 * @param out {string*} 存储编码结果, 可以通过比较
	 *  调用此函数前后的 out->length() 来判断该函数是进行了
	 *  编码过程还是临时缓存过程; 如果使用了 out 中的结果数据，
	 *  则用完后应该调用 out->clear() 清空用过的数据
	 */
	virtual void encode_finish(string* out);

	/* 流式解码函数，使用方法： decode_update->decode_update->...->decode_finish */

	/**
	 * 解码过程, 添加经过编码的数据, 经本函数后进行解码, 如果
	 * 输入的数据不满足解码时的字节数条件, 则内部仅是临时缓存
	 * 该数据, 等满足解码条件时才进行解码
	 * @param src {const char*} 经过编码后的数据
	 * @param n {int} 数据长度
	 * @param out {string*} 存储解码结果, 可以通过比较
	 *  调用此函数前后的 out->length() 来判断该函数是进行了
	 *  解码过程还是临时缓存过程; 如果使用了 out 中的结果数据，
	 *  则用完后应该调用 out->clear() 清空用过的数据
	 */
	virtual void decode_update(const char *src, int n, string* out);

	/**
	 * 解码结束后需要调用此函数来对可能存在于临时缓存中的
	 * 源数据进行最后的解码
	 * @param out {string*} 存储解码结果, 可以通过比较
	 *  调用此函数前后的 out->length() 来判断该函数是进行了
	 *  解码过程还是临时缓存过程; 如果使用了 out 中的结果数据，
	 *  则用完后应该调用 out->clear() 清空用过的数据
	 */
	virtual void decode_finish(string* out);

	/**
	 * 重置内部缓冲区
	 */
	virtual void reset(void);

	/**
	 * 在编码过程中设置是否自动在每个编码段添加 "\r\n"
	 * @param on {bool}
	 */
	virtual void add_crlf(bool on);

	/**
	 * 在解码过程中如果遇到非法字符是否将其添加在解码结果中
	 * @param on {bool}
	 */
	virtual void add_invalid(bool on);

	/**
	 * 根据输入的编码表生成相应的解码表
	 * @param toTab {const unsigned char*} 编码表字符串
	 * @param out {string*} 存储结果
	 */
	static void create_decode_tab(const unsigned char *toTab, string *out);

	/**
	 * 如果子类未重载以上虚函数而因此使用基类的以上默认虚函数时
	 * 则子类必须调用此函数设置自己的编码表, 解码表及填充字符
	 * @param toTab {const unsigned char*} 编码表
	 * @param unTab {const unsigned char*} 解码表
	 * @param fillChar {unsigned char} 填充字符
	 */
	void init(const unsigned char* toTab,
		const unsigned char* unTab, unsigned char fillChar);

	/**
	 * 设置转码器的工作状态，因为该转码器由编码器和解码器组成，
	 * 所以在使用 pipe_stream 方式工作时必须指定该转码器的状态
	 * 以指定是处于编码器状态还是解码器状态
	 * @param encoding {bool} 如果为 true 表示为编码器状态，否则
	 *  为解码器状态
	 */
	void set_status(bool encoding = true);

	// pipe_stream 虚函数重载

	virtual int push_pop(const char* in, size_t len,
		string* out, size_t max = 0);
	virtual int pop_end(string* out, size_t max = 0);
	virtual void clear(void);

	/**
	 * 静态函数，根据编码类型 MIME_ENC_XXX (参见：mime_define.hpp) 获得
	 * 对应的编解码对象，当有当 encoding 类型为 MIME_ENC_QP,
	 * MIME_ENC_BASE64, MIME_ENC_UUCODE, MIME_ENC_XXCODE
	 * @param encoding {int} 编码类型，只能为 MIME_ENC_QP, MIME_ENC_BASE64,
	 *  MIME_ENC_UUCODE, MIME_ENC_XXCODE
	 * @param warn_unsupport {bool} 当未找到匹配的编码对象时是否记录警告信息
	 * @return {mime_code*} 编码对象，当未找到匹配的编码类型时返回 NULL
	 */
	static mime_code* create(int encoding, bool warn_unsupport = true);

private:
	void encode(string* out);
	void decode(string* out);

	char  m_encodeBuf[57];
	int   m_encodeCnt;
	char  m_decodeBuf[76];
	int   m_decodeCnt;
	bool  m_addCrLf;
	bool  m_addInvalid;
	bool  m_encoding;
	const unsigned char *m_toTab;
	const unsigned char *m_unTab;
	unsigned char m_fillChar;
	string* m_pBuf;
	char* encoding_type_;
};

} // namespace acl

#endif // !defined(ACL_MIME_DISABLE)
