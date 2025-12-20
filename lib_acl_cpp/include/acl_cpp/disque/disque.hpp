#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "../stdlib/string.hpp"
#include "../redis/redis_command.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

typedef class redis_client disque_client;
typedef class redis_client_pool disque_client_pool;
typedef class redis_client_cluster disque_client_cluster;
class disque_cond;
class disque_node;
class disque_job;

/**
 * Disque command operation class
 */
class ACL_CPP_API disque : virtual public redis_command {
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
	 * Add a task message to the specified queue
	 * @param name {const char*} the name of the specified queue
	 *  Queue name
	 * @param job {const char*} a message to deliver
	 *  Task message string
	 * @param timeout {int} the command timeout in milliseconds
	 *  Timeout for command execution (milliseconds)
	 * @param args {const std::map<acl::string, int>*} the condition
	 *  for ADDJOB command, the conditions name include:
	 *  REPLICATE, DELAY, RETRY, TTL, MAXLEN, ASYNC, if the args was NULL,
	 *  none condition will be used in this operation;
	 *  Processing condition set for adding message. Corresponding condition items:
	 *   REPLICATE -- Number of replicas,
	 * DELAY -- How many seconds to wait before putting task into each node's queue
	 *   TTL -- Task lifetime (seconds)
	 *   MAXLEN -- Maximum number of tasks that can be stored in specified queue
	 * ASYNC -- Server uses asynchronous method to synchronize tasks to their
	 * replica nodes
	 * @return {const char*} a ID of the job will be returned, NULL will
	 *  be returned if some error happened.
	 *  Returns task ID. Returns NULL indicates error occurred
	 */
	const char* addjob(const char* name, const char* job,
		int timeout, const std::map<string, int>* args = NULL);
	const char* addjob(const char* name, const string& job,
		int timeout, const std::map<string, int>* args = NULL);
	const char* addjob(const char* name, const void* job, size_t job_len,
		int timeout, const std::map<string, int>* args = NULL);

	/**
	 * add a job to the specified queue
	 * Add task to specified message queue
	 * @param name {const char*} the name of the specified queue
	 *  Specified message queue name
	 * @param job {const char*} a message to deliver  Task to be added
	 * @param timeout {int} the command timeout in milliseconds
	 *  Command timeout limit with millisecond precision
	 * @param cond {const acl::disque_cond*} the condition for the ADDJOB
	 *  Condition for adding task, see class disque_cond
	 * @return {const char*} a ID of the job will be returned, NULL will
	 *  be returned if some error happened.
	 *  Returns task ID. Returns NULL indicates error occurred
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
	 * Get specified maximum number of tasks from specified queue collection
	 * @param names {const std::vector<acl::string>&} the specified queues
	 *  Specified list name collection
	 * @param timeout {int} the command timeout in milliseconds
	 *  Command timeout limit with millisecond precision
	 * @param count {size_t} the max count of the jobs to be got
	 *  Maximum number limit for returned task result set
	 * @return {const std::vector<acl::disque_job*>*} return the jobs,
	 *  or return NULL if the timeout is reached or some error happens.
	 *  Returns result set. Returns NULL if timeout or error occurs
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
	 * Notify node that task has been executed via given task ID. Node receiving
	 * ACK message will copy this message
	 * to multiple nodes, and try to perform garbage collection on task and ACK
	 * messages from cluster, thereby releasing
	 * occupied memory.
	 * @param job_ids {const std::vector<acl::string>&} the jobs' IDs
	 *  Task ID collection
	 * @return {int} return the number of IDs been ACKed, -1 will be
	 *  returned if some error happened
	 *  Returns number of acknowledged tasks. Returns -1 if error occurs
	 */
	int ackjob(const std::vector<string>& job_ids);

	/**
	 * perform a best effort cluster wide detection of the specified
	 * job IDs.
	 * Best effort deletion of given tasks cluster-wide. When network connection is
	 * good and all nodes are online,
	 * this command's effect is same as ACKJOB command's effect, but because
	 * message exchange caused by this command is less than
	 * ACKJOB, its speed is much faster than ACKJOB. However, when cluster contains
	 * failed nodes,
	 * FASTACK command is more likely than ACKJOB command to send same message
	 * multiple times
	 * @param job_ids {const std::vector<acl::string>&} the jobs' IDs
	 *  Task ID collection
	 * @return {int} return the number of IDs been ACKed, -1 will be
	 *  returned if some error happened
	 *  Returns number of acknowledged tasks. Returns -1 if error occurs
	 */
	int fastack(const std::vector<string>& job_ids);

	/**
	 * peek some jobs no more than the specified count from the specified
	 * queue and remain these jobs in queue.
	 * Return specified number of tasks from queue without removing tasks
	 * @param name {const char*} the specified queue
	 *  Specified queue name
	 * @param count {int} limit the max count of jobs to be got
	 *  Maximum number limit for returned result set
	 * @return {const std::vector<acl::disque_job*>*} return the jobs
	 *  if the queue isn't empty. NULL will be returned if the queue
	 *  is empty or some error happened.
	 *  Returns result set. Returns NULL if queue is empty or error occurs
	 */
	const std::vector<disque_job*>* qpeek(const char* name, int count);

	/**
	 * get the number of jobs stored in the specified queue
	 * Get number of tasks in specified queue
	 * @param name {const char*} the specified queue
	 *  Specified queue name
	 * @return {int} return the number of the jobs in queue
	 *  Returns number of tasks in specified queue. Returns -1 if error occurs
	 */
	int qlen(const char* name);

	/**
	 * get the stat information of the specified job by job id
	 * Get related information of task based on task ID
	 * @param job_id {const char*} the id of the job
	 *  Specified task ID
	 * @return {const acl::disque_job*} return the job's information,
	 *  return NULL if the job doesn't exist or some error happens.
	 * Returns information of specified task, see class disque_job. Returns NULL if
	 * task does not exist or error occurs
	 */
	const disque_job* show(const char* job_id);

	/**
	 * queue jobs if not already queued
	 * If given tasks are not already in queue, put them into queue
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 *  Specified task ID collection
	 * @return {int} return the number of jobs been queued, -1 will be
	 *  returned if some error happens.
	 *  Returns number of tasks put into queue. Returns -1 if error occurs
	 */
	int enqueue(const std::vector<string>& job_ids);

	/**
	 * remove the jobs from the queue
	 * Remove specified tasks from queue
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 *  Task ID collection to be removed
	 * @return {int} return the number of jobs been removed, -1 will be
	 *  returned if some error happens.
	 *  Returns number of removed tasks. Returns -1 if error occurs
	 */
	int dequeue(const std::vector<string>& job_ids);

	/**
	 * completely delete a job from a node.
	 * Completely delete given task in node. This command is very similar to
	 * FASTACK, the only difference is,
	 * deletion operation caused by DELJOB command only executes in a single node,
	 * it does not send DELJOB cluster bus
	 * message (cluster bus message) to other nodes
	 * @param job_ids {const std::vector<acl::string>&} the job IDs
	 * Task ID collection to be deleted
	 * @return {int} return the number of jobs been deleted, -1 will be
	 *  returned if some error happens.
	 *  Returns number of deleted tasks. Returns -1 if error occurs
	 */
	int deljob(const std::vector<string>& job_ids);

	/**
	 * display the information of the disque cluster
	 * Get current cluster's state information
	 * @param out {std::map<acl::string, acl::string>&} store result
	 *  Store result
	 * @return {bool} if the operation is successful
	 *  Whether operation is normal. Returns false if error occurs
	 */
	bool info(std::map<string, string>& out);

	/**
	 * get the information of all the nodes in the cluster
	 * Get information of all nodes in cluster
	 * @return {const std::vector<acl::disque_node*>*} all the nodes'
	 *  information in the cluster, return NULL if some error happened.
	 * Returns result set of all node information in cluster. Returns NULL if error
	 * occurs. See class disque_node
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

