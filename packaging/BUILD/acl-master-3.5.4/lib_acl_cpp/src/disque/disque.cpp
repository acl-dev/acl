#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/disque/disque_node.hpp"
#include "acl_cpp/disque/disque_job.hpp"
#include "acl_cpp/disque/disque_cond.hpp"
#include "acl_cpp/disque/disque.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

#define INT_LEN		11

disque::disque(void)
: job_(NULL)
, version_(0)
{
}

disque::disque(redis_client* conn)
: redis_command(conn)
, job_(NULL)
, version_(0)
{
}

disque::disque(redis_client_cluster* cluster)
: redis_command(cluster)
, job_(NULL)
, version_(0)
{
}

disque::disque(redis_client_cluster* cluster, size_t)
: redis_command(cluster)
, job_(NULL)
, version_(0)
{
}

disque::~disque(void)
{
	free_nodes();
	if (job_) {
		delete job_;
	}
	free_jobs();
}

const char* disque::addjob(const char* name, const char* job,
	int timeout, const std::map<string, int>* args /* = NULL */)
{
	return addjob(name, job, strlen(job), timeout, args);
}

const char* disque::addjob(const char* name, const string& job,
	int timeout, const std::map<string, int>* args /* = NULL */)
{
	return addjob(name, job.c_str(), job.length(), timeout, args);
}

const char* disque::addjob(const char* name, const void* job, size_t job_len,
	int timeout, const std::map<string, int>* args /* = NULL */)
{
	size_t argc = 4;
	if (args != NULL && !args->empty()) {
		argc += args->size() * 2;
	}

	const char** argv = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "ADDJOB";
	lens[0] = sizeof("ADDJOB") - 1;

	argv[1] = name;
	lens[1] = strlen(name);

	argv[2] = (const char*) job;
	lens[2] = job_len;

	char buf[INT_LEN];
	safe_snprintf(buf, sizeof(buf), "%d", timeout);
	argv[3] = buf;
	lens[3] = strlen(buf);

	size_t i = 4;

	if (args == NULL || args->empty()) {
		build_request(i, argv, lens);
		return get_status();
	}

	std::map<string, int>::const_iterator cit = args->begin();
	for (; cit != args->end(); ++cit) {
		if (cit->first.compare("REPLICATE", false) == 0
			|| cit->first.compare("DELAY", false) == 0
			|| cit->first.compare("RETRY", false) == 0
			|| cit->first.compare("TTL", false) == 0
			|| cit->first.compare("MAXLEN", false) == 0) {

			argv[i] = cit->first.c_str();
			lens[i] = cit->first.length();
			i++;

			char* tmp = (char*) dbuf_->dbuf_alloc(INT_LEN);
			safe_snprintf(tmp, INT_LEN, "%d", cit->second);
			argv[i] = tmp;
			lens[i] = strlen(tmp);
			i++;
		} else if (cit->first.compare("ASYNC", false) == 0
			&& cit->second != 0) {

			argv[i] = cit->first.c_str();
			lens[i] = cit->first.length();
			i++;
		}
	}

	build_request(i, argv, lens);
	return get_status();
}

const char* disque::addjob(const char* name, const char* job,
	int timeout, const disque_cond* cond)
{
	return addjob(name, job, strlen(job), timeout, cond);
}

const char* disque::addjob(const char* name, const string& job,
	int timeout, const disque_cond* cond)
{
	return addjob(name, job.c_str(), job.length(), timeout, cond);
}

const char* disque::addjob(const char* name, const void* job, size_t job_len,
	int timeout, const disque_cond* cond)
{
	if (cond == NULL) {
		return addjob(name, job, job_len, timeout);
	}
	std::map<string, int> args;
	int n = cond->get_replicate();
	if (n > 0) {
		args["REPLICATE"] = n;
	}
	n = cond->get_delay();
	if (n >= 0) {
		args["DELAY"] = n;
	}
	n = cond->get_retry();
	if (n > 0) {
		args["RETRY"] = n;
	}
	n = cond->get_ttl();
	if (n > 0) {
		args["TTL"] = n;
	}
	n = cond->get_maxlen();
	if (n > 0) {
		args["MAXLEN"] = n;
	}
	if (cond->is_async()) {
		args["ASYNC"] = 1;
	}

	return addjob(name, job, job_len, timeout, &args);
}

void disque::free_jobs(void)
{
	if (jobs_.empty()) {
		return;
	}
	std::vector<disque_job*>::iterator it;
	for (it = jobs_.begin(); it != jobs_.end(); ++it) {
		delete *it;
	}
	jobs_.clear();
}

const std::vector<disque_job*>* disque::getjob(const char* name,
	size_t timeout, size_t count)
{
	std::vector<string> names;
	names.push_back(name);
	return getjob(names, timeout, count);
}

const std::vector<disque_job*>* disque::getjob(const std::vector<string>& names,
	size_t timeout, size_t count)
{
	size_t argc = 2 + names.size() + 4;

	const char** argv = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = "GETJOB";
	lens[0] = sizeof("GETJOB") - 1;

	size_t i = 1;
	if (timeout > 0) {
		argv[i] = "TIMEOUT";
		lens[i] = sizeof("TIMEOUT") - 1;
		i++;

		char* tmp = (char*) dbuf_->dbuf_alloc(INT_LEN);
		safe_snprintf(tmp, INT_LEN, "%d", (int) timeout);
		argv[i] = tmp;
		lens[i] = strlen(tmp);
		i++;
	}

	if (count > 0) {
		argv[i] = "COUNT";
		lens[i] = sizeof("COUNT") - 1;
		i++;

		char* tmp = (char*) dbuf_->dbuf_alloc(INT_LEN);
		safe_snprintf(tmp, INT_LEN, "%d", (int) count);
		argv[i] = tmp;
		lens[i] = strlen(tmp);
		i++;
	}

	argv[i] = "FROM";
	lens[i] = sizeof("FROM") - 1;
	i++;

	for (std::vector<string>::const_iterator cit = names.begin();
		cit != names.end(); ++cit) {

		argv[i] = (*cit).c_str();
		lens[i] = (*cit).length();
		i++;
	}

	build_request(i, argv, lens);
	return get_jobs(NULL);
}

const std::vector<disque_job*>* disque::qpeek(const char* name, int count)
{
	size_t argc = 3;
	const char* argv[3];
	size_t lens[3];

	argv[0] = "QPEEK";
	lens[0] = sizeof("QPEEK") - 1;

	argv[1] = name;
	lens[1] = strlen(name);

	char tmp[INT_LEN];
	safe_snprintf(tmp, sizeof(tmp), "%d", count);
	argv[2] = tmp;
	lens[2] = strlen(tmp);

	build_request(argc, argv, lens);
	return get_jobs(name);
}

const std::vector<disque_job*>* disque::get_jobs(const char* name)
{
	const redis_result* result = run();
	if (result == NULL) {
		return NULL;
	}
	if (result->get_type() != REDIS_RESULT_ARRAY) {
		return NULL;
	}

	size_t n;
	const redis_result**children = result->get_children(&n);
	if (children == NULL || n == 0) {
		return NULL;
	}

	free_jobs();

	string buf;
	for (size_t i = 0; i < n; i++) {
		const redis_result* rr = children[i];
		if (rr->get_type() != REDIS_RESULT_ARRAY)
			continue;
		size_t k;
		const redis_result** jobs = rr->get_children(&k);
		if (jobs == NULL) {
			continue;
		}

		if (name == NULL) {
			if (k < 3) {
				continue;
			}

			disque_job* job = NEW disque_job;
			jobs_.push_back(job);

			jobs[0]->argv_to_string(buf);
			job->set_queue(buf.c_str());
			buf.clear();

			jobs[1]->argv_to_string(buf);
			job->set_id(buf.c_str());
			buf.clear();

			jobs[2]->argv_to_string(buf);
			job->set_body(buf.c_str(), buf.length());
			buf.clear();
		} else {
			disque_job* job = NEW disque_job;
			jobs_.push_back(job);

			job->set_queue(name);

			jobs[0]->argv_to_string(buf);
			job->set_id(buf.c_str());
			buf.clear();

			jobs[1]->argv_to_string(buf);
			job->set_body(buf.c_str(), buf.length());
			buf.clear();
		}
	}

	return &jobs_;
}

int disque::qlen(const char* name)
{
	size_t argc = 2;
	const char* argv[2];
	size_t lens[2];

	argv[0] = "QLEN";
	lens[0] = sizeof("QLEN") - 1;

	argv[1] = name;
	lens[1] = strlen(name);

	build_request(argc, argv, lens);
	return get_number();
}

const disque_job* disque::show(const char* job_id)
{
	if (job_) {
		delete job_;
		job_ = NULL;
	}

	size_t argc = 2;
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SHOW";
	lens[0] = sizeof("SHOW") - 1;

	argv[1] = job_id;
	lens[1] = strlen(job_id);

	build_request(argc, argv, lens);
	const redis_result* rr = run();
	if (rr == NULL)
		return NULL;

	job_ = NEW disque_job;
	if (!job_->init(*rr)) {
		delete job_;
		job_ = NULL;
		return NULL;
	}
	return job_;
}

int disque::ackjob(const std::vector<string>& job_ids)
{
	return jobs_bat(job_ids, "ACKJOB");
}

int disque::fastack(const std::vector<string>& job_ids)
{
	return jobs_bat(job_ids, "FASKACK");
}

int disque::enqueue(const std::vector<string>& job_ids)
{
	return jobs_bat(job_ids, "ENQUEUE");
}

int disque::dequeue(const std::vector<string>& job_ids)
{
	return jobs_bat(job_ids, "ENQUEUE");
}

int disque::deljob(const std::vector<string>& job_ids)
{
	return jobs_bat(job_ids, "DEQUEUE");
}

int disque::jobs_bat(const std::vector<string>& job_ids, const char* cmd)
{
	size_t argc = 1 + job_ids.size();
	const char** argv = (const char**) dbuf_->dbuf_alloc(argc * sizeof(char*));
	size_t* lens = (size_t*) dbuf_->dbuf_alloc(argc * sizeof(size_t));

	argv[0] = cmd;
	lens[0] = strlen(cmd);

	size_t i = 1;
	std::vector<string>::const_iterator cit;
	for (cit = job_ids.begin(); cit != job_ids.end(); ++cit) {
		argv[i] = (*cit).c_str();
		lens[i] = (*cit).length();
		i++;
	}

	build_request(argc, argv, lens);
	return get_number();
}

bool disque::info(std::map<string, string>& out)
{
	size_t argc = 1;
	const char* argv[1];
	size_t lens[1];

	argv[0] = "INFO";
	lens[0] = sizeof("INFO") - 1;

	build_request(argc, argv, lens);
	string buf;
	if (get_string(buf) <= 0) {
		return false;
	}

	string line;
	while ((buf.scan_line(line))) {
		const std::vector<string>& tokens = line.split2(":");
		if (tokens.size() != 2) {
			line.clear();
			continue;
		}
		out[tokens[0]] = tokens[1];
		line.clear();
	}

	return true;
}

const std::vector<disque_node*>* disque::hello()
{
	free_nodes();

	size_t argc = 1;
	const char* argv[1];
	size_t lens[1];

	argv[0] = "HELLO";
	lens[0] = sizeof("HELLO") - 1;

	build_request(argc, argv, lens);
	const redis_result* rr = run();
	if (rr == NULL) {
		return NULL;
	}

	size_t n;
	const redis_result** children = rr->get_children(&n);
	if (children == NULL || n < 3) {
		return NULL;
	}

	if (children[0]->get_type() == REDIS_RESULT_INTEGER) {
		version_ = children[0]->get_integer();
	}

	if (children[1]->get_type() == REDIS_RESULT_STRING) {
		children[1]->argv_to_string(myid_);
	}

	for (size_t i = 2; i < n; i++) {
		disque_node* node = create_node(children[i]);
		if (node != NULL) {
			nodes_.push_back(node);
		}
	}
	return &nodes_;
}

disque_node* disque::create_node(const redis_result* rr)
{
	if (rr->get_type() != REDIS_RESULT_ARRAY) {
		return NULL;
	}

	size_t n;
	const redis_result** children = rr->get_children(&n);
	if (n < 4) {
		return NULL;
	}

	if (children[0]->get_type() != REDIS_RESULT_STRING) {
		return NULL;
	}
	string id;
	children[0]->argv_to_string(id);

	if (children[1]->get_type() != REDIS_RESULT_STRING) {
		return NULL;
	}
	string ip;
	children[1]->argv_to_string(ip);

	if (children[2]->get_type() != REDIS_RESULT_STRING) {
		return NULL;
	}
	string tmp;
	children[2]->argv_to_string(tmp);
	int port = ::atoi(tmp.c_str());

	if (children[3]->get_type() != REDIS_RESULT_STRING) {
		return NULL;
	}
	children[3]->argv_to_string(tmp);
	int priority = ::atoi(tmp.c_str());

	disque_node* node = NEW disque_node;
	node->set_id(id.c_str());
	node->set_ip(ip.c_str());
	node->set_port(port);
	node->set_priority(priority);

	return node;
}

void disque::free_nodes(void)
{
	std::vector<disque_node*>::iterator it;
	for (it = nodes_.begin(); it != nodes_.end(); ++it) {
		delete *it;
	}
	nodes_.clear();
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
