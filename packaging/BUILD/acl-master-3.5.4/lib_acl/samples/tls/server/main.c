#include "lib_acl.h"

#if  defined(ACL_MS_WINDOWS)
#pragma comment(lib,"ws2_32")
#pragma comment(lib, "wsock32")
#endif

#include <time.h>
#include "tls.h"
#include "tls_params.h"
#ifdef ACL_UNIX
#include <getopt.h>
#endif
#include <stdio.h>

static char *conf_file = "tlssvr.cf";
static char  serv_addr[64];

int   var_cfg_server_tls_loglevel;
int   var_server_tls_ccert_vd;
int   var_cfg_server_tls_scache_timeout;
int   var_server_tls_set_sessid;
int   var_server_enforce_tls;
int   var_server_ask_client_cert;
int   var_server_tls_req_ccert;
int   var_server_starttls_tmout;
char *var_server_tls_cert_file;
char *var_server_tls_key_file;
char *var_server_tls_dcert_file;
char *var_server_tls_dkey_file;
char *var_server_tls_eccert_file;
char *var_server_tls_eckey_file;
char *var_server_tls_CAfile;
char *var_server_tls_CApath;
char *var_server_tls_dh1024_param_file;
char *var_server_tls_dh512_param_file;
char *var_server_tls_eecdh;
char *var_server_tls_fpt_dgst;

static ACL_CONFIG_INT_TABLE int_table[] = {
	{ "server_tls_loglevel", 0, &var_cfg_server_tls_loglevel, 0, 0 },
	{ "server_tls_ccert_vd", 9, &var_server_tls_ccert_vd, 0, 0 },
	{ "server_tls_scache_timeout", 10, &var_cfg_server_tls_scache_timeout, 0, 0 },
	{ "server_tls_set_sessid", 1, &var_server_tls_set_sessid, 0, 0 },
	{ "server_enforce_tls", 1, &var_server_enforce_tls, 0, 0 },
	{ "server_ask_client_cert", 1, &var_server_ask_client_cert, 0, 0 },
	{ "server_tls_req_ccert", 1, &var_server_tls_req_ccert, 0, 0 },
	{ "server_starttls_tmout", 300, &var_server_starttls_tmout, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE str_table[] = {
	{ "server_tls_cert_file", "conf/server.pem", &var_server_tls_cert_file },
	{ "server_tls_key_file", "conf/server.pem", &var_server_tls_key_file },
	{ "server_tls_dcert_file", "", &var_server_tls_dcert_file },
	{ "server_tls_dkey_file", "", &var_server_tls_dkey_file },
	{ "server_tls_eccert_file", "", &var_server_tls_eccert_file },
	{ "server_tls_eckey_file", "", &var_server_tls_eckey_file },
	{ "server_tls_CAfile", "conf/server.pem", &var_server_tls_CAfile },
	{ "server_tls_CApath", "", &var_server_tls_CApath },
	{ "server_tls_dh1024_param_file", "", &var_server_tls_dh1024_param_file },
	{ "server_tls_dh512_param_file", "", &var_server_tls_dh512_param_file },
	{ "server_tls_eecdh", "", &var_server_tls_eecdh },
	{ "server_tls_fpt_dgst", "sha1", &var_server_tls_fpt_dgst },
	{ 0, 0, 0 },
};

static TLS_SERVER_INIT_PROPS init_props;

static void init(void)
{
	acl_app_conf_load(conf_file);

	acl_get_app_conf_str_table(str_table);
	acl_get_app_conf_int_table(int_table);

	var_tlsmgr_stand_alone = 0;
	tls_server_init();

	TLS_SERVER_SETUP(&init_props,
			log_level = var_cfg_server_tls_loglevel,
			verifydepth = var_server_tls_ccert_vd,
			cache_type = TLS_MGR_SCACHE_SERVER,
			scache_timeout = var_cfg_server_tls_scache_timeout,
			set_sessid = var_server_tls_set_sessid,
			cert_file = var_server_tls_cert_file,
			key_file = var_server_tls_key_file,
			dcert_file = var_server_tls_dcert_file,
			dkey_file = var_server_tls_dkey_file,
			eccert_file = var_server_tls_eccert_file,
			eckey_file = var_server_tls_eckey_file,
			CAfile = var_server_tls_CAfile,
			CApath = var_server_tls_CApath,
			dh1024_param_file = var_server_tls_dh1024_param_file,
			dh512_param_file = var_server_tls_dh512_param_file,
			eecdh_grade = var_server_tls_eecdh,
			protocols = var_server_enforce_tls ?  var_server_tls_mand_proto : var_server_tls_proto,
			ask_ccert = var_server_ask_client_cert,
			fpt_dgst = var_server_tls_fpt_dgst);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c conf_file\n", procname);
}

static char *reply = "HTTP/1.1 200 OK\r\n"
	"Date: Thu, 09 Jul 2009 03:30:09 GMT\r\n"
	"Server: Apache/1.3.37 (Unix)\r\n"
	"Last-Modified: Fri, 13 Mar 2009 07:37:22 GMT\r\n"
	"Connection: close\r\n"
	"Content-Type: text/html\r\n"
	"\r\n"
	"<html><body>hello world!</body></html>";

static void free_ctx_fn(void *ctx)
{
	TLS_APPL_STATE *server_tls_ctx = (TLS_APPL_STATE*) ctx;
	tls_free_app_context(server_tls_ctx);
}

static void client_thread(void *arg)
{
	ACL_VSTREAM *client = (ACL_VSTREAM*) arg;
	static __thread TLS_APPL_STATE *server_tls_ctx;
	TLS_SESS_STATE *server_sess_ctx;
	TLS_SERVER_START_PROPS tls_props;
	char  buf[4096];
	int   ret, i;
	time_t last, now;

	int   tls_level = 2;
	char *namaddrport = "test.com";
	char *serverid = "service:addr:port:helo";
	char *tls_grade = "low";  /* high, medium, low, export, null */
	char *tls_exclusions = "";

	acl_tcp_nodelay(ACL_VSTREAM_SOCK(client), 1);

	if (server_tls_ctx == NULL) {
		server_tls_ctx = tls_server_create(&init_props);
		if (server_tls_ctx == NULL) {
			printf("tls_server_create error\n");
			return;
		}
		acl_pthread_atexit_add(server_tls_ctx, free_ctx_fn);
	}

	server_sess_ctx = TLS_SERVER_START(&tls_props,
			ctx = server_tls_ctx,
			stream = client,
			log_level = var_server_tls_loglevel,
			timeout = var_server_starttls_tmout,
			requirecert = (var_server_tls_req_ccert && var_server_enforce_tls),
			serverid = serverid,
			namaddr = namaddrport,
			cipher_grade = tls_grade,
			cipher_exclusions = tls_exclusions,
			fpt_dgst = var_server_tls_fpt_dgst);

	if (server_sess_ctx == NULL) {
		printf("TLS_SERVER_START error\n");
		acl_vstream_close(client);
		return;
	}

	if (tls_level >= TLS_LEV_VERIFY) {
		if (!TLS_CERT_IS_TRUSTED(server_sess_ctx)) {
			printf("Server certificate not trusted\n");
		}
	}

	if (tls_level > TLS_LEV_ENCRYPT) {
		if (!TLS_CERT_IS_MATCHED(server_sess_ctx)) {
			printf("Server certificate not verified\n");
		}
	}

	i = 0;
	time(&last);
	while (0) {
		ret = acl_vstream_gets(client, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			goto END;
		if (acl_vstream_writen(client, buf, ret) == ACL_VSTREAM_EOF)
			goto END;
		break;

		i++;
		if (i % 50000 == 0) {
			time(&now);
			printf("i: %d, time: %ld\n", i, now - last);
			last = now;
		}
	}

	while (1) {
		while (1) {
			ret = acl_vstream_gets_nonl(client, buf, sizeof(buf));
			if (ret == ACL_VSTREAM_EOF)
				goto END;

			if (ret == 0)
				break;
			/* printf("buf: %s, ret: %d\n", buf, ret); */
		}

		ret = acl_vstream_writen(client, reply, strlen(reply));
		/* printf("\nret: %d, reply: \n%s\n", ret, reply); */
		break;
	}

END:
	tls_server_stop(server_tls_ctx, client, var_server_starttls_tmout, 0, server_sess_ctx);
	acl_vstream_close(client);
}

static void run(void)
{
	ACL_VSTREAM *server, *client;
	acl_pthread_pool_t *pool;

	server = acl_vstream_listen(serv_addr, 128);
	if (server == NULL) {
		printf("listen %s error(%s)\n", serv_addr, acl_last_serror());
		return;
	}

	pool = acl_thread_pool_create(10, 0);
	if (pool == NULL) {
		acl_vstream_close(server);
		printf("create thread pool error(%s)\n", acl_last_serror());
		return;
	}
	
	while (1) {
		client = acl_vstream_accept(server, NULL, 0);
		if (client == NULL) {
			if (errno != ACL_EINTR) {
				printf("accept error(%s)\n", acl_last_serror());
				break;
			}
			continue;
		}
		acl_pthread_pool_add(pool, client_thread, client);
	}

	acl_vstream_close(server);
	acl_pthread_pool_destroy(pool);
}

static void free_fn(void)
{
	acl_myfree(conf_file);
}

int main(int argc, char *argv[])
{
	char  ch;

	acl_init();
	ACL_SAFE_STRNCPY(serv_addr, "0.0.0.0:443", sizeof(serv_addr));

	while ((ch = getopt(argc, argv, "hc:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			exit (0);
		case 'c':
			conf_file = acl_mystrdup(optarg);
			atexit(free_fn);
			break;
		default:
			break;
		}
	}

	init();
	run();
	acl_end();
	return (0);
}
