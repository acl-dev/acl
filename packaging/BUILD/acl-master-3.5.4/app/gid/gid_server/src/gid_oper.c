#include "lib_acl.h"
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "gid_oper.h"

typedef struct GID_STORE {
	ACL_FHANDLE fh;		/* 文件句柄 */
	ACL_VSTREAM *logger;	/* 日志句柄 */
	char tag[64];		/* 文件标识号 */
	char sid[64];		/* 会话ID号，相当于密码 */
	unsigned int step;	/* 步进值 */
	acl_int64  cur_gid;	/* 当前的ID值 */
	acl_int64  min_gid;	/* 最小的ID值 */
	acl_int64  max_gid;	/* 最大的ID值 */
} GID_STORE;

typedef struct {
	char tag[64];		/* 文件标识号 */
	char sid[64];		/* 会话ID号，相当于密码 */
	unsigned int step;	/* 步进值 */
	acl_int64  cur_gid;	/* 当前的ID值 */
	acl_int64  min_gid;	/* 最小的ID值 */
	acl_int64  max_gid;	/* 最大的ID值 */
} GID_STORE_CTX;

static int __sync_gid = 1;

/* 同步内存数据至磁盘 */

static int gid_store_sync(GID_STORE *store)
{
	char  buf[1024];

	/* 需要先将文件内容清空 */
#if 0
	if (acl_file_ftruncate(store->fh.fp, 0) < 0) {
		acl_msg_error("%s(%d), %s: ftruncate %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			ACL_VSTREAM_PATH(store->fh.fp), acl_last_serror());
		return (-1);
	}
#endif
	if (acl_vstream_fseek(store->fh.fp, SEEK_SET, 0) < 0) {
		acl_msg_error("%s(%d), %s: fseek %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			ACL_VSTREAM_PATH(store->fh.fp), acl_last_serror());
	}

	snprintf(buf, sizeof(buf), "%s:%s %d %lld %lld %lld\r\n",
		store->tag, store->sid, store->step, store->cur_gid,
		store->min_gid, store->max_gid);

	/* 初始化文件内容: tag:sid step cur_gid min_gid max_gid\r\n */
	if (acl_vstream_writen(store->fh.fp,
		buf, strlen(buf)) == ACL_VSTREAM_EOF)
	{
		acl_msg_error("%s(%d), %s: write to %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			ACL_VSTREAM_PATH(store->fh.fp), acl_last_serror());
		return (-1);
	}

	return (0);
}

/* 文件打开时的回调函数，有可能是打开了缓存句柄对象 */

static int gid_store_on_open(ACL_FHANDLE *fh, void *ctx)
{
	GID_STORE *store = (GID_STORE*) fh;
	GID_STORE_CTX *sc = (GID_STORE_CTX*) ctx;
	ACL_ARGV *argv;
	char  buf[512], *ptr;
	int   ret;

	/* 检查是否有问题 */
	if (fh->size != sizeof(GID_STORE))
		acl_msg_fatal("%s(%d), %s: size(%d) != GID_STORE's size(%d)",
			__FILE__, __LINE__, __FUNCTION__,
			(int) fh->size, (int) sizeof(GID_STORE));

	/* 如果是新文件则初始化 */
	if (fh->fsize == 0) {
		ACL_SAFE_STRNCPY(store->tag, sc->tag, sizeof(store->tag));
		ACL_SAFE_STRNCPY(store->sid, sc->sid, sizeof(store->sid));
		store->step = sc->step;
		store->cur_gid = sc->cur_gid;
		store->min_gid = sc->min_gid;
		store->max_gid = sc->max_gid;

		return (gid_store_sync(store));
	}

	/* 数据格式：tag:sid step cur_gid min_gid max_gid\r\n */

	/* 从文件中获得数据 */
	ret = acl_vstream_gets_nonl(fh->fp, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d), %s: gets from %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			ACL_VSTREAM_PATH(fh->fp), acl_last_serror());
		return (-1);
	}

	/* 拆分字符串 */
	argv = acl_argv_split(buf, ",\t ");
	if (argv->argc < 5) {
		acl_msg_error("%s(%d), %s: invalid line(%s) from %s",
			__FILE__, __LINE__, __FUNCTION__,
			buf, ACL_VSTREAM_PATH(fh->fp));
		acl_argv_free(argv);
		return (-1);
	}

	ACL_SAFE_STRNCPY(store->tag, argv->argv[0], sizeof(store->tag));
	ptr = strchr(store->tag, ':');
	if (ptr) {
		*ptr++ = 0;
		ACL_SAFE_STRNCPY(store->sid, ptr, sizeof(store->sid));
	}
	store->step = atoi(argv->argv[1]);

	/* 如果存储的步进值与参数中的步进值不一致，则优先采用参数步进值 */
	if (store->step != sc->step) {
		acl_msg_warn("%s(%d), %s: change step from %d to %d for %s",
			__FILE__, __LINE__, __FUNCTION__,
			store->step, sc->step, ACL_VSTREAM_PATH(fh->fp));
		store->step = sc->step;
	}
	store->cur_gid = atoll(argv->argv[2]);
	store->min_gid = atoll(argv->argv[3]);
	store->max_gid = atoll(argv->argv[4]);

	acl_argv_free(argv);
	return (0);
}

/* 文件句柄真正关闭时的回调函数 */

static void gid_store_on_close(ACL_FHANDLE *fh acl_unused)
{
	GID_STORE *store = (GID_STORE*) fh;

	/* 当设置了非实时同步标记时才需要在文件句柄关闭时同步磁盘 */
	if (__sync_gid == 0 && gid_store_sync(store) < 0)
		acl_msg_fatal("%s(%d), %s: save %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			ACL_VSTREAM_PATH(store->fh.fp), acl_last_serror());
}

/* 打开文件，如果文件不存在则创建新的文件，如果文件句柄已经存在则直接返回 */

static GID_STORE *gid_store_open(const char *path, const char *tag,
	const char *sid, unsigned int step)
{
	char  filepath[1024];
	unsigned int oflags = ACL_FHANDLE_O_NOATIME
		| ACL_FHANDLE_O_MLOCK
		| ACL_FHANDLE_O_EXCL;
	GID_STORE *store;
	GID_STORE_CTX ctx;

	ACL_SAFE_STRNCPY(ctx.tag, tag, sizeof(ctx.tag));
	ACL_SAFE_STRNCPY(ctx.sid, sid, sizeof(ctx.sid));
	ctx.step = step;
	ctx.cur_gid = 0;
	ctx.min_gid = 1;
	ctx.max_gid = 0x7FFFFFFFFFFFFFFFllu;

	snprintf(filepath, sizeof(filepath), "%s/%s", path, tag);
	store = (GID_STORE*) acl_fhandle_open(sizeof(GID_STORE), oflags,
			filepath, gid_store_on_open, &ctx, gid_store_on_close);
	if (store == NULL)
		acl_msg_fatal("%s(%d), %s: open %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			filepath, acl_last_serror());
	return (store);
}

/* 获得当前本地时间 */

static void logtime_fmt(char *buf, size_t size)
{
	time_t  now;
	struct tm local_time;

	(void) time (&now);
	(void) localtime_r(&now, &local_time);
	strftime(buf, size, "%Y/%m/%d %H:%M:%S", &local_time);
}

/* 记录日志 */

static void gid_logger(GID_STORE *store)
{
	char  buf[128];

	if (store->logger == NULL)
		return;

	logtime_fmt(buf, sizeof(buf));
	if (acl_vstream_fprintf(store->logger, "%s|%s %lld\r\n",
		buf, store->tag, store->cur_gid) == ACL_VSTREAM_EOF)
	{
		acl_msg_error("%s(%d), %s: fprintf to %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			ACL_VSTREAM_PATH(store->logger), acl_last_serror());
		store->logger = NULL;
	}
}

acl_int64 gid_next(const char *path, const char *tag_in,
	unsigned int step, int *errnum)
{
	acl_int64 gid;
	char  tag[128], *sid;
	GID_STORE *store;

	/* 需要从 tag 串中分离出 sid 串 */
	ACL_SAFE_STRNCPY(tag, tag_in, sizeof(tag));
	sid = strchr(tag, ':');
	if (sid)
		*sid++ = 0;
	else
		sid = "";
	store = gid_store_open(path, tag, sid, step);

	if (store->sid[0] != 0) {
		if (sid == NULL || strcmp(sid, store->sid) != 0) {
			acl_msg_error("%s(%d), %s: input sid(%s) invalid",
				__FILE__, __LINE__, __FUNCTION__,
				sid && *sid ? sid : "null");
			if (errnum)
				*errnum = GID_ERR_SID;

			/* 关闭文件句柄，延迟 60 秒后关闭 */
			acl_fhandle_close(&store->fh, 60);
			return (-1);
		}
	}

	/* 如果已经达到最大值，则崩溃了! */
	if (store->max_gid - (unsigned int) store->step <= store->cur_gid) {
		acl_msg_error("%s(%d), %s: %s Override!!, max_gid: %lld,"
			" step: %d, cur_gid: %lld", __FILE__, __LINE__,
			__FUNCTION__, ACL_VSTREAM_PATH(store->fh.fp),
			store->max_gid, store->step, store->cur_gid);

		if (errnum)
			*errnum = GID_ERR_OVERRIDE;
		/* 关闭文件句柄，延迟 60 秒后关闭 */
		acl_fhandle_close(&store->fh, 60);
		return (-1);
	}

	store->cur_gid += store->step;
	gid = store->cur_gid;

	if (__sync_gid && gid_store_sync(store) < 0) {
		if (errnum)
			*errnum = GID_ERR_SAVE;
		acl_msg_error("%s(%d), %s: save %s error(%s)",
			__FILE__, __LINE__, __FUNCTION__,
			ACL_VSTREAM_PATH(store->fh.fp),
			acl_last_serror());

		/* 关闭文件句柄，延迟 60 秒后关闭 */
		acl_fhandle_close(&store->fh, 60);
		return (-1);
	}

	/* 记日志 */
	if (store->logger)
		gid_logger(store);

	/* 关闭文件句柄，延迟 60 秒后关闭 */
	acl_fhandle_close(&store->fh, 60);

	if (errnum)
		*errnum = GID_OK;
	return (gid);
}

const char *gid_serror(int errnum)
{
	static const struct {
		int  err;
		const char *str;
	} errors[] = {
		{ GID_OK, "ok" },
		{ GID_ERR_SID, "sid invalid" },
		{ GID_ERR_OVERRIDE, "gid override" },
		{ GID_ERR_SAVE, "gid save error" },
		{ 0, 0 }
	};
	static const char *unknown = "unkown error";
	int   i;

	for (i = 0; errors[i].str != NULL; i++) {
		if (errnum == errors[i].err)
			return (errors[i].str);
	}
	return (unknown);
}

void gid_init(int fh_limit, int sync_gid, int debug_section)
{
	__sync_gid = sync_gid;
	 acl_fhandle_init(fh_limit, debug_section, ACL_FHANDLE_F_LOCK);
}

void gid_finish()
{
	acl_fhandle_end();
}
