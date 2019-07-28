#include "StdAfx.h"
#include "global.h"

global::global()
{
	const char* filepath = acl_process_path();
	ACL_VSTRING* path = acl_vstring_alloc(256);
	acl_sane_dirname(path, filepath);
	path_ = acl_vstring_str(path);
	acl_vstring_free(path);
}

global::~global()
{

}

void global::get_filename(const char* filepath, acl::string& buf)
{
	ACL_VSTRING* bp = acl_vstring_alloc(256);
	acl_sane_basename(bp, filepath);
	buf = acl_vstring_str(bp);
	acl_vstring_free(bp);
}