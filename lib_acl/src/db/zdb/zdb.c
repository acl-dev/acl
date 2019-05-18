#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <stdio.h>
#include <string.h>
#include "stdlib/acl_make_dirs.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_file.h"
#include "db/zdb.h"

#endif

#ifndef ACL_CLIENT_ONLY

#include "zdb_private.h"

/*----------------------------------------------------------------------------*/

void zdb_init()
{
	/* 初始化文件句柄 */
	acl_fhandle_init(1000, 100, 0);
}

void zdb_end()
{
	/* 需要关闭所有存储句柄 */
	acl_fhandle_end();
}

#define	PATH_LEN	256
#define	IDISK_LEN	32
#define	PRIORITY_LEN	32
#define	LIMIT_LEN	32
#define	COUNT_LEN	32
#define	INFO_LEN	(PATH_LEN + IDISK_LEN + PRIORITY_LEN + LIMIT_LEN + COUNT_LEN + 2)
#define	ITEM_CNT	5

/* 存储格式: {path} {idisk} {priority} {limit} {count} */

static void free_disk(void *arg)
{
	ZDB_DISK *disk = (ZDB_DISK*) arg;

	if (disk->path)
		acl_myfree(disk->path);
	acl_myfree(disk);
}

static ZDB_DISK *zdb_disks_load(const char *dbname, const char *dbpath)
{
	const char *myname = "zdb_disks_load";
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	ACL_FILE *fp = NULL;
	char  disk_info[INFO_LEN + 1];
	ZDB_DISK *disk, *disks;
	ACL_ARRAY *a = NULL;
	ACL_ITER iter;
	int   n, i;

#undef	RETURN
#define	RETURN(x) do {  \
	if (fp)  \
		acl_fclose(fp);  \
	acl_vstring_free(buf);  \
	if (a)  \
		acl_array_destroy(a, free_disk);  \
	return (x);  \
} while (0)

	acl_vstring_sprintf(buf, "%s/.%s.disk", dbpath, dbname);
	fp = acl_fopen(STR(buf), "r");
	if (fp == NULL) {
		acl_msg_error("%s(%d): fopen(%s) error(%s)",
			myname, __LINE__, STR(buf), acl_last_serror());
		RETURN (NULL);
	}

	a = acl_array_create(10);
	while (1) {
		ACL_ARGV *argv;

		if (acl_fgets_nonl(disk_info, sizeof(disk_info), fp) == NULL)
			break;
		argv = acl_argv_split(disk_info, "|");
		if (argv->argc != ITEM_CNT) {
			acl_msg_error("%s(%d): invalid line(%s)",
				myname, __LINE__, disk_info);
			acl_argv_free(argv);
			continue;
		}
		disk = (ZDB_DISK*) acl_mycalloc(1, sizeof(ZDB_DISK));
		disk->path = acl_mystrdup(argv->argv[0]);
		disk->idisk = atoi(argv->argv[1]);
		disk->priority = atoi(argv->argv[2]);
		disk->limit = acl_atoui64(argv->argv[3]);
		disk->count = acl_atoui64(argv->argv[4]);
		if (acl_array_append(a, disk) < 0)
			acl_msg_fatal("%s(%d): add disk error(%s)",
				myname, __LINE__, acl_last_serror());
		acl_argv_free(argv);
	}

	n = acl_array_size(a);
	if (n <= 0) {
		acl_msg_error("%s(%d): empty array of ZDB_DISK", myname, __LINE__);
		RETURN (NULL);
	}

	disks = (ZDB_DISK*) acl_mycalloc(n + 1, sizeof(ZDB_DISK));
	i = 0;
	acl_foreach(iter, a) {
		disk = (ZDB_DISK*) iter.data;
		disks[i].limit = disk->limit;
		disks[i].count = disk->count;
		disks[i].path = acl_mystrdup(disk->path);
		disks[i].idisk = disk->idisk;
		disks[i].priority = disk->priority;
		disks[i].dat_ifiles = NULL;
		disks[i].dat_ifiles_size = 0;
		if (disks[i].idisk != i) {
			acl_msg_error("%s(%d): idisk(%d) != %d invalid for %s",
				myname, __LINE__, disks[i].idisk, i, disks[i].path);
			acl_myfree(disks);
			RETURN (NULL);
		}
		i++;
	}

	disks[i].path = NULL;  /* 将最后一个置空表示结束 */
	RETURN (disks);
}

static void zdb_disks_update(const char *dbname, const char *dbpath, const ZDB_DISK *disks)
{
	const char *myname = "zdb_disks_update";
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	ACL_FILE *fp;
	char  tmp[32];
	int   i;

	acl_vstring_sprintf(buf, "%s/.%s.disk", dbpath, dbname);
	fp = acl_fopen(STR(buf), "w");
	if (fp == NULL) {
		acl_msg_error("%s(%d): fopen(%s) error(%s)",
			myname, __LINE__, STR(buf), acl_last_serror());
		acl_vstring_free(buf);
		return;
	}

	/* 存储格式: {path} {idisk} {priority} {limit} {count} */

	i = 0;
	for (i = 0; disks[i].path != NULL; i++) {
		acl_vstring_sprintf(buf, "%s|%d|%d",
			disks[i].path, disks[i].idisk, disks[i].priority);
		acl_vstring_strcat(buf, "|");
		acl_ui64toa(disks[i].limit, tmp, sizeof(tmp));
		acl_vstring_strcat(buf, tmp);
		acl_vstring_strcat(buf, "|");
		acl_ui64toa(disks[i].count, tmp, sizeof(tmp));
		acl_vstring_strcat(buf, tmp);
		if (acl_fputs(STR(buf), fp) == EOF) {
			acl_msg_error("%s(%d): fputs to %s/%s.disks error(%s)",
				myname, __LINE__, dbname, dbpath, acl_last_serror());
			break;
		}
	}

	acl_vstring_free(buf);
	acl_fclose(fp);
}

int zdb_disk_select(ZDB *db)
{
	const char *myname = "zdb_disk_select";
	int   idisk = -1, i;
	acl_int64 count = ((acl_int64) 1 << 62);

	for (i = 0; db->dat_disks[i].path != NULL; i++) {
		if (db->dat_disks[i].count < count) {
			count = db->dat_disks[i].count;
			idisk = i;
		}
	}

	if (idisk == -1) {
		acl_msg_error("%s(%d): idisk(%d) < 0, i(%d), no disk available",
			myname, __LINE__, idisk, i);
	} else {
		db->dat_disks[idisk].count++;
	}

	return (idisk);
}

void zdb_sync(ZDB *db)
{
	zdb_disks_update(db->dbname, db->key_path, db->dat_disks);
}

ZDB *zdb_open(const char *dbname, unsigned int oflags, const ZDB_CFG *cfg)
{
	const char *myname = "zdb_open";
	ZDB *db;
	ZDB_DISK *disks;
#ifdef	INCLUDE_PATH
	int   i;
	ACL_VSTRING *buf = acl_vstring_alloc(256);
#endif

	/* 先保证存储目录存在 */
	acl_make_dirs(cfg->key_path, 0700);

	disks = zdb_disks_load(dbname, cfg->key_path);
	if (disks == NULL) {
		acl_msg_error("%s(%d): zdb_disks_load error",
			myname, __LINE__);
		return (NULL);
	}

	db = (ZDB*) acl_mycalloc(1, sizeof(ZDB));

	db->dbname = acl_mystrdup(dbname);
	db->key_path = acl_mystrdup(cfg->key_path);
	db->oflags = oflags;
	db->dat_disks = disks;

	if (cfg->key_cache_max <= 0 || cfg->key_cache_timeout <= 0) {
		db->oflags &=~ZDB_FLAG_CACHE_KEY;
		db->key_cache_max = 0;
		db->key_cache_timeout = 0;
		db->key_wback_max = 0;
	} else {
		db->key_cache_max = cfg->key_cache_max;
		db->key_cache_timeout = cfg->key_cache_timeout;
		db->key_wback_max = cfg->key_wback_max;
	}

	if (cfg->dat_cache_max <= 0 || cfg->dat_cache_timeout <= 0) {
		db->oflags &= ~ZDB_FLAG_CACHE_DAT;
		db->dat_cache_max = 0;
		db->dat_cache_timeout = 0;
		db->dat_wback_max = 0;
	} else {
		db->dat_cache_max = cfg->dat_cache_max;
		db->dat_cache_timeout = cfg->dat_cache_timeout;
		db->dat_wback_max = cfg->dat_wback_max;
	}


	db->path_tmp = acl_vstring_alloc(256);

	db->key_begin = cfg->key_begin;
	db->key_limit = cfg->key_limit;

	db->dat_limit = cfg->dat_limit;
	db->blk_dlen = cfg->blk_dlen;
	db->dat_nstep = cfg->dat_nstep;

	db->key_get = zdb_key_get;
	db->key_set = zdb_key_set;
	db->dat_get = zdb_dat_get;
	db->dat_add = zdb_dat_add;
	db->dat_update = zdb_dat_update;

#ifdef	INCLUDE_PATH
	for (i = 0; i < ZDB_KEY_DIR_LIMIT; i++) {
		acl_vstring_sprintf(buf, "%s/%d", cfg->key_path, i);
		acl_make_dirs(STR(buf), 0700);
	}
	acl_vstring_free(buf);
#endif
	return (db);
}

void zdb_close(ZDB *db)
{
	int   i;

	zdb_sync(db);  /* 先同步磁盘分区信息至磁盘 */
	acl_myfree(db->dbname);
	acl_myfree(db->key_path);
	acl_vstring_free(db->path_tmp);

	for (i = 0; db->dat_disks[i].path != NULL; i++) {
		if (db->dat_disks[i].dat_ifiles)
			acl_myfree(db->dat_disks[i].dat_ifiles);
		acl_myfree(db->dat_disks[i].path);
	}
	acl_myfree(db->dat_disks);
	acl_myfree(db);
}

ZDB_BLK *zdb_lookup(ZDB *db, zdb_key_t key, size_t *size_ptr, ZDB_BLK_OFF *blk_off_buf)
{
	const char *myname = "zdb_lookup";
	ZDB_BLK_OFF blk_off;
	zdb_key_t key_tmp;
	ZDB_BLK *blk;
	int   ret;

	if (key < db->key_begin) {
		acl_msg_error("%s(%d): key(" ACL_FMT_I64D ") < key_begin("
			ACL_FMT_I64D "), invalid",
			myname, __LINE__, key, db->key_begin);
		return (NULL);
	}

	ret = db->key_get(db, key, &blk_off);
	if (ret <= 0)
		return (NULL);

	blk = db->dat_get(db, &blk_off, &key_tmp, size_ptr);
	if (blk == NULL) {
		acl_msg_error("%s(%d): zdb_dat_get null for key("
			ACL_FMT_I64D ")", myname, __LINE__, key);
		if (blk_off_buf) {
			blk_off_buf->offset = -1;
			blk_off_buf->inode = -1;
		}
		return (NULL);
	}

	if (blk_off_buf)
		memcpy(blk_off_buf, &blk_off, sizeof(blk_off));

	/* 必须判断键的一致性 */

	if (key != key_tmp) {
		acl_msg_warn("%s(%d): key(" ACL_FMT_I64D ") != key_tmp("
			ACL_FMT_I64D "), blk_off: " ACL_FMT_I64D ", inode: %d",
			myname, __LINE__, key, key_tmp,
			blk_off.offset, blk_off.inode & DIR_MASK);
		zdb_blk_free(blk);
		return (NULL);
	}

	return (blk);
}

int zdb_update(ZDB *db, zdb_key_t key, const ZDB_BLK_OFF *blk_off_saved,
	const void *dat, size_t len)
{
	const char *myname = "zdb_update";
	ZDB_BLK_OFF blk_off;
	int   ret;

	if (key < db->key_begin) {
		acl_msg_error("%s(%d): key(" ACL_FMT_I64D ") < key_begin("
			ACL_FMT_I64D "), invalid",
			myname, __LINE__, key, db->key_begin);
		return (-1);
	}

	if (dat == NULL) {
		acl_msg_error("%s(%d): data null", myname,  __LINE__);
		return (-1);
	}

	if (len <= 0) {
		acl_msg_error("%s(%d): len(%d) invalid", myname, __LINE__, (int) len);
		return (-1);
	}

	/* 可以重复利用传入的 blk_off_saved (此值是上次调用 zdb_lookup 获得) */

	if (blk_off_saved == NULL || blk_off_saved->offset < 0) {
		blk_off_saved = NULL;
		ret = db->key_get(db, key, &blk_off);
		if (ret < 0) {
			acl_msg_error("%s(%d): key_get key(" ACL_FMT_I64D ") error(%s)",
				myname, __LINE__, key, acl_last_serror());
			return (-1);
		}
		if (ret > 0)
			blk_off_saved = &blk_off;
	}

	if (blk_off_saved == NULL) {
		db->status |= ZDB_STAT_KEY_NEW;  /* 设置状态位以表明当前为新值 */
		ret = db->dat_add(db, key, dat, (int) len);
		db->status &= ~ZDB_STAT_KEY_NEW;  /* 清除标志位 */
	} else {
		db->status &=~ ZDB_STAT_KEY_NEW;  /* 清除标志位表明是修改旧数据 */
		ret = db->dat_update(db, key, blk_off_saved, dat, len);
	}

	return (ret);
}

/*----------------------------------------------------------------------------*/
#endif /* ACL_CLIENT_ONLY */
