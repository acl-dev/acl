#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include <list>
#include <vector>
#include "acl_cpp/redis/redis_result.hpp"

namespace acl
{

class redis_client;
class redis_client_cluster;
class redis_request;

/**
 * redis �ͻ���������Ĵ��鸸��;
 * the redis command classes's base virtual class, which includes the basic
 * functions for all sub-classes
 */
class ACL_CPP_API redis_command
{
public:
	/**
	 * ȱʡ�Ĺ��캯�������ʹ�ô˹��캯����ʼ��������������� set_client ��
	 * set_cluster ���� redis �ͻ�������������ͨѶ��ʽ��
	 * default constructor. You must set the communication method by
	 * set_client or set_cluster functions.
	 */
	redis_command();

	/**
	 * ��ʹ�÷Ǽ�Ⱥģʽʱ�Ĺ��캯��������ʹ�ô˹��캯������ redis ͨ�������
	 * Using this constructor to set the redis communication mode,
	 * usually in no-cluster mode.
	 * @param conn {redis_client*} redis ͨ�������
	 *  the redis communication in no-cluster mode
	 */
	redis_command(redis_client* conn);

	/**
	 * ��Ⱥģʽ�Ĺ��캯�����ڹ��������ʱָ���˼�Ⱥģʽ�� redis_client_cluster ����
	 * Using this constructor to set the redis_client_cluster, usually in
	 * cluster mode.
	 * @param cluster {redis_client_cluster*} redis ��Ⱥ���Ӷ���
	 *  redis cluster object in cluster mode
	 * @param max_conns {size_t} �뼯Ⱥ�����н��֮���ÿ�����ӳص����������
	 *  the max of every connection pool with all the redis nodes
	 */
	redis_command(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_command() = 0;

	/**
	 * ���ظ�ʹ��һ���̳��� redis_command ��������� redis ʱ����Ҫ��
	 * ��һ�ε���ǰ���ñ��������ͷ��ϴβ�������ʱ����;
	 * when reusing a redis command sub-class, the reset method should be
	 * called first to rlease some resources in last command operation
	 * @param save_slot {bool} �����ü�Ⱥģʽʱ���ò��������Ƿ���Ҫ����
	 *  �����ϣ��ֵ������������� redis ��������е� key ֵ���䣬����Ա�����
	 *  ��ϣ��ֵ�Լ����ڲ����½��м���Ĵ���;
	 *  when in cluster mode, if your operations is on the same key, you
	 *  can set the param save_slot to false which can reduse the times
	 *  of compute the same key's hash-slot.
	 */
	void clear(bool save_slot = false);

	ACL_CPP_DEPRECATED_FOR("clear")
	void reset(bool save_slot = false);

	/**
	 * ��ʹ�÷Ǽ�Ⱥ��ʽʱ��ͨ���������������ӳػ�õ����Ӷ���;
	 * when not using cluster mode, the function is used
	 * to set the connection for next redis command operation.
	 * @param conn {redis_client*} �� redis �ͻ���������й���;
	 *  the redis connection to be set in next redis operation
	 */
	void set_client(redis_client* conn);

	/**
	 * ��õ�ǰ redis �ͻ�����������Ӷ���;
	 * get redis connection set by set_client function
	 * @return {redis_client*} ���� NULL ��ʾû�����Ӷ����뵱ǰ���������
	 *  ���а�;
	 *  the internal redis connection be returned, NULL if no redis
	 *  connection be set 
	 */
	redis_client* get_client() const
	{
		return conn_;
	}

	/**
	 * ��õ�ǰ redis ����������󶨵ķ�������ַ��ֻ�е��ö����� redis_client
	 * ��ʱ�������� set_client) �ſ��Ե��ñ�����
	 * get the redis-server's addr used by the current command. this
	 * method can only be used only if the redis_client was set by
	 * set_client method.
	 * @return {const char*} ���ؿմ� "" ��ʾû�а� redis ���Ӷ���
	 *  if "" was resturned, the redis connection was not set
	 */
	const char* get_client_addr() const;

	/**
	 * �������ӳؼ�Ⱥ������;
	 * set the redis cluster object in redis cluster mode
	 * @param cluster {redis_client_cluster*} redis ��Ⱥ���Ӷ���;
	 *  the redis_cluster connection object which can connect to any
	 *  redis-server and support connection pool
	 * @param max_conns {size_t} ���ڲ���̬�������ӳض���ʱ����ֵָ��ÿ����̬����
	 *  �����ӳص������������;
	 *  when dynamicly creating connection pool to any redis-server, use
	 *  this param to limit the max number for each connection pool
	 */
	void set_cluster(redis_client_cluster* cluster, size_t max_conns);

	/**
	 * ��������õ����ӳؼ�Ⱥ������;
	 * get redis_cluster object set by set_cluster function
	 * @return {redis_client_cluster*}
	 */
	redis_client_cluster* get_cluster() const
	{
		return cluster_;
	}

	/**
	 * ����ڴ�ؾ�������ڴ���� redis_command �ڲ�����;
	 * get memory pool handle be set
	 * @return {dbuf_pool*}
	 */
	dbuf_pool* get_pool() const
	{
		return pool_;
	}

	/**
	 * ��õ�ǰ���������������;
	 * get the result type returned from redis-server
	 * @return {redis_result_t}
	 */
	redis_result_t result_type() const;

	/**
	 * ������ֵΪ REDIS_RESULT_STATUS ����ʱ������������״̬��Ϣ;
	 * when result type is REDIS_RESULT_STATUS, the status info can be
	 * get by this function
	 * @return {const char*} ���� "" ��ʾ����;
	 *  "" will be returned on error
	 */
	const char* result_status() const;

	/**
	 * ������ʱ����ֵΪ REDIS_RESULT_ERROR ���ͣ����������س�����Ϣ;
	 * when result type is REDIS_RESULT_ERROR, the error info can be
	 * get by this function
	 * @return {const char*} ���ؿմ� "" ��ʾû�г�����Ϣ;
	 *  "" will be returned when no error info
	 */
	const char* result_error() const;

	/**
	 * ��õ�ǰ������洢�Ķ���ĸ���, �÷������Ի�ý��Ϊ������������
	 * (result_child/result_value) ����Ҫ������Ԫ�صĸ���;
	 * get number of result objects, just for functions
	 * result_child/result_value 
	 * @return {size_t} ����ֵ��洢���͵Ķ�Ӧ��ϵ���£�
	 *  the relation between return value and result type, as below:
	 *  REDIS_RESULT_ERROR: 1
	 *  REDIS_RESULT_STATUS: 1
	 *  REDIS_RESULT_INTEGER: 1
	 *  REDIS_RESULT_STRING: > 0 ʱ��ʾ���ַ������ݱ��зֳɷ������ڴ��ĸ���;
	 *       when the result type is REDIS_RESULT_STRING and the the
	 *       string is too large, the string was be cut into many small
	 *       chunks, the returned value is the chunks number
	 *  REDIS_RESULT_ARRAY: children_->size()
	 */
	size_t result_size() const;

	/**
	 * ������ֵΪ REDIS_RESULT_INTEGER ����ʱ�����������ض�Ӧ�� 32 λ����ֵ;
	 * get 32-bits number value if result type is REDIS_RESULT_INTERGER
	 * @param success {bool*} ��ָ��� NULL ʱ��¼���������Ƿ�ɹ�;
	 *  if the param pointer is not NULL, which will save status of
	 *  success or not for result from redis-server
	 * @return {int}
	 */
	int result_number(bool* success = NULL) const;

	/**
	 * ������ֵΪ REDIS_RESULT_INTEGER ����ʱ�����������ض�Ӧ�� 64 λ����ֵ;
	 * get 64-bits number value if result type is REDIS_RESULT_INTERGER
	 * @param success {bool*} ��ָ��� NULL ʱ��¼���������Ƿ�ɹ�;
	 *  if the param pointer is not NULL, which will save status of
	 *  success or not for result from redis-server
	 * @return {long long int}
	 */
	long long int result_number64(bool* success = NULL) const;

	/**
	 * ���ض�Ӧ�±������(���������ͷ� REDIS_RESULT_ARRAY ʱ��;
	 * get string result when result type isn't REDIS_RESULT_ARRAY
	 * @param i {size_t} �����±�;
	 *  the array index
	 * @param len {size_t*} ��Ϊ�� NULL ָ��ʱ�洢���������ݵĳ���;
	 *  *len will save the result's length if len is not NULL
	 * @return {const char*} ���� NULL ��ʾ�±�Խ��;
	 *  NULL will return if i beyonds the array's size
	 */
	const char* get_result(size_t i, size_t* len = NULL) const;

	/**
	 * �жϵ�ǰ���󶨵� redis ����������(redis_client) �����Ƿ��Ѿ��رգ�
	 * ֻ���ڲ��� conn_ ������ǿ�ʱ���ô˺�����������;
	 * to judge if the redis connection was be closed, only redis_client
	 * object be set internal
	 * @return {bool}
	 */
	bool eof() const;

	/**
	 * ��ñ��� redis �������̵Ľ��;
	 * get result object of last redis operation
	 * @return {redis_result*}
	 */
	const redis_result* get_result() const;

	/**
	 * ����ѯ���Ϊ�������ʱ���ñ��������һ������Ԫ�ض���;
	 * get one result ojbect of array if result type is REDIS_RESULT_ARRAY
	 * @param i {size_t} ���������±�ֵ;
	 *  the result array's index
	 * @return {const redis_result*} �����������������Ϊ�ջ����ʱ
	 *  �÷������� NULL;
	 *  NULL will be resturned when result is not REDIS_RESULT_ARRAY or
	 *  array empty or error
	 */
	const redis_result* result_child(size_t i) const;

	/**
	 * ���� redis-server ��õ�������һ���ַ������͵Ľ����ʱ�����Ե���
	 * ���������ĳ��ָ���±�λ�õ�����;
	 * when the reply from redis-serveer are strings array, this
	 * function can be used to get the string specified by a subscript
	 * @param i {size_t} �±꣨�� 0 ��ʼ��
	 *  the subscript of strings array
	 * @param len {size_t*} ����ָ��ǿգ���洢�����ؽ���ĳ��ȣ�������
	 *  �������طǿ�ָ��ʱ��Ч��
	 *  if len not a NULL pointer, it will store the length of string
	 *  specified by the subscript
	 * @return {const char*} ���ض�Ӧ�±��ֵ�������� NULL ʱ��ʾ���±�û
	 *  ��ֵ��Ϊ�˱�֤ʹ���ϵİ�ȫ�ԣ����ص��������ܱ�֤������� \0 ��β��
	 *  �ڼ������ݳ���ʱ�������ý�β������Ϊ�˼��ݶ��������Σ������߻���
	 *  Ӧ��ͨ�����ص� len ��ŵĳ���ֵ��������ݵ���ʵ����
	 *  the string will be returned associate with the subscript, if there
	 *  are nothing with the subscript, NULL will be returned
	 */
	const char* result_value(size_t i, size_t* len = NULL) const;

	/////////////////////////////////////////////////////////////////////
	/**
	 * �����Ƿ���������ݽ��з�Ƭ�������Ϊ true ���ڲ�����װ����Э���ʱ�򲻻�
	 * ���������ݿ�������װ��һ�������Ĵ����ݿ�
	 * just for request package, setting flag for sending data with
	 * multi data chunks; this is useful when the request data is large
	 * @param on {bool} �ڲ�Ĭ��ֵΪ false
	 *  if true the request data will not be combined one package,
	 *  internal default is false
	 */
	void set_slice_request(bool on);

	/**
	 * �����Ƿ����Ӧ���ݽ��з�Ƭ�������Ϊ true �򵱷������ķ������ݱȽϴ�ʱ��
	 * �����ݽ��з�Ƭ���ֳ�һЩ�����������ݿ�
	 * just for response package, settint flag for receiving data
	 * if split the large response data into multi little chunks
	 * @param on {bool} �ڲ�Ĭ��ֵΪ false
	 *  if true the response data will be splitted into multi little
	 *  data, which is useful for large reponse data for avoiding
	 *  malloc large continuously memory from system.
	 *  internal default is false
	 */
	void set_slice_respond(bool on);

protected:
	const redis_result* run(size_t nchild = 0);
	const redis_result* run(redis_client_cluster* cluster, size_t nchild);

	void build_request(size_t argc, const char* argv[], size_t lens[]);
	void clear_request();
	const redis_result** scan_keys(const char* cmd, const char* key,
		int& cursor, size_t& size, const char* pattern,
		const size_t* count);

	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::map<string, string>& attrs);
	void build(const char* cmd, const char* key,
		const std::map<string, const char*>& attrs);

	void build(const char* cmd, const char* key,
		const std::vector<string>& names,
		const std::vector<string>& values);
	void build(const char* cmd, const char* key,
		const std::vector<const char*>& names,
		const std::vector<const char*>& values);

	void build(const char* cmd, const char* key,
		const char* names[], const char* values[], size_t argc);
	void build(const char* cmd, const char* key,
		const int names[], const char* values[], size_t argc);
	void build(const char* cmd, const char* key,
		const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::vector<string>& names);
	void build(const char* cmd, const char* key,
		const std::vector<const char*>& names);
	void build(const char* cmd, const char* key,
		const std::vector<int>& names);

	void build(const char* cmd, const char* key,
		const char* names[], size_t argc);
	void build(const char* cmd, const char* key,
		const char* names[], const size_t lens[], size_t argc);
	void build(const char* cmd, const char* key,
		const int names[], size_t argc);

protected:
	int get_number(bool* success = NULL);
	long long int get_number64(bool* success = NULL);
	int get_number(std::vector<int>& out);
	int get_number64(std::vector<long long int>& out);
	bool check_status(const char* success = "OK");

	int get_status(std::vector<bool>& out);
	const char* get_status();

	int get_string(string& buf);
	int get_string(string* buf);
	int get_string(char* buf, size_t size);
	int get_strings(std::vector<string>& result);
	int get_strings(std::vector<string>* result);
	int get_strings(std::list<string>& result);
	int get_strings(std::list<string>* result);
	int get_strings(std::map<string, string>& result);
	int get_strings(std::vector<string>& names,
		std::vector<string>& values);
	int get_strings(std::vector<const char*>& names,
		std::vector<const char*>& values);

	/************************** common *********************************/
protected:
	dbuf_pool* pool_;

	// ���ݼ�ֵ�����ϣ��ֵ
	void hash_slot(const char* key);
	void hash_slot(const char* key, size_t len);

private:
	char  addr_[32];
	redis_client* conn_;
	redis_client_cluster* cluster_;
	size_t max_conns_;
	unsigned long long used_;
	int slot_;
	int redirect_max_;
	int redirect_sleep_;

	redis_client* peek_conn(redis_client_cluster* cluster, int slot);
	redis_client* redirect(redis_client_cluster* cluster, const char* addr);
	const char* get_addr(const char* info);
	void set_client_addr(const char* addr);
	void set_client_addr(redis_client& conn);

private:
	/************************** request ********************************/
	bool slice_req_;
	string* request_buf_;
	redis_request* request_obj_;
	size_t  argv_size_;
	const char**  argv_;
	size_t* argv_lens_;
	size_t  argc_;

	void argv_space(size_t n);
	void build_request1(size_t argc, const char* argv[], size_t lens[]);
	void build_request2(size_t argc, const char* argv[], size_t lens[]);

private:
	/************************** respond ********************************/
	bool slice_res_;
	const redis_result* result_;
};

} // namespace acl
