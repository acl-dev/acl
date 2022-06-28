#ifndef	__DICT_POOL_INCLUDE_H__
#define	__DICT_POOL_INCLUDE_H__

#include "lib_acl.h"
#include "dict.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct DICT_POOL DICT_POOL;

typedef struct DICT_POOL_DB DICT_POOL_DB;

/**
 * 初始化，仅能被调用一次
 */
DICT_API void dict_pool_init(void);

/**
 * 创建一个存储池
 * @param partions {const char*} 存储分区情况的字符串数组
 * @param partions_size {int} partions 分区的个数
 * @param dict_type {const char*} 存储类型 (btree/hash/cdb)
 * @param dict_name {const char*} 存储池名
 * @param dict_size {int} 该存储池被分为几个存储分区
 * @return {DICT_POOL*} 存储池的对象指针, 若为 NULL 则表示创建失败
 */
DICT_API DICT_POOL *dict_pool_new(const char **partions, int partions_size,
	const char *dict_type, const char *dict_path,
	const char *dict_name, int pool_size);

/**
 * 关闭并释放一个存储池
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 */
DICT_API void dict_pool_free(DICT_POOL *pool);

/**
 * 添加一个 key/value 对至一个存储池
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 * @param key {char*} 健值地址
 * @param key_len {size_t} key 的长度
 * @param value {char*} 数据地址
 * @param len {size_t} value 数据的长度大小(字节)
 * @return {int} 0: OK, < 0: Error
 */
DICT_API int  dict_pool_set(DICT_POOL *pool, char *key, size_t key_len, char *value, size_t len);

/**
 * 从一个存储池中读取数据
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 * @param key {char*} 健值地址
 * @param key_len {size_t} key 的长度
 * @param size {size_t*} 存储所读到数据对象的长度大小(字节)
 * @param {char*} 所读到的数据地址，当为空时则表示该对象不存在,
 * 注：如果返回的数据地址不为空，则调用者需在用完该返回数据对象后
 *     用 acl_myfree/1 释放掉该返回的动态内存地址
 */
DICT_API char *dict_pool_get(DICT_POOL *pool, char *key, size_t key_len, size_t *size);

/**
 * 从一个存储池中删除数据
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 * @param key {char*} 健值地址
 * @param key_len {size_t} key 的长度
 * @return {int} 0: ok; != 0: error
 */
DICT_API int  dict_pool_del(DICT_POOL *pool, char *key, size_t key_len);

/**
 * 遍历存储中的所有数据
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 * @param key {char**} 如果结果非空则存储健值结果, 如果返回值不为空，
 *  则释放 key 需要调用: acl_myfree/1, 否则会造成内存泄露
 * @param key_size {size_t*} 若结果非空则存储健值结果的长度
 * @param val {char**} 如果结果非空则存储数据结果, 如果返回值不为空，
 *  则释放 val 需要调用: acl_myfree/1, 否则 会造成内存泄露
 * @param val_size {size_t*} 如果结果非空则存储数据结果的长度
 * @return {int} 0: 表示结果非空, != 0: 表示结果为空
 */
DICT_API int dict_pool_seq(DICT_POOL *pool, char **key, size_t *key_size,
        char **val, size_t *val_size);

/**
 * 重置存储遍历对象
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 */
DICT_API void dict_pool_seq_reset(DICT_POOL *pool);

/**
 * 删除当前数据遍历光标所指的数据
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 * @return {int} 0: 删除OK, != 0: 不存在或删除失败
 */
DICT_API int dict_pool_seq_delcur(DICT_POOL *pool);

/**
 * 根据健值从存储池中获得该健所在的存储DB
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 * @param key {char*} 健值地址
 * @param key_len {size_t} key 的长度
 * @return {DICT_POOL_DB*} 存储DB
 */
DICT_API DICT_POOL_DB *dict_pool_db(DICT_POOL *pool, const char *key, size_t key_len);

/**
 * 获得某存储DB的文件全路径
 * @param pool {DICT_POOL*} 某个存储池的对象指针
 * @return {const char*} 存储文件名
 */
DICT_API const char *dict_pool_db_path(DICT_POOL_DB *db);

/**
 * 加锁某个存储DB
 * @param {DICT_POOL_DB*} 存储DB
 */
DICT_API void dict_pool_db_lock(DICT_POOL_DB *db);

/**
 * 解锁某个存储DB
 * @param {DICT_POOL_DB*} 存储DB
 */
DICT_API void dict_pool_db_unlock(DICT_POOL_DB *db);

/**
 * 添加或修改数据
 * @param {DICT_POOL_DB*} 存储DB
 * @param key {char*} 健值地址
 * @param key_len {size_t} key 的长度
 * @param value {char*} 值
 * @param len {size_t} value 长度
 * @return {int} 0: ok; < 0: error
 */
DICT_API int  dict_pool_db_set(DICT_POOL_DB *db, char *key, size_t key_len, char *value, size_t len);

/**
 * 从存储DB中获得某个健所对应的值
 * @param {DICT_POOL_DB*} 存储DB
 * @param key {char*} 健值地址
 * @param key_len {size_t} key 的长度
 * @param size {size_t*} 如果查询结果非空则存储返回结果的长度
 * @return {char*} 健所对应的值, 需要用 acl_myfree()/1 释放
 */
DICT_API char *dict_pool_db_get(DICT_POOL_DB *db, char *key, size_t key_len, size_t *size);

/**
 * 从存储DB中删除某个健对应的值
 * @param {DICT_POOL_DB*} 存储DB
 * @param key {char*} 健值地址
 * @param key_len {size_t} key 的长度
 * @return {int} 0: ok; != 0: error
 */
DICT_API int dict_pool_db_del(DICT_POOL_DB *db, char *key, size_t key_len);

/**
 * 重置之前曾遍历的存储DB
 * @param {DICT_POOL_DB*} 存储DB
 */
DICT_API void dict_pool_db_seq_reset(DICT_POOL_DB *db);

/**
 * 遍历某个存储DB
 * @param {DICT_POOL_DB*} 存储DB
 * @param key {char**} 存储健的地址
 * @param key_size {size_t*} 存储在 key 中数据的长度
 * @param value {char**} 存储值的地址
 * @param value_size {size_t*} 存储在 value 中数据的长度
 * @return {int} 0: 表示结果非空, != 0: 表示结果为空
 */
DICT_API int dict_pool_db_seq(DICT_POOL_DB *db, char **key, size_t *key_size,
        char **val, size_t *val_size);

#ifdef	__cplusplus
}
#endif

#endif
