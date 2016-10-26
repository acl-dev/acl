#ifndef ACL_JSON_INCLUDE_H
#define ACL_JSON_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_dbuf_pool.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_array.h"

typedef struct ACL_JSON ACL_JSON;
typedef struct ACL_JSON_NODE ACL_JSON_NODE;

struct ACL_JSON_NODE {
	ACL_VSTRING *ltag;          /**< ��ǩ�� */
	ACL_VSTRING *text;          /**< ���ڵ�ΪҶ�ڵ�ʱ���ı����ݷǿ� */
	ACL_JSON_NODE *tag_node;    /**< ����ǩֵΪ json �ڵ�ʱ����ǿ� */
	int   type;                 /**< �ڵ����� */
#define	ACL_JSON_T_A_STRING      (1 << 0)
#define	ACL_JSON_T_A_NUMBER      (1 << 1)
#define	ACL_JSON_T_A_BOOL        (1 << 2)
#define	ACL_JSON_T_A_NULL        (1 << 3)
#define	ACL_JSON_T_A_DOUBLE      (1 << 4)

#define	ACL_JSON_T_STRING        (1 << 5)
#define	ACL_JSON_T_NUMBER        (1 << 6)
#define	ACL_JSON_T_BOOL          (1 << 7)
#define	ACL_JSON_T_NULL          (1 << 8)
#define ACL_JSON_T_DOUBLE        (1 << 9)

#define ACL_JSON_T_ARRAY         (1 << 10)
#define ACL_JSON_T_OBJ           (1 << 11)
#define ACL_JSON_T_LEAF          (1 << 12)
#define ACL_JSON_T_MEMBER        (1 << 13)
#define ACL_JSON_T_PAIR          (1 << 14)
#define	ACL_JSON_T_ELEMENT       (1 << 15)

	ACL_JSON_NODE *parent;      /**< ���ڵ� */
	ACL_RING children;          /**< �ӽڵ㼯�� */
	int  depth;                 /**< ��ǰ�ڵ����� */

	/* private */
	ACL_JSON *json;             /**< json ���� */
	ACL_RING node;              /**< ��ǰ�ڵ� */
	int   quote;                /**< �� 0 ��ʾ ' �� " */
	int   left_ch;              /**< ���ڵ�ĵ�һ���ַ�: { or [ */
	int   right_ch;             /**< ���ڵ�����һ���ַ�: } or ] */
	int   backslash;            /**< ת���ַ� \ */
	int   part_word;            /**< ������ֵ���������־λ */

	/* public: for acl_iterator, ͨ�� acl_foreach �г��ýڵ��һ���ӽڵ� */

	/* ȡ������ͷ���� */
	ACL_JSON_NODE *(*iter_head)(ACL_ITER*, ACL_JSON_NODE*);
	/* ȡ��������һ������ */
	ACL_JSON_NODE *(*iter_next)(ACL_ITER*, ACL_JSON_NODE*);
	/* ȡ������β���� */
	ACL_JSON_NODE *(*iter_tail)(ACL_ITER*, ACL_JSON_NODE*);
	/* ȡ��������һ������ */
	ACL_JSON_NODE *(*iter_prev)(ACL_ITER*, ACL_JSON_NODE*);
};

enum {
	ACL_JSON_S_ROOT,	/**< ���ڵ� */
	ACL_JSON_S_OBJ,		/**< ��ǩ����ֵ */
	ACL_JSON_S_MEMBER,
	ACL_JSON_S_ARRAY,	/**< json �ڵ� array */
	ACL_JSON_S_ELEMENT,
	ACL_JSON_S_PAIR,	/**< name:value pair */
	ACL_JSON_S_NEXT,	/**< ��һ���ڵ� */
	ACL_JSON_S_TAG,		/**< �����ǩ�� */
	ACL_JSON_S_VALUE,	/**< �ڵ�ֵ������� */
	ACL_JSON_S_COLON,	/**< ð�� : */
	ACL_JSON_S_STRING,
	ACL_JSON_S_STREND
};

struct ACL_JSON {
	int   depth;                /**< ������ */
	int   node_cnt;             /**< �ڵ�����, ���� root �ڵ� */
	ACL_JSON_NODE *root;        /**< json ���ڵ� */
	int   finish;               /**< �Ƿ�������� */
	unsigned flag;              /**< ��־λ */
#define	ACL_JSON_FLAG_PART_WORD	(1 << 0)  /**< �Ƿ���ݰ������ */

	/* public: for acl_iterator, ͨ�� acl_foreach �����г������ӽڵ� */

	/* ȡ������ͷ���� */
	ACL_JSON_NODE *(*iter_head)(ACL_ITER*, ACL_JSON*);
	/* ȡ��������һ������ */
	ACL_JSON_NODE *(*iter_next)(ACL_ITER*, ACL_JSON*);
	/* ȡ������β���� */
	ACL_JSON_NODE *(*iter_tail)(ACL_ITER*, ACL_JSON*);
	/* ȡ��������һ������ */
	ACL_JSON_NODE *(*iter_prev)(ACL_ITER*, ACL_JSON*);

	/* private */

	int   status;               /**< ״̬����ǰ����״̬ */

	ACL_JSON_NODE *curr_node;   /**< ��ǰ���ڴ���� json �ڵ� */
	ACL_DBUF_POOL *dbuf;        /**< �Ự�ڴ�ض��� */
	ACL_DBUF_POOL *dbuf_inner;  /**< �Ự�ڴ�ض��� */
	size_t dbuf_keep;
};

/*----------------------------- in acl_json.c -----------------------------*/

/**
 * ����һ�� json �ڵ�
 * @param json {ACL_JSON*} json ����
 * @return {ACL_JSON_NODE*} json �ڵ����
 */
ACL_API ACL_JSON_NODE *acl_json_node_alloc(ACL_JSON *json);

/**
 * ��ĳ�� json �ڵ㼰���ӽڵ�� json ������ɾ��, ���ͷŸýڵ㼰���ӽڵ�
 * ��ռ�ռ亯�����ͷŸ� json �ڵ���ռ�ڴ�
 * @param node {ACL_JSON_NODE*} json �ڵ�
 * @return {int} ����ɾ���Ľڵ����
 */
ACL_API int acl_json_node_delete(ACL_JSON_NODE *node);

/**
 * ��ĳ�� json �ڵ�����ֵܽڵ�(���ֵܽڵ�����Ƕ����� json �ڵ�)
 * @param node1 {ACL_JSON_NODE*} �򱾽ڵ���� json �ڵ�
 * @param node2 {ACL_JSON_NODE*} ����ӵ��ֵ� json �ڵ�
 */
ACL_API void acl_json_node_append(ACL_JSON_NODE *node1, ACL_JSON_NODE *node2);

/**
 * ��ĳ�� json �ڵ���Ϊ�ӽڵ����ĳ�� json �ڵ���
 * @param parent {ACL_JSON_NODE*} ���ڵ�
 * @param child {ACL_JSON_NODE*} �ӽڵ�
 */
ACL_API void acl_json_node_add_child(
	ACL_JSON_NODE *parent, ACL_JSON_NODE *child);

/**
 * ��һ�� JSON ����� JSON �ڵ㸴���� JSON �����е�һ�� JSON �ڵ��У�������
 * Ŀ���´����� JSON �ڵ�
 * @param json {ACL_JSON*} Ŀ�� JSON ����
 * @param from {ACL_JSON_NODE*} Դ JSON �����һ�� JSON �ڵ�
 * @return {ACL_JSON_NODE*} ���طǿն���ָ��
 */
ACL_API ACL_JSON_NODE *acl_json_node_duplicate(
	ACL_JSON *json, ACL_JSON_NODE *from);

/**
 * ���ĳ�� json �ڵ�ĸ��ڵ�
 * @param node {ACL_JSON_NODE*} json �ڵ�
 * @return {ACL_JSON_NODE*} ���ڵ�, ���Ϊ NULL ���ʾ�丸�ڵ㲻����
 */
ACL_API ACL_JSON_NODE *acl_json_node_parent(ACL_JSON_NODE *node);

/**
 * ���ĳ�� json �ڵ�ĺ�һ���ֵܽڵ�
 * @param node {ACL_JSON_NODE*} json �ڵ�
 * @return {ACL_JSON_NODE*} ���� json �ڵ�ĺ�һ���ֵܽڵ�, ��ΪNULL���ʾ������
 */
ACL_API ACL_JSON_NODE *acl_json_node_next(ACL_JSON_NODE *node);

/**
 * ���ĳ�� json �ڵ��ǰһ���ֵܽڵ�
 * @param node {ACL_JSON_NODE*} json �ڵ�
 * @return {ACL_JSON_NODE*} ���� json �ڵ��ǰһ���ֵܽڵ�, ��ΪNULL���ʾ������
 */
ACL_API ACL_JSON_NODE *acl_json_node_prev(ACL_JSON_NODE *node);

/**
 * �����ǰ json �ڵ�������ַ���
 * @param node {ACL_JSON_NODE*} json �ڵ����
 * @return {const char*} json �ڵ�������͵������ַ���
 */
ACL_API const char *acl_json_node_type(const ACL_JSON_NODE *node);

/**
 * ����һ�� json ����
 * @return {ACL_JSON*} �´����� json ����
 */
ACL_API ACL_JSON *acl_json_alloc(void);

/**
 * ����һ�� json ����
 * @param dbuf {ACL_DBUF_POOL*} �ڴ�ض��󣬵�����Է� NULL ʱ���� json ����
 *  �������ڵ��ڴ���������Ͻ��з��䣬�����ڲ��Զ����������� json ���ڴ��
 * @return {ACL_JSON*} �´����� json ����
 */
ACL_API ACL_JSON *acl_json_dbuf_alloc(ACL_DBUF_POOL *dbuf);

/**
 * ����һ�� JSON �����һ�� JSON �ڵ㴴��һ���µ� JSON ����
 * @param node {ACL_JSON_NODE*} Դ JSON �����һ�� JSON �ڵ�
 * @return {ACL_JSON*} �´����� JSON ����
 */
ACL_API ACL_JSON *acl_json_create(ACL_JSON_NODE *node);

/**
 * ��ĳһ�� ACL_JSON_NODE �ڵ���Ϊһ�� json ����ĸ��ڵ㣬
 * �Ӷ����Է���ر������ýڵ�ĸ����ӽڵ�(�ڱ��������е�����
 * �ڵ㲻�����ڵ�����)���ñ�����ʽ�б��ڵ���
 * ����ĳһ�� ACL_JSON_NODE �ڵ�ʱ���ܱ�����һ���ӽڵ������
 * @param json {ACL_JSON*} json ����
 * @param node {ACL_JSON_NODE*} ACL_JSON_NODE �ڵ�
 */
ACL_API void acl_json_foreach_init(ACL_JSON *json, ACL_JSON_NODE *node);

/**
 * �ͷ�һ�� json ����, ͬʱ�ͷŸö��������ɵ����� json �ڵ�
 * @param json {ACL_JSON*} json ����
 */
ACL_API void acl_json_free(ACL_JSON *json);

/**
 * ���� json ����������
 * @param json {ACL_JSON*} json ����
 */
ACL_API void acl_json_reset(ACL_JSON *json);

/*------------------------- in acl_json_parse.c ---------------------------*/

/**
 * ���� json ����, ���������Զ����� json �ڵ���
 * @param json {ACL_JSON*} json ����
 * @param data {const char*} �� '\0' ��β�������ַ���, ������������ json ����;
 *  Ҳ�����ǲ������� json ����, ����ѭ�����ô˺���, �����������ݳ���������; �ò���
 *  ��Ϊ NULL����ֱ�ӷ��ؿմ���ַ����˽�ֹΪ NULL
 * @return {const char*} �����������󣬸÷���ֵ��ʾʣ�����ݵ�ָ���ַ
 */
ACL_API const char* acl_json_update(ACL_JSON *json, const char *data);

/**
 * �ж� JSON �����Ƿ����
 * @param json {ACL_JSON*} json ����
 * @return {int} ���ط� 0 ֵ��ʾ������ɣ������ʾδ���
 */
ACL_API int acl_json_finish(ACL_JSON *json);

/*------------------------- in acl_json_util.c ----------------------------*/

/**
 * �� json �����л�õ�һ����������ǩ����ͬ�� json �ڵ�
 * @param json {ACL_JSON*} json ����
 * @param tag {const char*} ��ǩ����
 * @return {ACL_JSON_NODE*} ���������� json �ڵ�, ������ NULL ��
 *  ��ʾû�з��������� json �ڵ�
 */
ACL_API ACL_JSON_NODE *acl_json_getFirstElementByTagName(
	ACL_JSON *json, const char *tag);

/**
 * �ͷ��� acl_json_getElementsByTagName, acl_json_getElementsByName,
 * �Ⱥ������صĶ�̬�������, ��Ϊ�ö�̬�����е�
 * Ԫ�ض��� ACL_JSON ������Ԫ�ص�����, �����ͷŵ��ö�̬�����, ֻҪ ACL_JSON
 * �����ͷ�, ��ԭ�����ڸ������е�Ԫ����Ȼ����ʹ��.
 * �������ͷ������ xml �ڵ�Ԫ��
 * @param a {ACL_ARRAY*} ��̬�������
 */
ACL_API void acl_json_free_array(ACL_ARRAY *a);

/**
 * �� json �����л�����е���������ǩ����ͬ�� json �ڵ�ļ���
 * @param json {ACL_JSON*} json ����
 * @param tag {const char*} ��ǩ����
 * @return {ACL_ARRAY*} ���������� json �ڵ㼯��, ���� ��̬������, ������ NULL ��
 *  ��ʾû�з��������� json �ڵ�, �ǿ�ֵ��Ҫ���� acl_json_free_array �ͷ�
 */
ACL_API ACL_ARRAY *acl_json_getElementsByTagName(
	ACL_JSON *json, const char *tag);

/**
 * �� json �����л�����е�������༶��ǩ����ͬ�� json �ڵ�ļ���
 * @param json {ACL_JSON*} json ����
 * @param tags {const char*} �༶��ǩ������ '/' �ָ�������ǩ��������� json ���ݣ�
 *  { 'root': [
 *      'first': { 'second': { 'third': 'test1' } },
 *      'first': { 'second': { 'third': 'test2' } },
 *      'first': { 'second': { 'third': 'test3' } }
 *    ]
 *  }
 *  ����ͨ���༶��ǩ����root/first/second/third һ���Բ�����з��������Ľڵ�
 * @return {ACL_ARRAY*} ���������� json �ڵ㼯��, ���� ��̬������, ������ NULL ��
 *  ��ʾû�з��������� json �ڵ�, �ǿ�ֵ��Ҫ���� acl_json_free_array �ͷ�
 */
ACL_API ACL_ARRAY *acl_json_getElementsByTags(
	ACL_JSON *json, const char *tags);

/**
 * ���� json ����ʱ���� json Ҷ�ڵ�
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param name {const char*} ��ǩ�����ǿ�
 * @param value {const char*} ��ǩֵ���ǿ�
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_text(ACL_JSON *json,
	const char *name, const char *value);
#define acl_json_create_leaf acl_json_create_text

/**
 * ���� json ����ʱ���� json �������͵�Ҷ�ڵ�
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param name {const char*} ��ǩ�����ǿ�
 * @param value {int} ��������ֵ
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_bool(ACL_JSON *json,
	const char *name, int value);

/**
 * ���� json ����ʱ���� json null ���͵�Ҷ�ڵ�
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param name {const char*} ��ǩ�����ǿ�
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_null(ACL_JSON *json, const char *name);

/**
 * ���� json ����ʱ���� json int ���͵�Ҷ�ڵ�
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param name {const char*} ��ǩ�����ǿ�
 * @param value {acl_int64} �з�������ֵ
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_int64(ACL_JSON *json,
	const char *name, acl_int64 value);

/**
 * ���� json ����ʱ���� json double ���͵�Ҷ�ڵ�
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param name {const char*} ��ǩ�����ǿ�
 * @param value {double} �з�������ֵ
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_double(ACL_JSON *json,
	const char *name, double value);

/**
 * ���� json ������ַ����ڵ㣬�� json �淶���ýڵ�ֻ�ܼ��������������
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param text {const char*}
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_text(ACL_JSON *json,
	const char *text);

/**
 * ���� json �������ֵ�ڵ㣬�� json �淶���ýڵ�ֻ�ܼ��������������
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param text {acl_int64}
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_int64(ACL_JSON *json,
	acl_int64 value);
/**
 * ���� json �������ֵ�ڵ㣬�� json �淶���ýڵ�ֻ�ܼ��������������
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param text {double}
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_double(ACL_JSON *json,
	double value);

/**
 * ���� json ����Ĳ����ڵ㣬�� json �淶���ýڵ�ֻ�ܼ��������������
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param value {int} �� 0 ��ʾ true�������ʾ false
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_bool(ACL_JSON *json, int value);

/**
 * ���� json ����� null �ڵ㣬�� json �淶���ýڵ�ֻ�ܼ��������������
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_null(ACL_JSON *json);

/**
 * ���� json ����ʱ���� json ����(�������� {} �Ķ���)
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_obj(ACL_JSON *json);

/**
 * ���� json ����ʱ���� json �������(�������� [] �Ķ���)
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_array(ACL_JSON *json);

/**
 * ���� json ����ʱ���� json �ڵ����(�� tagname: ACL_JSON_NODE)
 * @param json {ACL_JSON*} �� acl_json_alloc / acl_json_alloc1 ����
 * @param name {const char*} json �ڵ�ı�ǩ��
 * @param value {ACL_JSON_NODE*} json �ڵ������Ϊ��ǩֵ
 * @return {ACL_JSON_NODE*} �´����Ľڵ�������ͷ� ACL_JSON ����ʱ
 *  һ���ͷţ����Բ���Ҫ�����ͷ�
 */
ACL_API ACL_JSON_NODE *acl_json_create_node(ACL_JSON *json,
	const char *name, ACL_JSON_NODE *value);

/**
 * ���� json ����ʱ����һ���� acl_json_create_obj �� acl_json_create_array
 * ������ json �ڵ�����ӽڵ㣬���ӽڵ�����������½ӿڴ����Ľڵ�:
 * acl_json_create_leaf, acl_json_create_obj, acl_json_create_array
 */
ACL_API void acl_json_node_append_child(ACL_JSON_NODE *parent,
	ACL_JSON_NODE *child);

/**
 * �� json �����һ�� JSON �ڵ�ת���ַ�������
 * @param node {ACL_JSON_NODE*} json �ڵ����
 * @param buf {ACL_VSTRING*} �洢������Ļ����������ò���Ϊ��ʱ�����ڲ���
 *  �Զ�����һ�λ�������Ӧ���������Ҫ�ͷŵ����ǿպ����ڲ���ֱ�ӽ�����洢����
 * @return {ACL_VSTRING*} json �ڵ����ת�����ַ�����Ĵ洢��������
 *  �÷���ֵ��Զ�ǿգ�ʹ���߿���ͨ�� ACL_VSTRING_LEN(x) �����ж������Ƿ�Ϊ�գ�
 *  ���ص� ACL_VSTRING ָ�����Ϊ�ú����ڲ������ģ����û���������
 *  acl_vstring_free �����ͷ�
 */
ACL_API ACL_VSTRING *acl_json_node_build(ACL_JSON_NODE *json, ACL_VSTRING *buf);

/**
 * �� json ����ת���ַ�������
 * @param json {ACL_JSON*} json ����
 * @param buf {ACL_VSTRING*} �洢������Ļ����������ò���Ϊ��ʱ�����ڲ���
 *  �Զ�����һ�λ�������Ӧ���������Ҫ�ͷŵ����ǿպ����ڲ���ֱ�ӽ�����洢����
 * @return {ACL_VSTRING*} json ����ת�����ַ�����Ĵ洢���������÷���ֵ��Զ�ǿգ�
 *  ʹ���߿���ͨ�� ACL_VSTRING_LEN(x) �����ж������Ƿ�Ϊ�գ����ص� ACL_VSTRING
 *  ָ�����Ϊ�ú����ڲ������ģ����û��������� acl_vstring_free �����ͷ�
 */
ACL_API ACL_VSTRING *acl_json_build(ACL_JSON *json, ACL_VSTRING *buf);

/**
 * ��ʽ JSON ����ת�ַ���������̣����ú����ڽ� JSON ����תΪ�ַ����Ĺ����У�
 * һ��ת��һ�߽�����ͨ���ص���������������ߣ������߿����޶������޶����û�
 * ��������ʱ�����ô��������Ӧ�ڵ�JSON����ת�ɵ��ַ����ǳ���ʱ(�糬��100 MB),
 * ��Ϊ������ʽת����ʽ�����Բ�����Ҫ����һ�����ڴ�
 * @param json {ACL_JSON*} json ����
 * @param length {size_t} ��ת��Ϊ�ַ����Ĺ�����������������ȳ����ó�����ص�
 *  �û��趨�Ļص�����
 * @param callback {int (*)(ACL_JSON*, ACL_VSTRING*, void*)} �û��趨�Ļص�
 *  ���������ص��������ĵڶ�������Ϊ NULL ʱ��ʾ������ϣ�����û��ڸûص�
 *  ��ĳ�α����ú󷵻�ֵ < 0 ��ֹͣ�������
 * @param ctx {void*} callback ���������һ������
 */
ACL_API void acl_json_building(ACL_JSON *json, size_t length,
	int (*callback)(ACL_JSON *, ACL_VSTRING *, void *), void *ctx);

#ifdef __cplusplus
}
#endif

#endif
