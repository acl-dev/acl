#pragma once

class charset_radar
{
public:
	charset_radar(void);
	~charset_radar(void);

	/**
	 * 识别给定字符串的字符集
	 * @param data 需要识别的字符串
	 * @param len  字符串长度
	 * @param charset_result  识别的字符集
	 * @return {bool} 是否识别成功
	 */
	bool detact(const char *data, int len, acl::string &charset_result);
	bool detact(const acl::string &data, acl::string &charset_result);

	/*
	 * 设置是否开启调试模式
	 */
	void setDebugMode(bool flag)
	{
		debug_mode_ = flag;
	}

private:
	bool debug_mode_;
};

//bool format_utf8(const char *str, int len, acl::string &out);
