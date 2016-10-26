#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/db/db_handle.hpp"

typedef struct sqlite3 sqlite3;

namespace acl {

class charset_conv;

class ACL_CPP_API db_sqlite : public db_handle
{
public:
	/**
	 * ���캯��
	 * @param charset {const char*} �����ַ���(gbk, utf-8, ...)
	 */
	db_sqlite(const char* dbfile, const char* charset = "utf-8");
	~db_sqlite(void);

	/**
	 * ���ص�ǰ�� sqlite �İ汾��Ϣ
	 */
	const char* version(void) const;

	/**
	 * �����ݿ�򿪺�ͨ���˺��������ݿ�Ĳ�������������ã�
	 * �������õ�������Ҫ�ϸ���ѭ sqlite ���������ѡ��Ҫ��
	 * @param pragma {const char*} ����ѡ�����ݣ���ʽΪ��
	 *  PRAGMA xxx=xxx
	 *  �磺PRAGMA synchronous = NORMAL
	 * @return {bool} �������ݿ��Ƿ�ɹ�
	 */
	bool set_conf(const char* pragma);

	/**
	 * �����ݿ�򿪵��ô˺�������������������ѡ��
	 * @param pragma {const char*} ����ѡ�����ݣ���ʽΪ��
	 *  PRAGMA xxx
	 *  �磺PRAGMA synchronous
	 * @param out {string&} �������ֵ�ǿ���洢���
	 * @return {const char*} Ϊ����˵�������ò����ڻ����ݿ�δ��
	 */
	const char* get_conf(const char* pragma, string& out);

	/**
	 * �����ݿ�򿪵�������������ݿ����������ѡ��
	 * @param pragma {const char*} ָ��������ѡ�����ò���Ϊ�գ�
	 *  ��������е�����ѡ���ʽΪ��PRAGMA xxx���磺PRAGMA synchronous
	 */
	void show_conf(const char* pragma = NULL);

	/**
	 * �����ݿ�򿪺����е�Ӱ��ļ�¼����
	 * @return {int} Ӱ���������-1 ��ʾ����
	 */
	int affect_total_count() const;

	/**
	 * ֱ�ӻ�� sqlite �ľ����������� NULL ���ʾ sqlite ��û�д�
	 * �����ʱ�ڲ��Զ��ر��� sqlite
	 * @return {sqlite3*}
	 */
	sqlite3* get_conn() const
	{
		return db_;
	}

	/********************************************************************/
	/*            ����Ϊ���� db_handle ����ӿ�                         */
	/********************************************************************/

	/**
	 * �������ݿ����������
	 * @return {const char*}
	 */
	const char* dbtype() const;

	/**
	 * ����ϴ����ݿ�����ĳ�������
	 * @return {int}
	 */
	int get_errno() const;

	/**
	 * ����ϴ����ݿ�����ĳ��������
	 * @return {const char*}
	 */
	const char* get_error() const;

	/**
	 * ���� db_handle �Ĵ���ӿ�
	 * @param charset {const char*} �����ݿ�����ʱ���õ��ַ���������
	 *  �����ǿ�ʱ���Ḳ�ǹ��캯���д�����ַ���
	 * @return {bool} ���Ƿ�ɹ�
	 */
	bool dbopen(const char* charset = NULL);

	/**
	 * ���� db_handle �Ĵ���ӿڣ����ݿ��Ƿ��Ѿ�����
	 * @return {bool} ���� true �������ݿ��Ѿ�����
	 */
	bool is_opened() const;

	/**
	 * ���� db_handle �Ĵ���ӿ�
	 * @return {bool} �ر��Ƿ�ɹ�
	 */
	bool close(void);

	/**
	 * ���� db_handle �Ĵ���ӿڣ��������ʵ�ִ˽ӿ������ж����ݱ��Ƿ����
	 * @return {bool} �Ƿ����
	 */
	bool tbl_exists(const char* tbl_name);

	/**
	 * ���� db_handle �Ĵ���ӿ�
	 * @param sql {const char*} ��׼�� SELECT SQL ��䣬����һ����Ҫ
	 *  ע��� SQL �����뾭��ת�崦���Է�ֹ SQL ע�빥��
	 * @return {bool} ִ���Ƿ�ɹ�
	 */
	bool sql_select(const char* sql);

	/**
	 * ���� db_handle �Ĵ���ӿ�
	 * @param sql {const char*} ��׼�� INSERT/UPDATE/DELETE SQL ��䣬
	 *  ����һ����Ҫע��� SQL �����뾭��ת�崦���Է�ֹ SQL ע�빥��
	 * @return {bool} ִ���Ƿ�ɹ�
	 */
	bool sql_update(const char* sql);

	/**
	 * ���� db_handle �Ĵ���ӿڣ��ϴ� sql ����Ӱ��ļ�¼����
	 * @return {int} Ӱ���������-1 ��ʾ����
	 */
	int affect_count() const;

private:
	// sqlite ����
	sqlite3* db_;

	// ���ݴ洢�ļ�
	string dbfile_;

	// �ַ���ת����
	charset_conv* conv_;

	// �����ַ���
	string charset_;

	// ����ִ��SQL��ѯ�ĺ���
	bool exec_sql(const char* sql);
};

} // namespace acl
