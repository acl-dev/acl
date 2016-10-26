#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/db/db_pool.hpp"

namespace acl {

class db_handle;
class mysql_conf;

class ACL_CPP_API mysql_pool : public db_pool
{
public:
	/**
	 * ���� mysql ���ݿ�ʱ�Ĺ��캯��
	 * @param dbaddr {const char*} mysql ��������ַ����ʽ��IP:PORT��
	 *  �� UNIX ƽ̨�¿���Ϊ UNIX ���׽ӿ�
	 * @param dbname {const char*} ���ݿ���
	 * @param dbuser {const char*} ���ݿ��û�
	 * @param dbpass {const char*} ���ݿ��û�����
	 * @param dblimit {int} ���ݿ����ӳص��������������
	 * @param dbflags {unsigned long} mysql ���λ
	 * @param auto_commit {bool} �Ƿ��Զ��ύ
	 * @param conn_timeout {int} �������ݿⳬʱʱ��(��)
	 * @param rw_timeout {int} �����ݿ�ͨ��ʱ��IOʱ��(��)
	 * @param charset {const char*} �������ݿ���ַ���(utf8, gbk, ...)
	 */
	mysql_pool(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		int dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60, const char* charset = "utf8");

	/**
	 * ���캯��
	 * @param conf {const mysql_conf&} mysql ���ݿ��������ö���
	 */
	mysql_pool(const mysql_conf& conf);
	~mysql_pool();

protected:
	// ���� connect_pool ���麯�����������ݿ����Ӿ��
	connect_client* create_connect();

	//@override
	void set_charset(const char* charset);

private:
	mysql_conf* conf_;
};

} // namespace acl
