#include "lib_acl.h"
#include "dns.h"

static void group_add(ACL_DNS *dns_handle, ACL_FILE *fp, char *line)
{
	const char *myname = "group_add";
	char *ptr = line, *label, *group, *name;
	char  buf[1024], refer[256];
	ACL_ARGV *argv_excepts;
	ACL_ARGV *argv_ips;
	ACL_VSTRING *buf1, *buf2;
	char *ip_list, *excepts;
	ACL_ITER  iter;

	label = acl_mystrtok(&ptr, " \t");
	if (label == NULL) {
		acl_msg_warn("%s(%d): data(%s) invalid",
			myname, __LINE__, line);
		return;
	}
	group = acl_mystrtok(&ptr, " \t");
	if (group == NULL) {
		return;
	}

	argv_excepts = acl_argv_alloc(1);
	argv_ips = acl_argv_alloc(1);

	ACL_SAFE_STRNCPY(refer, group, sizeof(refer));

	while ((acl_fgets_nonl(buf, sizeof(buf), fp)) != NULL) {
		ptr = buf;
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;
		if (strcasecmp(ptr, "#@group_end") == 0)
			break;
		label = acl_mystrtok(&ptr, " \t");
		if (label == NULL) {
			acl_msg_warn("%s(%d): line(%s) invalid",
				myname, __LINE__, buf);
			continue;
		}
		name = acl_mystrtok(&ptr, " \t");
		if (name == NULL) {
			acl_msg_warn("%s(%d): line(%s) invalid",
				myname, __LINE__, buf);
			continue;
		}
		if (strcasecmp(label, "#@refer") == 0) {
			ACL_SAFE_STRNCPY(refer, name, sizeof(refer));
		} else if (strcasecmp(label, "#@except") == 0) {
			acl_argv_add(argv_excepts, name, NULL); 
		} else if (strcasecmp(label, "#@ip") == 0) {
			acl_argv_add(argv_ips, name, NULL);
		}
	}

	buf1 = acl_vstring_alloc(256);
	acl_foreach(iter, argv_ips) {
		ptr = (char*) iter.data;
		if (ACL_VSTRING_LEN(buf1) > 0)
			acl_vstring_strcat(buf1, ",");
		acl_vstring_strcat(buf1, ptr);
	}
	if (ACL_VSTRING_LEN(buf1) > 0)
		ip_list = acl_vstring_str(buf1);
	else
		ip_list = NULL;

	buf2 = acl_vstring_alloc(256);
	acl_foreach(iter, argv_excepts) {
		ptr = (char*) iter.data;
		if (ACL_VSTRING_LEN(buf2) > 0)
			acl_vstring_strcat(buf2, ",");
		acl_vstring_strcat(buf2, ptr);
	}
	if (ACL_VSTRING_LEN(buf2) > 0)
		excepts = acl_vstring_str(buf2);
	else
		excepts = NULL;

	acl_dns_add_group(dns_handle, group, refer, ip_list, excepts);
	acl_argv_free(argv_excepts);
}

static void host_add(ACL_DNS *dns_handle, char *line)
{
	char *ptr = line, *ip, *name;

	ip = acl_mystrtok(&ptr, " \t");
	if (ip == NULL)
		return;
	name = acl_mystrtok(&ptr, " \t");
	if (name == NULL)
		return;
	acl_dns_add_host(dns_handle, name, ip);
}

static void hosts_load(ACL_DNS *dns_handle, const char *filename)
{
	const char *myname = "hosts_load";
	ACL_FILE *fp;
	char  buf[1024];

	fp = acl_fopen(filename, "r");
	if (fp == NULL) {
		acl_msg_error("%s(%d): fopen(%s) error(%s)",
			myname, __LINE__, filename, acl_last_serror());
		return;
	}

	while ((acl_fgets_nonl(buf, sizeof(buf), fp)) != NULL) {
		char *ptr;

		ptr = buf;
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;
		if (*ptr == '#') {
			ptr++;
			if (strncasecmp(ptr, "@group_name",
				sizeof("@group_name") - 1) == 0)
			{
				group_add(dns_handle, fp, ptr);
			}
		} else {
			host_add(dns_handle, ptr);
		}
	}

	acl_fclose(fp);
}

void dns_hosts_load(ACL_DNS *dns_handle, const char *hosts_list)
{
	ACL_ARGV *argv;
	ACL_ITER iter;

	if (dns_handle == NULL || hosts_list == NULL || *hosts_list == 0)
		return;
	argv = acl_argv_split(hosts_list, ";, \t");
	acl_foreach(iter, argv) {
		const char *filename = (const char*) iter.data;
		hosts_load(dns_handle, filename);
	}
	acl_argv_free(argv);
}
