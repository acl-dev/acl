
#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct USER_INFO {
	char  user_name[256];
	char  user_age[4];
	char  user_sex[8];
	char  user_phone[20];
	char  user_home[20];
} USER_INFO;

static const char *db_name = "test.db";
static const char *tab_name = "test.user_info";
static ACL_MDB *mdb;
static const char *key_names[] =
	{ "user_name", "user_home", "user_age", "user_sex", "home_sex", NULL };
static unsigned int key_flags[] =
	{ ACL_MDT_FLAG_UNI, 0, 0, 0, 0, 0 };

static ACL_SLICE *__slice;

static void create_db(void)
{
	ACL_MDT *mdt;

	// 创建数据库及数据表
	mdb = acl_mdb_create(db_name, "avl");
	//mdb = acl_mdb_create(db_name, "binhash");
	//mdb = acl_mdb_create(db_name, "hash");

	mdt = acl_mdb_tbl_create(mdb, tab_name,
			ACL_MDT_FLAG_NUL,
			/* ACL_MDT_FLAG_SLICE1, */
			/* ACL_MDT_FLAG_SLICE_RTGC_OFF, */
			100, key_names, key_flags);
	acl_assert(mdt);
	__slice = acl_slice_create("for USER_INFO", 0, sizeof(USER_INFO), ACL_SLICE_FLAG_GC1);
}

static void close_db(void)
{
	acl_mdb_free(mdb);
	acl_slice_destroy(__slice);
}

static void add_user(void)
{
	const char *key_values1[] = { "王五", "北京", "26", "male", "北京男生", NULL };
	const char *key_values2[] = { "王燕", "上海", "26", "female", "上海女生", NULL };
	const char *key_values3[] = { "王云", "上海", "25", "female", "上海女生", NULL };
	const char *key_values4[] = { "王燕", "上海", "26", "female", "上海女生", NULL };
	const char *key_values5[] = { "王云", "上海", "25", "female", "上海女生", NULL };

	USER_INFO *pinfo;
	USER_INFO info1 = { "王五", "26", "male", "1111111", "北京" };
	USER_INFO info2 = { "王燕", "26", "female", "1111111", "上海" };
	USER_INFO info3 = { "王云", "25", "female", "1111111", "上海" };
	USER_INFO info4 = { "王燕", "26", "female", "1111111", "上海" };
	USER_INFO info5 = { "王云", "25", "female", "1111111", "上海" };

	printf(">>>开始添加用户...\r\n");

	// 添加数据记录
	pinfo = &info1;
	(void) acl_mdb_add(mdb, tab_name, pinfo, sizeof(USER_INFO), key_names, key_values1);
	printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
		info1.user_name, info1.user_age, info1.user_sex, info1.user_home);

	pinfo = (USER_INFO*) acl_slice_calloc(__slice);
	memcpy(pinfo, &info2, sizeof(USER_INFO));
	(void) acl_mdb_add(mdb, tab_name, pinfo, sizeof(USER_INFO), key_names, key_values2);
	printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
		pinfo->user_name, pinfo->user_age, pinfo->user_sex, pinfo->user_home);

	pinfo = (USER_INFO*) acl_slice_calloc(__slice);
	memcpy(pinfo, &info3, sizeof(USER_INFO));
	(void) acl_mdb_add(mdb, tab_name, pinfo, sizeof(USER_INFO), key_names, key_values3);
	printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
		pinfo->user_name, pinfo->user_age, pinfo->user_sex, pinfo->user_home);


	(void) acl_mdb_add(mdb, tab_name, &info4, sizeof(USER_INFO), key_names, key_values4);
	(void) acl_mdb_add(mdb, tab_name, &info5, sizeof(USER_INFO), key_names, key_values5);

	printf("\r\n");
}

static void search_user(void)
{
	ACL_MDT_RES *res;
	const USER_INFO *pinfo;

	// 查询所有 user_sex 为 male 的数据对象
	res = acl_mdb_find(mdb, tab_name, "user_sex", "male", 0, 0);
	printf(">>>查询 user_sex=male results ...\r\n");
	if (res) {
		while (1) {
			pinfo = (const USER_INFO*) acl_mdt_fetch_row(res);
			if (pinfo == NULL)
				break;
			printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
				pinfo->user_name, pinfo->user_age, pinfo->user_sex, pinfo->user_home);
		}
		acl_mdt_res_free(res);
	}

	// 查询所有 user_sex 为 male 的数据对象
	res = acl_mdb_find(mdb, tab_name, "user_age", "26", 0, 0);
	printf(">>>查询 user_age=26 results ...\r\n");
	if (res) {
		while (1) {
			pinfo = (const USER_INFO*) acl_mdt_fetch_row(res);
			if (pinfo == NULL)
				break;
			printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
				pinfo->user_name, pinfo->user_age, pinfo->user_sex, pinfo->user_home);
		}
		acl_mdt_res_free(res);
	}

	// 查询所有北京的用户
	res = acl_mdb_find(mdb, tab_name, "user_home", "北京", 0, 0);
	printf(">>>查询 user_home=北京 results ...\r\n");
	if (res) {
		while (1) {
			pinfo = (const USER_INFO*) acl_mdt_fetch_row(res);
			if (pinfo == NULL)
				break;
			printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
				pinfo->user_name, pinfo->user_age, pinfo->user_sex, pinfo->user_home);
		}
		acl_mdt_res_free(res);
	}

	// 查询所有上海女生
	res = acl_mdb_find(mdb, tab_name, "home_sex", "上海女生", 0, 0);
	printf(">>>查询 上海女生 results ...\r\n");
	if (res) {
		while (1) {
			pinfo = (const USER_INFO*) acl_mdt_fetch_row(res);
			if (pinfo == NULL)
				break;
			printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
				pinfo->user_name, pinfo->user_age, pinfo->user_sex, pinfo->user_home);
		}
		acl_mdt_res_free(res);
	}
}

static void delete_user()
{
	int   n;

	printf(">>>删除 上海女生 ...\r\n");
	n = acl_mdb_del(mdb, tab_name, "home_sex", "上海女生", NULL);
	printf(">>>ok, delete total number=%d\r\n", n);
	acl_mdb_del(mdb, tab_name, "home_sex", "上海女生", NULL);
	printf(">>>删除 北京 用户 ...\r\n");
	n = acl_mdb_del(mdb, tab_name, "user_home", "北京", NULL);
	printf(">>>ok, delete total number=%d\r\n", n);
}

static int walk_fn(const void *data, unsigned int dlen acl_unused)
{
	const USER_INFO *pinfo = (const USER_INFO*) data;

	printf("\tuser_name(%s), user_age(%s), user_sex(%s), user_home(%s)\r\n",
		pinfo->user_name, pinfo->user_age, pinfo->user_sex, pinfo->user_home);

	return (0);
}

static void walk_table(void)
{
	int   ret;

	printf(">>>遍历数据表\r\n");

	ret = acl_mdb_walk(mdb, tab_name, walk_fn, 0, 0);
	printf(">>>table: %s's total number=%d\r\n", tab_name, ret);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	create_db();
	add_user();
	printf("\r\n");

	walk_table();
	printf("\r\n");

	search_user();
	printf("\r\n");

	delete_user();
	printf("\r\n");

	search_user();
	printf("\r\n");

	walk_table();

	close_db();
	getchar();
	return (0);
}
