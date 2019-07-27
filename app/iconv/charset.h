#pragma once

class charset_radar
{
public:
	charset_radar(void);
	~charset_radar(void);

	/**
	 * 璇嗗埆缁欏畾瀛楃涓茬殑瀛楃闆�
	 * @param data 闇€瑕佽瘑鍒殑瀛楃涓�
	 * @param len  瀛楃涓查暱搴�
	 * @param charset_result  璇嗗埆鐨勫瓧绗﹂泦
	 * @return {bool} 鏄惁璇嗗埆鎴愬姛
	 */
	bool detact(const char *data, int len, acl::string &charset_result);
	bool detact(const acl::string &data, acl::string &charset_result);

	/*
	 * 璁剧疆鏄惁寮€鍚皟璇曟ā寮�
	 */
	void setDebugMode(bool flag)
	{
		debug_mode_ = flag;
	}

private:
	bool debug_mode_;
};

//bool format_utf8(const char *str, int len, acl::string &out);
