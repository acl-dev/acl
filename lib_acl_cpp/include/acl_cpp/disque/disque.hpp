#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

typedef class redis_client disque_client;
typedef class redis_client_pool disque_client_pool;
typedef class redis_client_cluster disque_client_cluster;
class disque_cond;
class disque_node;
class disque_job;

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
	 * see redis_command::redis_command(redis_client_cluster*£¬ size_t)
	 */
	disque(redis_client_cluster* cluster, size_t max_conns);

	virtual ~disque();

	/////////////////////////////////////////////////////////////////////

	/**
	 * add a job to the specified queue
	 * @param name {const char*} the name of the specified queue
	 * @param job {const char*} a message to deliver
	 * @param timeout {int} the command timeout in milliseconds
	 * @param args {const std::map<acl::string, int>*} the condition
	 *  for ADDJOB command, the conditions name include:
	 *  REPLICATE, DELAY, RETRY, TTL, MAXLEN, ASYNC, if the args was NULL,
	 *  none condition will be used in this operation
	 * @return {const char*} a ID of the job will be returned, NULL will
	 *  be returned if some error happened.
	 */
	const char* addjob(const char* name, const char* job,
		int timeout, const std::map<string, int>* args = NULL);
	const char* addjob(const char* name, const string& job,
		int timeout, const std::map<string, int>* args = NULL);
	const char* addjob(const char* name, const void* job, size_t job_len,
		int timeout, const std::map<string, int>* args = NULL);

	/**
	* add a job to the specified queue
	* @param name {const char*} the name of the specified queue
	* @param job {const char*} a message to deliver
	* @param timeout {int} the command timeout in milliseconds
	* @param cond {const acl::disque_cond*} the condition for the ADDJOB
	* @return {const char*} a ID of the job will be returned, NULL will
	*  be returned if some error happened.
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
	 * @param names {const std::vector<acl::string>&} the specified queues
	 * @param timeout {int} the command timeout in milliseconds
	 * @param count {size_t} the max count of the jobs to be got
	 * @return {const std::vector<acl::disque_job*>*} return the jobs,
	 *  or return NULL if the timeout is reached or some error happens.
	 */
	const std::vector<disque_job*>* getjob(const std::vector<string>& names,
		size_t timeout, size_t count);

	/**
	 * acknowledge the execution of one or more jobs via IDs. The node
	 * receiving the ACK will replicate it to multiple nodes and will try
	 * to garbage collect both the job and the ACKs from the cluster so
	 * that memory can be freed.
	 * @param job_ids {const std::vector<acl::string>&} the jobs' IDs
	 * @return {int} return the number of IDs been ACKed, -1 will be
	 *  returned if some error happened
	 */
	int ackjob(const std::vector<string>& job_ids);

	/**
	 * perform a best effort cluster wide detection of the specified
	 * job IDs.
	 * @param job_ids {const std::vector<acl::string>&} the jobs' IDs
	 * @return {int} return the number of IDs been ACKed, -1 will be
	 *  returned if some error happened
	 */
	int fastack(const std::vector<string>& job_ids);

	/**
	 * peek some jobs no more than the specified count from the specified
	 * queue and remain these jobs in queue.
	 * @param name {const char*} the specified queue
	 * @param count {int} limit the max count of jobs to be got
	 * @return {const std::vector<acl::disque_job*>*} return the jobs
	 *  if the queue isn't empty. NULL will be returned if the queue
	 *  is empty or some error happened.
	 */
	const std::vector<disque_job*>* qpeek(const char* name, int count);

	/**
	 * get the number of jobs stored in the specified queue
	 * @param name {const char*} the specified queue
	 * @return {int} return the number of the jobs in queue
	 */
	int qlen(const char* name);

	/**
	 * get the stat information of the specified job by job id
	 * @param job_id {const char*} the id of the job
	 * @return {const acl::disque_job*} return the job's information,
	 *  return NULL if the job doesn't exist or some error happens.
	 */
	const disque_job* show(const char* job_id);

	/**
	 * queue jobs if not already queued
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 * @return {int} return the number of jobs been queued, -1 will be
	 *  returned if some error happens.
	 */
	int enqueue(const std::vector<string>& job_ids);

	/**
	 * remove the jobs from the queue
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 * @return {int} return the number of jobs been removed, -1 will be
	 *  returned if some error happens.
	 */
	int dequeue(const std::vector<string>& job_ids);

	/**
	 * completely delete a job from a node.
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 * @return {int} return the number of jobs been deleted, -1 will be
	 *  returned if some error happens.
	 */
	int deljob(const std::vector<string>& job_ids);

	/**
	 * display the information of the disque cluster
	 * @param out {std::map<acl::string, acl::string>&} store result
	 * @return {bool} if the operation is successful
	 */
	bool info(std::map<string, string>& out);

	/**
	 * get the information of all the nodes in the cluster
	 * @return {const std::vector<acl::disque_node*>*} all the nodes'
	 *  information in the cluster, return NULL if some error happened.
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
