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

static char *conf_file = "tlscli.cf";
static char  serv_addr[64];

int   var_client_tls_loglevel;
int   var_client_tls_scert_vd;
int   var_client_starttls_tmout;
char *var_client_tls_cert_file;
char *var_client_tls_key_file;
char *var_client_tls_dcert_file;
char *var_client_tls_dkey_file;
char *var_client_tls_eccert_file;
char *var_client_tls_eckey_file;
char *var_client_tls_CAfile;
char *var_client_tls_CApath;
char *var_client_tls_fpt_dgst;

static ACL_CONFIG_INT_TABLE int_table[] = {
	{ "client_tls_loglevel", 0, &var_client_tls_loglevel, 0, 0 },
	{ "client_tls_scert_vd", 9, &var_client_tls_scert_vd, 0, 0 },
	{ "client_starttls_tmout", 300, &var_client_starttls_tmout, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE str_table[] = {
	{ "client_tls_cert_file", "conf/foo-cert.pem", &var_client_tls_cert_file },
	{ "client_tls_key_file", "conf/foo-key.pem", &var_client_tls_key_file },
	{ "client_tls_dcert_file", "", &var_client_tls_dcert_file },
	{ "client_tls_dkey_file", "", &var_client_tls_dkey_file },
	{ "client_tls_eccert_file", "", &var_client_tls_eccert_file },
	{ "client_tls_eckey_file", "", &var_client_tls_eckey_file },
	{ "client_tls_CAfile", "conf/cacert.pem", &var_client_tls_CAfile },
	{ "client_tls_CApath", "", &var_client_tls_CApath },
	{ "client_tls_fpt_dgst", "sha1", &var_client_tls_fpt_dgst },
	{ 0, 0, 0 },
};

static TLS_CLIENT_INIT_PROPS init_props;

static void init(void)
{
	acl_app_conf_load(conf_file);

	acl_get_app_conf_str_table(str_table);
	acl_get_app_conf_int_table(int_table);

	tls_client_init();

#if 0
	TLS_CLIENT_SETUP(&init_props,
			log_level = var_client_tls_loglevel,
			verifydepth = var_client_tls_scert_vd,
			cache_type = TLS_MGR_SCACHE_CLIENT,
			cert_file = var_client_tls_cert_file,
			key_file = var_client_tls_key_file,
			dcert_file = var_client_tls_dcert_file,
			dkey_file = var_client_tls_dkey_file,
			eccert_file = var_client_tls_eccert_file,
			eckey_file = var_client_tls_eckey_file,
			CAfile = var_client_tls_CAfile,
			CApath = var_client_tls_CApath,
			fpt_dgst = var_client_tls_fpt_dgst);
#else
	init_props.log_level = var_client_tls_loglevel;
	init_props.verifydepth = var_client_tls_scert_vd;
	init_props.cache_type = TLS_MGR_SCACHE_CLIENT;
	init_props.cert_file = var_client_tls_cert_file;
	init_props.key_file = var_client_tls_key_file;
	init_props.dcert_file = var_client_tls_dcert_file;
	init_props.dkey_file = var_client_tls_dkey_file;
	init_props.eccert_file = var_client_tls_eccert_file;
	init_props.eckey_file = var_client_tls_eckey_file;
	init_props.CAfile = var_client_tls_CAfile;
	init_props.CApath = var_client_tls_CApath;
	init_props.fpt_dgst = var_client_tls_fpt_dgst;

	tls_client_setup(&init_props);
#endif
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -c conf_file -s server_addr\n", procname);
}

#if 1
static char *request = "GET /accounts/ServiceLogin?service=mail&passive=true&rm=false&continue=https%3A%2F%2Fmail.google.com%2Fmail%2F%3Fui%3Dhtml%26zy%3Dl&bsv=zpwhtygjntrz&ss=1&scc=1&ltmpl=default&ltmplcache=2 HTTP/1.1\r\n"
			"HOST: www.google.com\r\n"
			"\r\n";
#elif 1
static char *request = "GET / HTTP/1.1\r\n"
			"HOST: www.google.com\r\n"
			"\r\n";
#elif 1
static char *request = "GET /urchin.js HTTP/1.1\r\n"
			"HOST: ssl.google-analytics.com\r\n"
			"\r\n";
#endif

static void run_one(TLS_APPL_STATE *client_tls_ctx)
{
	ACL_VSTREAM *client;
	TLS_SESS_STATE *client_sess_ctx;
	TLS_CLIENT_START_PROPS tls_props;
	char  buf[4096];
	int   i, ret;
	time_t last, now;

	int   tls_level = 2;
	char *tls_nexthop = "test.com.cn";
	char *host = "test.com.cn";
	char *namaddrport = "test.com.cn";
	char *serverid = "service:addr:port:helo";
	char *tls_protocols = SSL_TXT_TLSV1;
	char *tls_grade = "low";  /* high, medium, low, export, null */
	char *tls_exclusions = "";
	ACL_ARGV *tls_matchargv = 0;

	client = acl_vstream_connect(serv_addr, ACL_BLOCKING, 20, 30, 8192);
	if (client == NULL) {
		printf("connect %s error(%s)\n", serv_addr, acl_last_serror());
		return;
	}

	acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(client));

#if 0
	client_sess_ctx = TLS_CLIENT_START(&tls_props,
			ctx = client_tls_ctx,
			stream = client,
			log_level = var_client_tls_loglevel,
			timeout = var_client_starttls_tmout,
			tls_level = tls_level,
			nexthop = tls_nexthop,
			host = host,
			namaddr = namaddrport,
			serverid = serverid,
			protocols = tls_protocols,
			cipher_grade = tls_grade,
			cipher_exclusions = tls_exclusions,
			matchargv = tls_matchargv,
			fpt_dgst = var_client_tls_fpt_dgst);
#else
	tls_props.ctx = client_tls_ctx;
	tls_props.stream = client;
	tls_props.log_level = var_client_tls_loglevel;
	tls_props.timeout = var_client_starttls_tmout;
	tls_props.tls_level = tls_level;
	tls_props.nexthop = tls_nexthop;
	tls_props.host = host;
	tls_props.namaddr = namaddrport;
	tls_props.serverid = serverid;
	tls_props.protocols = tls_protocols;
	tls_props.cipher_grade = tls_grade;
	tls_props.cipher_exclusions = tls_exclusions;
	tls_props.matchargv = tls_matchargv;
	tls_props.fpt_dgst = var_client_tls_fpt_dgst;

	client_sess_ctx = tls_client_start(&tls_props);
#endif

	if (client_sess_ctx == NULL) {
		printf("TLS_CLIENT_START error\n");
		acl_vstream_close(client);
		return;
	}

	if (tls_level >= TLS_LEV_VERIFY) {
		if (!TLS_CERT_IS_TRUSTED(client_sess_ctx)) {
			printf("Server certificate not trusted\n");
		}
	}

	if (tls_level > TLS_LEV_ENCRYPT) {
		if (!TLS_CERT_IS_MATCHED(client_sess_ctx)) {
			printf("Server certificate not verified\n");
		}
	}

	time(&last);
	i = 0;
	while (1) {
		ret = acl_vstream_fprintf(client, "hello world\n");
		if (ret == ACL_VSTREAM_EOF)
			goto END;
		ret = acl_vstream_gets(client, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			goto END;
		break;
		i++;
		if (i % 50000 == 0) {
			time(&now);
			printf("client: i=%d, time=%ld\n", i, now - last);
			last = now;
		}
	}

if (0)
{
	if (acl_vstream_writen(client, request, strlen(request)) == ACL_VSTREAM_EOF)
		printf("write request error\n");
	else {
		while (1) {
			if (acl_vstream_gets_nonl(client, buf, sizeof(buf)) == ACL_VSTREAM_EOF)
				break;
			/*
			printf("%s\n", buf);
			*/
		}
		/*
		printf("gets respond over now\n");
		*/
	}
}

END:
	tls_client_stop(client_tls_ctx, client, var_client_starttls_tmout, 0, client_sess_ctx);
	acl_vstream_close(client);
}

static void *run_thread(void *arg acl_unused)
{
	int   i, n = 1000000;
	time_t last = time(NULL), now;
	TLS_APPL_STATE *client_tls_ctx;

	client_tls_ctx = tls_client_create(&init_props);
	if (client_tls_ctx == NULL) {
		printf("tls_client_create error\n");
		return (NULL);
	}

	for (i = 0; i < n; i++) {
		run_one(client_tls_ctx);
		if (i % 100 == 0) {
			now = time(NULL);
			printf("i: %d, time: %ld\n", i, now - last);
			last = now;
		}
	}

	tls_free_app_context(client_tls_ctx);
	return (NULL);
}

static void run(void)
{
#define	MAX_THREAD	4
	acl_pthread_t tid[MAX_THREAD];
	acl_pthread_attr_t attr;
	int   i;
	void *tmp;

	acl_pthread_attr_init(&attr);
	/*
	acl_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	*/
	for (i = 0; i < MAX_THREAD; i++) {
		acl_pthread_create(&tid[i], &attr, run_thread, 0);
	}

	for (i = 0; i < MAX_THREAD; i++) {
		acl_pthread_join(tid[i], &tmp);
	}
}

int main(int argc, char *argv[])
{
	char  ch;

	acl_init();
	ACL_SAFE_STRNCPY(serv_addr, "127.0.0.1:443", sizeof(serv_addr));

	while ((ch = getopt(argc, argv, "hc:s:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			exit (0);
		case 'c':
			conf_file = acl_mystrdup(optarg);
			break;
		case 's':
			ACL_SAFE_STRNCPY(serv_addr, optarg, sizeof(serv_addr));
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
