#pragma once

class mail_object : public acl::diff_object
{
public:
	/**
	 * 鏋勯€犲嚱鏁�
	 * @param manager {diff_manager&}
	 * @param key {const char*} 浠ュ瓧绗︿覆鏂瑰紡琛ㄧず鐨勯敭锛岄潪绌哄瓧绗︿覆
	 * @param val {const char*} 浠ュ瓧绗︿覆鏂瑰紡琛ㄧず鐨勫€硷紝闈炵┖瀛楃涓�
	 */
	mail_object(acl::diff_manager& manager, const char* key, const char* val);

	void set_ctime(long long n);

public:
	// override: 鍩虹被绾櫄鍑芥暟鐨勫疄鐜�
	const char* get_key() const;

	// override: 鍩虹被绾櫄鍑芥暟鐨勫疄鐜�
	const char* get_val() const;

	// override: 鍩虹被绾嚱鏁扮殑瀹炵幇
	bool operator== (const acl::diff_object& obj) const;

	// @override
	bool check_range(long long from, long long to) const;

private:
	const char* key_;
	const char* val_;
	long long ctime_;

	// 鏋愭瀯鍑芥暟澹版槑涓虹鏈夌殑锛屼粠鑰岃姹傚姩鎬佸垱寤烘湰绫诲璞�
	~mail_object();
};
