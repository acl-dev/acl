#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "../stdlib/string.hpp"
#include "../redis/redis_command.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

typedef class redis_client disque_client;
typedef class redis_client_pool disque_client_pool;
typedef class redis_client_cluster disque_client_cluster;
class disque_cond;
class disque_node;
class disque_job;

/**
 * disque 命令操作类
 */
class ACL_CPP_API disque : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	disque();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	disque(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	disque(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	disque(redis_client_cluster* cluster, size_t max_conns);

	virtual ~disque();

	/////////////////////////////////////////////////////////////////////

	/**
	 * add a job to the specified queue
	 * 添加一个任务消息至指定的队列中
	 * @param name {const char*} the name of the specified queue
	 *  队列名称
	 * @param job {const char*} a message to deliver
	 *  任务消息字符串
	 * @param timeout {int} the command timeout in milliseconds
	 *  该命令执行的超时时间（毫秒）
	 * @param args {const std::map<acl::string, int>*} the condition
	 *  for ADDJOB command, the conditions name include:
	 *  REPLICATE, DELAY, RETRY, TTL, MAXLEN, ASYNC, if the args was NULL,
	 *  none condition will be used in this operation
	 *  添加消息的处理条件集合，对应的条件内容项：
	 *   REPLICATE -- 副本个数，
	 *   DELAY -- 指定任务在放入各个节点的队列之前， 需要等待多少秒钟
	 *   TTL -- 任务生存周期（秒）
	 *   MAXLEN -- 指定队列最多可以存放多少个待传递的任务
	 *   ASYNC -- 服务端采用异步方式将任务同步至其副本结点
	 * @return {const char*} a ID of the job will be returned, NULL will
	 *  be returned if some error happened.
	 *  返回任务 ID 号，如果返回 NULL 则表示出错
	 */
	const char* addjob(const char* name, const char* job,
		int timeout, const std::map<string, int>* args = NULL);
	const char* addjob(const char* name, const string& job,
		int timeout, const std::map<string, int>* args = NULL);
	const char* addjob(const char* name, const void* job, size_t job_len,
		int timeout, const std::map<string, int>* args = NULL);

	/**
	 * add a job to the specified queue
	 * 向指定消息队列添加任务
	 * @param name {const char*} the name of the specified queue
	 *  指定的消息队列名称
	 * @param job {const char*} a message to deliver
	 *  将添加的任务
	 * @param timeout {int} the command timeout in milliseconds
	 *  毫秒精度的命令超时限制
	 * @param cond {const acl::disque_cond*} the condition for the ADDJOB
	 *  添加任务的条件，参见类 disque_cond
	 * @return {const char*} a ID of the job will be returned, NULL will
	 *  be returned if some error happened.
	 *  返回任务 ID 号，如果返回 NULL 则表示出错
	 */
	const char* addjob(const char* name, const char* job,
		int timeout, const disque_cond* cond);
	const char* addjob(const char* name, const string& job,
		int timeout, const disque_cond* cond);
	const char* addjob(const char* name, const void* job, size_t job_len,
		int timeout, const disque_cond* cond);

	/**
	 * get jobs from the specified queues, or return NULL if the timeout
	 * is reached.
	 * 从指定的队列集合中获得取指定最大数量的任务
	 * @param names {const std::vector<acl::string>&} the specified queues
	 *  指定的列表名称集合
	 * @param timeout {int} the command timeout in milliseconds
	 *  毫秒精度的命令超时限制
	 * @param count {size_t} the max count of the jobs to be got
	 *  指定了返回任务结果集的最大个数限制
	 * @return {const std::vector<acl::disque_job*>*} return the jobs,
	 *  or return NULL if the timeout is reached or some error happens.
	 *  返回结果集。如果超时或发生错误则返回 NULL
	 */
	const std::vector<disque_job*>* getjob(const std::vector<string>& names,
		size_t timeout, size_t count);
	const std::vector<disque_job*>* getjob(const char* name,
		size_t timeout, size_t count);

	/**
	 * acknowledge the execution of one or more jobs via IDs. The node
	 * receiving the ACK will replicate it to multiple nodes and will try
	 * to garbage collect both the job and the ACKs from the cluster so
	 * that memory can be freed.
	 * 通过给定任务 ID ， 向节点告知任务已经被执行。接收到 ACK 消息的节点会将该消息
	 * 复制至多个节点， 并尝试对任务和来自集群的 ACK 消息进行垃圾回收操作， 从而释放
	 * 被占用的内存。
	 * @param job_ids {const std::vector<acl::string>&} the jobs' IDs
	 *  任务 ID 集合
	 * @return {int} return the number of IDs been ACKed, -1 will be
	 *  returned if some error happened
	 *  返回被确认的任务个数，如果出错则返回 -1
	 */
	int ackjob(const std::vector<string>& job_ids);

	/**
	 * perform a best effort cluster wide detection of the specified
	 * job IDs.
	 * 尽最大努力在集群范围内对给定的任务进行删除；在网络连接良好并且所有节点都在线时， 
	 * 这个命令的效果和 ACKJOB 命令的效果一样， 但是因为这个命令引发的消息交换比
	 * ACKJOB 要少， 所以它的速度比 ACKJOB 要快不少；但是当集群中包含了失效节点的
	 * 时候， FASTACK 命令比 ACKJOB 命令更容易出现多次发送同一消息的情况
	 * @param job_ids {const std::vector<acl::string>&} the jobs' IDs
	 *  任务 ID 集合
	 * @return {int} return the number of IDs been ACKed, -1 will be
	 *  returned if some error happened
	 *  返回被确认的任务个数，如果出错则返回 -1
	 */
	int fastack(const std::vector<string>& job_ids);

	/**
	 * peek some jobs no more than the specified count from the specified
	 * queue and remain these jobs in queue.
	 * 在不取出任务的情况下， 从队列里面返回指定数量的任务
	 * @param name {const char*} the specified queue
	 *  指定的队列名称
	 * @param count {int} limit the max count of jobs to be got
	 *  限定了返回结果集的最大数量
	 * @return {const std::vector<acl::disque_job*>*} return the jobs
	 *  if the queue isn't empty. NULL will be returned if the queue
	 *  is empty or some error happened.
	 *  返回结果集，如果队列为空或出错则返回 NULL
	 */
	const std::vector<disque_job*>* qpeek(const char* name, int count);

	/**
	 * get the number of jobs stored in the specified queue
	 * 获得指定队列中的任务数量
	 * @param name {const char*} the specified queue
	 *  指定的队列名称
	 * @return {int} return the number of the jobs in queue
	 *  返回指定队列中的任务数量，返回出错则返回 -1
	 */
	int qlen(const char* name);

	/**
	 * get the stat information of the specified job by job id
	 * 根据任务 ID 获得任务的相关信息
	 * @param job_id {const char*} the id of the job
	 *  指定的任务 ID
	 * @return {const acl::disque_job*} return the job's information,
	 *  return NULL if the job doesn't exist or some error happens.
	 *  返回指定任务的信息，参考类 disque_job；如果任务不存在或出错则返回 NULL
	 */
	const disque_job* show(const char* job_id);

	/**
	 * queue jobs if not already queued
	 * 如果给定任务未被放入到队列里， 则把它们放入到队列里
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 *  指定的任务 ID 集合
	 * @return {int} return the number of jobs been queued, -1 will be
	 *  returned if some error happens.
	 *  返回被放入队列里的任务数量，如果出错则返回 -1
	 */
	int enqueue(const std::vector<string>& job_ids);

	/**
	 * remove the jobs from the queue
	 * 从队列里面移除指定的任务
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 *  准备移除的任务 ID 集合
	 * @return {int} return the number of jobs been removed, -1 will be
	 *  returned if some error happens.
	 *  返回被移除的任务数量，如果出错则返回 -1
	 */
	int dequeue(const std::vector<string>& job_ids);

	/**
	 * completely delete a job from a node.
	 * 在节点里面彻底地删除给定的任务。 这个命令和 FASTACK 很相似， 唯一的不同是，
	 * DELJOB 命令引发的删除操作只会在单个节点里面执行， 它不会将 DELJOB 集群总线
	 * 消息（cluster bus message）发送至其他节点
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 * 被删除的任务 ID 集合
	 * @return {int} return the number of jobs been deleted, -1 will be
	 *  returned if some error happens.
	 *  返回被删除的任务数量，如果出错则返回 -1
	 */
	int deljob(const std::vector<string>& job_ids);

	/**
	 * display the information of the disque cluster
	 * 获得当前集群的状态信息
	 * @param out {std::map<acl::string, acl::string>&} store result
	 *  存储结果
	 * @return {bool} if the operation is successful
	 *  操作是否正常，如果出错则返回 false
	 */
	bool info(std::map<string, string>& out);

	/**
	 * get the information of all the nodes in the cluster
	 * 获得集群中所有结点的信息
	 * @return {const std::vector<acl::disque_node*>*} all the nodes'
	 *  information in the cluster, return NULL if some error happened.
	 *  返回集群中所有结点信息的结果集，如果出错则返回 NULL；参考类 disque_node
	 */
	const std::vector<disque_node*>* hello();

private:
	int jobs_bat(const std::vector<string>& job_ids, const char* cmd);

private:
	disque_job* job_;
	std::vector<disque_job*> jobs_;

	const std::vector<disque_job*>* get_jobs(const char* name);
	void free_jobs();

private:
	int version_;
	string myid_;
	std::vector<disque_node*> nodes_;

	disque_node* create_node(const redis_result* rr);
	void free_nodes();
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
