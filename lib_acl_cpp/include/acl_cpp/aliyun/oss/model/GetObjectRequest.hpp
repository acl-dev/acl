#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class dbuf_pool;
class ResponseHeaderOverrides;

class ACL_CPP_API GetObjectRequest
{
public:
	GetObjectRequest(const char* bucket, const char* key,
		dbuf_pool* pool = NULL);
	~GetObjectRequest();

	GetObjectRequest& setBucketName(const char* name);
	GetObjectRequest& setKey(const char* key);
	GetObjectRequest& setRange(long long int start, long long int end);
	GetObjectRequest& setMatchingETagConstraints(
		const std::list<string>& etags);
	GetObjectRequest& setNonmatchingETagConstraints(
		const std::list<string>& etags);
	GetObjectRequest& setUnmodifiedSinceConstraint(time_t date);
	GetObjectRequest& setModifiedSinceConstraint(time_t date);
	GetObjectRequest& setResponseHeaders(
		ResponseHeaderOverrides* responseHeaders);

	void reset();

	const char* getBucketName() const
	{
		return bucket_;
	}

	const char* getKey() const
	{
		return key_;
	}

	void getRange(long long int& start, long long int& end) const
	{
		start = range_[0];
		end = range_[1];
	}

	const std::list<char*>& getMatchingETagConstraints() const
	{
		return matching_etags_;
	}

	const std::list<char*>& getNonmatchingETagConstraints() const
	{
		return non_matching_etags_;
	}

	time_t getUnmodifiedSinceConstraint() const
	{
		return unmodified_since_;
	}

	time_t getModifiedSinceConstraint() const
	{
		return modified_since_;
	}

	ResponseHeaderOverrides* getResponseHeaders() const
	{
		return response_headers_;
	}

private:
	dbuf_pool* pool_;
	char* bucket_;
	char* key_;
	long long int range_[2];
	std::list<char*> matching_etags_;
	std::list<char*> non_matching_etags_;
	time_t unmodified_since_;
	time_t modified_since_;
	ResponseHeaderOverrides* response_headers_;
};

} // namespace acl
