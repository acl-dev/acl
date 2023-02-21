#include "stdafx.h"
#include "redis_object.h"
#include "redis_request.h"

redis_request::redis_request(acl::dbuf_guard* dbuf,
	acl::redis_client_pipeline* pipeline)
: dbuf_(dbuf)
, cmd_(pipeline)
{
	size_ = 100;
	argc_ = 0;
	argv_ = (const char**) dbuf_->dbuf_alloc(sizeof(const char*) * size_);
	lens_ = (size_t*) dbuf_->dbuf_alloc(sizeof(size_t) * size_);
}

redis_request::~redis_request(void) {}

acl::redis_pipeline_message& redis_request::get_message(void) {
	return cmd_.get_pipeline_message();
}

void redis_request::build_request(const redis_object& obj) {
	argc_ = 0;
	add_object(obj);

#if 0
	for (size_t i = 0; i < argc_; i++) {
		printf(">>argv[%zd]=%s, lens[%zd]=%zd\r\n",
			i, argv_[i], i, lens_[i]);
	}
#endif

	if (argc_ >= 2) {
		cmd_.hash_slot(argv_[1]);
	}

	cmd_.build_request(argc_, argv_, lens_);
}

void redis_request::add_object(const redis_object& obj) {
	redis_object_t type = obj.get_type();
	//printf(">>>>request type=%d\n", (int) type);

	switch (type) {
	case REDIS_OBJECT_ARRAY:
		add_array(obj);
		break;
	case REDIS_OBJECT_STRING:
		add_string(obj);
		break;
	default:
		logger_error("unknown type=%d", (int) type);
		break;
	}
}

void redis_request::add_array(const redis_object& obj) {
	size_t size;
	const redis_object** children = obj.get_children(&size);

	// XXX
	if (children == NULL || size == 0) {
		logger_error("invalid children=%p, size=%zd", children, size);
		return;
	}

	for (size_t i = 0; i < size; i++) {
		const redis_object* child = children[i];
		add_object(*child);
	}
}

void redis_request::add_string(const redis_object& obj) {
	size_t n = obj.get_size();
	assert(n == 1);
	argv_[argc_] = obj.get(0);
	lens_[argc_] = strlen(argv_[argc_]);
	argc_++;
}
