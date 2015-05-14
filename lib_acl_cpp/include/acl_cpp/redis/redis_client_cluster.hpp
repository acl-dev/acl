#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl
{

class redis_pool;

/**
 * redis �ͻ��˼�Ⱥ�࣬ͨ�����������ע���� redis �ͻ���������(redis_command)��
 * ��ʹ���еĿͻ��������Զ�֧�ּ�Ⱥ�� redis ���
 * redis client cluster class. The class's object is set in the redis_command
 * using redis_command::set_cluster(redis_cluster*), and all the redis client
 * command will support the redis cluster mode.
 */
class ACL_CPP_API redis_client_cluster : public connect_manager
{
public:
	/**
	 * ���캯��;
	 * constructor
	 * @param conn_timeout {int} ���������ӳ�ʱʱ��(��);
	 *  timeout in seconds for connecting the redis-server
	 * @param rw_timeout {int}������ IO ��д��ʱʱ��(��);
	 *  read/write timeout in seconds from/to the redis-server
	 * @param max_slot {int} ��ϣ�����ֵ; the max hash-slot value of keys
	 */
	redis_client_cluster(int conn_timeout = 30, int rw_timeout = 30,
		int max_slot = 16384);
	virtual ~redis_client_cluster();

	/**
	 * ���ݹ�ϣ��ֵ��ö�Ӧ�����ӳ�;
	 * get one connection pool with the given slot
	 * @param slot {int} ��ϣ��ֵ;
	 *  the hash-slot value of key
	 * @return {redis_client_pool*} �����Ӧ�Ĺ�ϣ�۲������򷵻� NULL;
	 *  return the connection pool of the hash-slot, and return NULL
	 *  when the slot not exists
	 */
	redis_client_pool* peek_slot(int slot);

	/**
	 * ��̬���ù�ϣ��ֵ��Ӧ�� redis �����ַ���ú���������ʱ�ڲ����߳�������;
	 * dynamicly set redis-server addr with one slot, which is protected
	 * by thread mutex internal, no one will be set if the slot were
	 * beyyond the max hash-slot
	 * @param slot {int} ��ϣ��ֵ;
	 *  the hash-slot
	 * @param addr {const char*} redis ��������ַ;
	 *  one redis-server addr
	 */
	void set_slot(int slot, const char* addr);

	/**
	 * ���� redis ��Ⱥ�е�һ����㣬�Զ��������еĹ�ϣ�۶�Ӧ�Ľ���ַ
	 * according one node of the cluster, auto find all nodes with all
	 * slots range
	 * @param addr {const char*} ��Ⱥ�е�һ���������ַ����ʽ ip:port
	 *  on server node's addr of the cluster, addr format is "ip:port"
	 * @param max_conns {int} ��Ⱥ����ÿ������������ӳص������������
	 *  the max connections limit for each connection pool
	 */
	void set_all_slot(const char* addr, int max_conns);

	/**
	 * ��̬�����ϣ�۶�Ӧ�� redis �����ַ���Ա������¼���λ�ã��ڲ����߳�����������;
	 * dynamicly remove one slot and redis-server addr mapping, which is
	 * protected by thread mutex
	 * @param slot {int} ��ϣ��ֵ;
	 *  hash-slot value
	 */
	void clear_slot(int slot);

	/**
	 * ��ù�ϣ�����ֵ;
	 * get the max hash-slot
	 * @return {int}
	 */
	int get_max_slot() const
	{
		return max_slot_;
	}

	//////////////////////////////////////////////////////////////////////
	/**
	 * ����Э���ض�������ķ�ֵ��Ĭ��ֵΪ 15;
	 * set redirect limit for MOVE/ASK, default is 15
	 * @param max {int} �ض��������ֵ��ֻ�е���ֵ > 0 ʱ����Ч;
	 *  the redirect times limit for MOVE/ASK commands
	 */
	void set_redirect_max(int max);

	/**
	 * ����Э���ض�������ķ�ֵ;
	 * get redirect limit of MOVE/ASK commands in one redis redirect process
	 * @return {int}
	 */
	int get_redirect_max() const
	{
		return redirect_max_;
	}

	/**
	 * ���ض������ >= 2 ʱ�������ߵ�ʱ��(����)��Ĭ��ֵΪ 100 ���룬��������
	 * �ô��ǵ�һ�� redis �����������ߺ������ӽ������Ϊ���������Ҫ
	 * ʱ���(�� redis.conf �е� cluster-node-timeout ���������)������
	 * Ϊ�����ض���Ĵ�����Χ�ڲ�������Ҫ�ȴ��ӽ������Ϊ�����;
	 * if redirect happenning more than 2 in one redis command process,
	 * the process can sleep for a one avoiding redirect too fast, you
	 * can set the waiting time with microsecond here, default value is
	 * 100 microseconds; this only happends when redis-server died.
	 * @param n {int} ÿ���ض���ʱ����Ϣʱ��(����)��Ĭ��ֵΪ 100 ����;
	 * microseonds to sleep when redirect times are more than 2,
	 * default is 100 ms
	 */
	void set_redirect_sleep(int n);

	/**
	 * ��� set_redirect_sleep ���õĻ�Ĭ�ϵ�ʱ��;
	 * get sleep time set by set_redirect_sleep function
	 * @return {int} ��λΪ����;
	 *  return sleep value in microsecond
	 */
	int get_redirect_sleep() const
	{
		return redirect_sleep_;
	}

protected:
	/**
	 * ���ി�麯���������������ӳض���;
	 * virtual function of base class, which is used to create
	 * the connection pool
	 * @param addr {const char*} ������������ַ����ʽ��ip:port;
	 * the server addr for the connection pool, such as ip:port
	 * @param count {int} ���ӳصĴ�С����;
	 * the max connections in one connection pool
	 * @param idx {size_t} �����ӳض����ڼ����е��±�λ��(�� 0 ��ʼ);
	 * the index of the connection pool in pool array
	 */
	virtual connect_pool* create_pool(const char* addr,
		int count, size_t idx);
private:
	int   conn_timeout_;
	int   rw_timeout_;
	int   max_slot_;
	const char**  slot_addrs_;
	std::vector<char*> addrs_;
	int   redirect_max_;
	int   redirect_sleep_;
};

} // namespace acl
