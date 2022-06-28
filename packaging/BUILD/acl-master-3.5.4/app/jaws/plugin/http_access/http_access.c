#include "lib_acl.h"
#include "http_plugin.h"
#include "http_access.h"

static ACL_FIFO *__domains_allow;

typedef struct {
	char *domain;
	int   dlen;
	int   permit;
	int   nocase;
} DOMAIN_ACCESS;

static int domain_correct(const char *domain)
{
	const char *myname = "domain_correct";
	const char *ptr = domain;
	int   lastch = 0;

	while (*ptr) {
		if (!(ACL_ISALNUM(*ptr) || *ptr == '.' || *ptr == '_' || *ptr == '-')) {
			acl_msg_warn("%s(%d): domain(%s) invalid, char(%c, %d)",
				myname, __LINE__, domain, *ptr, *ptr);
			return (0);
		}
		lastch = *ptr;
		ptr++;
	}

	if (!((lastch >= 'a' && lastch <= 'z') || (lastch >= 'A' && lastch <= 'Z'))) {
		acl_msg_warn("%s(%d): domain(%s) invalid, last char(%c, %d)",
			myname, __LINE__, domain, lastch, lastch);
		return (0);
	}
	return (1);
}

void http_access_init(void)
{
	const char *myname = "http_access_init";
	ACL_ARGV *argv;
	ACL_ITER  iter;

	__domains_allow = acl_fifo_new();

	if (!var_cfg_http_domain_allow || !(*var_cfg_http_domain_allow))
		return;

	argv = acl_argv_split(var_cfg_http_domain_allow, " \t,;");
	acl_foreach(iter, argv) {
		DOMAIN_ACCESS *dacc;
		char *ptr = (char*) iter.data;
		char *ptr1, *ptr2;
		int   permit = 1, nocase = 1;

		ptr1 = strchr(ptr, ':');
		if (ptr1) {
			*ptr1++ = 0;
			ptr2 = strchr(ptr, ':');
			if (ptr2) {
				*ptr2++ = 0;
				nocase = atoi(ptr2);
			}
			permit = atoi(ptr1);
		}
		if (!domain_correct(ptr))
			continue;
		dacc = (DOMAIN_ACCESS*) acl_mycalloc(1, sizeof(DOMAIN_ACCESS));
		dacc->domain = acl_mystrdup(ptr);
		dacc->dlen = strlen(dacc->domain);
		dacc->permit = permit;
		dacc->nocase = nocase;
		acl_fifo_push(__domains_allow, dacc);
		acl_msg_info("%s: add domain(%s, %s, %s)",
			myname, dacc->domain, permit ? "allow" : "deny",
			nocase ? "incaseble" : "caseble");
	}

	acl_argv_free(argv);
}

int http_access_permit(const char *domain)
{
	ACL_ITER iter;

	acl_foreach(iter, __domains_allow) {
		DOMAIN_ACCESS *dacc = (DOMAIN_ACCESS*) iter.data;

		if (dacc->nocase) {
			if (acl_strrncasecmp(domain, dacc->domain, dacc->dlen) == 0) {
				return (dacc->permit);
			}
		} else {
			if (acl_strrncmp(domain, dacc->domain, dacc->dlen) == 0) {
				return (dacc->permit);
			}
		}
	}

	return (var_cfg_http_domain_allow_all);
}
