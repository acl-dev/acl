#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "http_vhost.h"
#include "http_service.h"

#define SET_STR(_xcp_, _name_, _value_, _default_) do { \
       	ACL_CFG_SET_ITEM_STR(_xcp_, _name_, _value_); \
	if (_value_ == NULL) \
		_value_ = acl_mystrdup(_default_); \
} while (0)

#define SET_INT ACL_CFG_SET_ITEM_INT

static void add_vhost(const char *file_path, int def)
{
	const char *myname = "add_vhost";
	ACL_XINETD_CFG_PARSER *xcp;
	char *cf_host = NULL, *cf_root = NULL, *cf_default = NULL;
	int   i, n;
	const ACL_ARRAY *vpath_maps;
	char *vpath_map;
	ACL_ARGV *map_argv;
	HTTP_VHOST *vhost;
	char  ebuf[256];

	xcp = acl_xinetd_cfg_load(file_path);
	if (xcp == NULL) {
		acl_msg_error("%s(%d): load file(%s) error(%s)",
			myname, __LINE__, file_path,
			acl_last_strerror(ebuf, sizeof(ebuf)));
		return;
	}

	SET_STR(xcp, "host", cf_host, "localhost");
	SET_STR(xcp, "root_path", cf_root, "/opt/jaws/var/www/htdocs/");
	SET_STR(xcp, "default_page", cf_default, "index.html");

	if (def)
		vhost = http_vhost_add_def(cf_host, cf_root, cf_default);
	else
		vhost = http_vhost_add(cf_host, cf_root, cf_default);

	vpath_maps = acl_xinetd_cfg_get_ex(xcp, "vpath_map");
	if (vpath_maps) {
		n = acl_array_size(vpath_maps);
		for (i = 0; i < n; i++) {
			vpath_map = (char *) acl_array_index(vpath_maps, i);
			if (vpath_map == NULL)
				break;
			map_argv = acl_argv_split(vpath_map, "\t ");
			if (map_argv->argc < 3) {
				acl_msg_error("%s(%d): vpath_map(%s) invalid",
					myname, __LINE__, vpath_map);
				acl_argv_free(map_argv);
				continue;
			}

			http_vpath_add(vhost, map_argv->argv[0],
				map_argv->argv[1], atoi(map_argv->argv[2]));
			acl_argv_free(map_argv);
		}
	}

	acl_myfree(cf_host);
	acl_myfree(cf_root);
	acl_myfree(cf_default);

	acl_xinetd_cfg_free(xcp);
}

void http_conf_load(const char *path, const char *default_cf)
{
	const char *myname = "http_conf_load";
	ACL_SCAN_DIR *scan_dir;
	const char *file_name;
	ACL_VSTRING *file_path;
	char  ebuf[256];

	http_vhost_init();

	add_vhost(default_cf, 1);

	file_path = acl_vstring_alloc(256);
	if (file_path == NULL)
		acl_msg_fatal("%s(%d): acl_vstring_alloc error(%s)",
			myname, __LINE__,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	scan_dir = acl_scan_dir_open(path, 1);
	if (scan_dir == NULL)
		acl_msg_fatal("%s(%d): acl_scan_dir_open error(%s)",
			myname, __LINE__,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	while (1) {
		file_name = acl_scan_dir_next_file(scan_dir);
		if (file_name == NULL)
			break;
		acl_vstring_sprintf(file_path, "%s/%s", path, file_name);
		add_vhost(acl_vstring_str(file_path), 0);
	}

	acl_scan_dir_close(scan_dir);
	acl_vstring_free(file_path);
}
