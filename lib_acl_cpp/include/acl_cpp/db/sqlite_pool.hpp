#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/db/db_pool.hpp"

namespace acl {

class db_handle;

class ACL_CPP_API sqlite_pool : public db_pool
{
public:
	/**
	 * ���캯��
	 * @param dbfile {const char*} sqlite ���ݿ�������ļ�
	 * @param dblimit {size_t} ���ݿ����ӳ��������������
	 * @param charset {const char*} �����ļ����ַ���
	 */
	sqlite_pool(const char* dbfile, size_t dblimit = 64,
		const char* charset = "utf-8");
	~sqlite_pool();

protected:
	// ���� connect_pool ���麯�����������ݿ����Ӿ��
	connect_client* create_connect();

	//@override
	void set_charset(const char* charset);

private:
	// sqlite �����ļ���
	char* dbfile_;
	// sqlite �����ļ������ַ���
	char* charset_;
};

} // namespace acl
