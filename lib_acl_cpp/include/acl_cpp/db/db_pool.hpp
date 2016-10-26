#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"

namespace acl {

class db_handle;
class locker;

class ACL_CPP_API db_pool : public connect_pool
{
public:
	/**
	 * ���ݿ⹹�캯��
	 * @param dbaddr {const char*} ���ݿ��ַ
	 * @param count {size_t} ���ӳ�������Ӹ�������
	 * @param idx {size_t} �����ӳض����ڼ����е��±�λ��(�� 0 ��ʼ)
	 */
	db_pool(const char* dbaddr, size_t count, size_t idx = 0);
	virtual ~db_pool() {};

	/**
	 * �����ݿ����ӳػ��һ�����ݿ���󣬲���Ҫ������ݿ����ӣ����û�����
	 * ��ʽ���ٵ��� db_handle::open ���̣�
	 * ����������� db_pool->put(db_handle*) �����ӹ黹�����ݿ����ӳأ�
	 * �ɸú�����õ����Ӿ������ delete�������������ӳص��ڲ�����������
	 * @param charset {const char*} �����ݿ�ʱ���õ��ַ���
	 * @return {db_handle*} ���ݿ����Ӷ��󣬷��ؿձ�ʾ����
	 */
	db_handle* peek_open(const char* charset = NULL);

	/**
	 * ��õ�ǰ���ݿ����ӳص��������������
	 * @return {size_t}
	 */
	size_t get_dblimit() const
	{
		return get_max();
	}

	/**
	 * ��õ�ǰ���ݿ����ӳص�ǰ��������
	 * @return {size_t}
	 */
	size_t get_dbcount() const
	{
		return get_count();
	}

	/**
	 * �������ݿ����ӳ��п������ӵ���������(��)
	 * @param ttl {int} ��������(��)
	 */
	void set_idle(int ttl)
	{
		set_idle_ttl(ttl);
	}

protected:
	// �麯�������������Ҫ�����ַ������� peek_open ������ʱ���˺���
	// ���ȱ����������������ݿ����ӵ��ַ���
	virtual void set_charset(const char*) {}
};

class ACL_CPP_API db_guard : public connect_guard
{
public:
	db_guard(db_pool& pool) : connect_guard(pool) {}
	~db_guard(void);
};

} // namespace acl
