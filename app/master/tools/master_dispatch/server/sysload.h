#pragma once

class sysload {

public:
	sysload() {}
	~sysload() {}

	/**
	 * 获得系统当前的负载
	 * @param out {acl::string*} 如果非空，则存储字符串格式的结果
	 * @return {double}
	 */
	static double get_load(acl::string* out);
};
