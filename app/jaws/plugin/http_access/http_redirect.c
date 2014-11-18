#include "lib_acl.h"
#include "lib_protocol.h"
#include "http_plugin.h"
#include "http_redirect.h"

static ACL_FIFO	 *__redirect_list = NULL;

static HTTP_DOMAIN_MAP *http_domain_map_new(const char *from, const char *to)
{
	HTTP_DOMAIN_MAP *hdm = (HTTP_DOMAIN_MAP*)
		acl_mymalloc(sizeof(HTTP_DOMAIN_MAP));

	hdm->domain_from = acl_mystrdup(from);
	hdm->domain_to = acl_mystrdup(to);
	hdm->size_from = strlen(from);
	return (hdm);
}

static void http_domain_map_free(HTTP_DOMAIN_MAP *hdm)
{
	acl_myfree(hdm->domain_from);
	acl_myfree(hdm->domain_to);
	acl_myfree(hdm);
}

void http_redirect_end(void)
{
	if (!__redirect_list)
		return;
	acl_fifo_free(__redirect_list, (void (*)(void*)) http_domain_map_free);
	__redirect_list = NULL;
}

void http_redirect_init(void)
{
	const char *myname = "http_redirect_init";
	ACL_ARGV *argv;
	ACL_ITER  iter;

	__redirect_list = acl_fifo_new();

	if (var_cfg_http_domain_redirect == NULL
	    || *var_cfg_http_domain_redirect == 0)
		return;
	
	/* 去掉多余的空格和缩近 */
	acl_mystr_trim(var_cfg_http_domain_redirect);
	/* 统一转换为小写 */
	acl_lowercase(var_cfg_http_domain_redirect);
	argv = acl_argv_split(var_cfg_http_domain_redirect, ";");
	acl_foreach(iter, argv) {
		HTTP_DOMAIN_MAP *hdm;
		char *ptr = (char*) iter.data;
		char *ptr1 = strchr(ptr, '=');
		if (ptr1 == NULL || *(ptr1 + 1) == 0) {
			acl_msg_warn("%s(%d): invalid redirect(%s)",
				myname, __LINE__, ptr);
			continue;
		}
		*ptr1++ = 0;
		hdm = http_domain_map_new(ptr, ptr1);
		acl_msg_info("%s(%d): add(%s, %s) map", myname, __LINE__, ptr, ptr1);
		acl_fifo_push(__redirect_list, hdm);
	}
}

HTTP_DOMAIN_MAP *http_redirect_lookup(const char *domain)
{
	char  buf[256];
	ACL_ITER iter;

	if (__redirect_list == NULL)
		return (NULL);

	ACL_SAFE_STRNCPY(buf, domain, sizeof(buf));
	acl_lowercase(buf);

	acl_foreach(iter, __redirect_list) {
		HTTP_DOMAIN_MAP *hdm = (HTTP_DOMAIN_MAP*) iter.data;
		if (acl_strrncmp(domain, hdm->domain_from, hdm->size_from) == 0)
			return (hdm);
	}
	return (NULL);
}
