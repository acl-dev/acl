#ifndef ACL_JSON_INCLUDE_H
#define ACL_JSON_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_iterator.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_array.h"

typedef struct ACL_JSON ACL_JSON;
typedef struct ACL_JSON_NODE ACL_JSON_NODE;

struct ACL_JSON_NODE {
	ACL_VSTRING *ltag;          /**< 标签名 */
	ACL_VSTRING *text;          /**< 当结点为叶结点时该文本内容非空 */
	ACL_JSON_NODE *tag_node;    /**< 当标签值为 json 结点时此项非空 */
	int   type;                 /**< 结点类型 */
#define	ACL_JSON_T_A_STRING      (1 << 0)
#define	ACL_JSON_T_A_NUMBER      (1 << 1)
#define	ACL_JSON_T_A_BOOL        (1 << 2)
#define	ACL_JSON_T_NUMBER        (1 << 3)
#define ACL_JSON_T_OBJ           (1 << 4)
#define ACL_JSON_T_ARRAY         (1 << 5)
#define	ACL_JSON_T_BOOL          (1 << 6)
#define	ACL_JSON_T_NULL          (1 << 7)

#define ACL_JSON_T_TEXT          (1 << 8)
#define ACL_JSON_T_LEAF          ACL_JSON_T_TEXT
#define ACL_JSON_T_MEMBER        (1 << 9)
#define ACL_JSON_T_PAIR          (1 << 10)
#define	ACL_JSON_T_ELEMENT       (1 << 11)

	ACL_JSON_NODE *parent;      /**< 父结点 */
	ACL_RING children;          /**< 子结点集合 */
	int  depth;                 /**< 当前结点的深度 */

	/* private */
	ACL_JSON *json;             /**< json 对象 */
	ACL_RING node;              /**< 当前结点 */
	int   quote;                /**< 非 0 表示 ' 或 " */
	int   left_ch;              /**< 本结点的第一个字符: { or [ */
	int   right_ch;             /**< 本结点的最后一个字符: } or ] */
	int   backslash;            /**< 转义字符 \ */
	int   part_word;            /**< 半个汉字的情况处理标志位 */

	/* public: for acl_iterator, 通过 acl_foreach 列出该结点的一级子结点 */

	/* 取迭代器头函数 */
	ACL_JSON_NODE *(*iter_head)(ACL_ITER*, ACL_JSON_NODE*);
	/* 取迭代器下一个函数 */
	ACL_JSON_NODE *(*iter_next)(ACL_ITER*, ACL_JSON_NODE*);
	/* 取迭代器尾函数 */
	ACL_JSON_NODE *(*iter_tail)(ACL_ITER*, ACL_JSON_NODE*);
	/* 取迭代器上一个函数 */
	ACL_JSON_NODE *(*iter_prev)(ACL_ITER*, ACL_JSON_NODE*);
};

enum {
	ACL_JSON_S_ROOT,	/**< 根结点 */
	ACL_JSON_S_OBJ,		/**< 标签对象值 */
	ACL_JSON_S_MEMBER,
	ACL_JSON_S_ARRAY,	/**< json 结点 array */
	ACL_JSON_S_ELEMENT,
	ACL_JSON_S_PAIR,	/**< name:value pair */
	ACL_JSON_S_NEXT,	/**< 下一个结点 */
	ACL_JSON_S_TAG,		/**< 对象标签名 */
	ACL_JSON_S_VALUE,	/**< 结点值处理过程 */
	ACL_JSON_S_COLON,	/**< 冒号 : */
	ACL_JSON_S_STRING,
	ACL_JSON_S_STREND
};

struct ACL_JSON {
	int   depth;                /**< 最大深度 */
	int   node_cnt;             /**< 结点总数, 包括 root 结点 */
	ACL_JSON_NODE *root;        /**< json 根结点 */
	int   finish;               /**< 是否分析结束 */
	unsigned flag;              /**< 标志位 */
#define	ACL_JSON_FLAG_PART_WORD	(1 << 0)  /**< 是否兼容半个汉字 */

	/* public: for acl_iterator, 通过 acl_foreach 可以列出所有子结点 */

	/* 取迭代器头函数 */
	ACL_JSON_NODE *(*iter_head)(ACL_ITER*, ACL_JSON*);
	/* 取迭代器下一个函数 */
	ACL_JSON_NODE *(*iter_next)(ACL_ITER*, ACL_JSON*);
	/* 取迭代器尾函数 */
	ACL_JSON_NODE *(*iter_tail)(ACL_ITER*, ACL_JSON*);
	/* 取迭代器上一个函数 */
	ACL_JSON_NODE *(*iter_prev)(ACL_ITER*, ACL_JSON*);

	/* private */

	int   status;                   /**< 状态机当前解析状态 */

	ACL_ARRAY *node_cache;      /**< json 结点缓存池 */
	int   max_cache;            /**< json 结点缓存池的最大容量 */
	ACL_JSON_NODE *curr_node;   /**< 当前正在处理的 json 结点 */
	ACL_SLICE_POOL *slice;      /**< 内存池对象 */
};

/*----------------------------- in acl_json.c -----------------------------*/

/**
 * 创建一个 json 结点
 * @param json {ACL_JSON*} json 对象
 * @return {ACL_JSON_NODE*} json 结点对象
 */
ACL_API ACL_JSON_NODE *acl_json_node_alloc(ACL_JSON *json);

/**
 * 将某个 json 结点及其子结点从 json 对象中删除, 并释放该结点及其子结点
 * 所占空间函数来释放该 json 结点所占内存
 * @param node {ACL_JSON_NODE*} json 结点
 * @return {int} 返回删除的结点个数
 */
ACL_API int acl_json_node_delete(ACL_JSON_NODE *node);

/**
 * 向某个 json 结点添加兄弟结点(该兄弟结点必须是独立的 json 结点)
 * @param node1 {ACL_JSON_NODE*} 向本结点添加 json 结点
 * @param node2 {ACL_JSON_NODE*} 新添加的兄弟 json 结点
 */
ACL_API void acl_json_node_append(ACL_JSON_NODE *node1, ACL_JSON_NODE *node2);

/**
 * 将某个 json 结点作为子结点加入某父 json 结点中
 * @param parent {ACL_JSON_NODE*} 父结点
 * @param child {ACL_JSON_NODE*} 子结点
 */
ACL_API void acl_json_node_add_child(ACL_JSON_NODE *parent, ACL_JSON_NODE *child);

/**
 * 将一个 JSON 对象的 JSON 结点复制至 JSON 对象中的一个 JSON 结点中，并返回
 * 目标新创建的 JSON 结点
 * @param json {ACL_JSON*} 目标 JSON 对象
 * @param from {ACL_JSON_NODE*} 源 JSON 对象的一个 JSON 结点
 * @return {ACL_JSON_NODE*} 返回非空对象指针
 */
ACL_API ACL_JSON_NODE *acl_json_node_duplicate(ACL_JSON *json, ACL_JSON_NODE *from);

/**
 * 获得某个 json 结点的父结点
 * @param node {ACL_JSON_NODE*} json 结点
 * @return {ACL_JSON_NODE*} 父结点, 如果为 NULL 则表示其父结点不存在
 */
ACL_API ACL_JSON_NODE *acl_json_node_parent(ACL_JSON_NODE *node);

/**
 * 获得某个 json 结点的后一个兄弟结点
 * @param node {ACL_JSON_NODE*} json 结点
 * @return {ACL_JSON_NODE*} 给定 json 结点的后一个兄弟结点, 若为NULL则表示不存在
 */
ACL_API ACL_JSON_NODE *acl_json_node_next(ACL_JSON_NODE *node);

/**
 * 获得某个 json 结点的前一个兄弟结点
 * @param node {ACL_JSON_NODE*} json 结点
 * @return {ACL_JSON_NODE*} 给定 json 结点的前一个兄弟结点, 若为NULL则表示不存在
 */
ACL_API ACL_JSON_NODE *acl_json_node_prev(ACL_JSON_NODE *node);

/**
 * 输出当前 json 结点的类型字符串
 * @param node {ACL_JSON_NODE*} json 结点对象
 * @return {const char*} json 结点对象类型的描述字符串
 */
ACL_API const char *acl_json_node_type(const ACL_JSON_NODE *node);

/**
 * 创建一个 json 对象
 * @return {ACL_JSON*} 新创建的 json 对象
 */
ACL_API ACL_JSON *acl_json_alloc(void);

/**
 * 创建一个 json 对象，该 json 对象及所有的内部内存分配都在该内存池上进行分配
 * @param slice {ACL_SLICE_POOL*} 内存池对象，可以为空指针，表明不用内存池
 * @return {ACL_JSON*} 新创建的 json 对象
 */
ACL_API ACL_JSON *acl_json_alloc1(ACL_SLICE_POOL *slice);

/**
 * 根据一个 JSON 对象的一个 JSON 结点创建一个新的 JSON 对象
 * @param node {ACL_JSON_NODE*} 源 JSON 对象的一个 JSON 结点
 * @return {ACL_JSON*} 新创建的 JSON 对象
 */
ACL_API ACL_JSON *acl_json_create(ACL_JSON_NODE *node);

/**
 * 将某一个 ACL_JSON_NODE 结点作为一个 json 对象的根结点，
 * 从而可以方便地遍历出该结点的各级子结点(在遍历过程中的所有
 * 结点不含本结点自身)，该遍历方式有别于单独
 * 遍历某一个 ACL_JSON_NODE 结点时仅能遍历其一级子结点的情形
 * @param json {ACL_JSON*} json 对象
 * @param node {ACL_JSON_NODE*} ACL_JSON_NODE 结点
 */
ACL_API void acl_json_foreach_init(ACL_JSON *json, ACL_JSON_NODE *node);

/**
 * 打开或关闭 json 的缓存功能，当复用 ACL_JSON 对象时打开
 * json 的结点缓存功能有利提高效率
 * @param json {ACL_JSON*} json 对象
 * @param max_cache {int} 缓存的最大值，当该值 > 0 时会打开 json 解析器
 *  对 json 结点的缓存功能，否则会关闭 json 解析器对 json 结点的缓存功能
 */
ACL_API void acl_json_cache(ACL_JSON *json, int max_cache);

/**
 * 释放 JSON 中缓存的 JSON 结点对象
 * @param json {ACL_JSON*} json 对象
 */
ACL_API void acl_json_cache_free(ACL_JSON *json);

/**
 * 释放一个 json 对象, 同时释放该对象里容纳的所有 json 结点
 * @param json {ACL_JSON*} json 对象
 */
ACL_API int acl_json_free(ACL_JSON *json);

/**
 * 重置 json 解析器对象
 * @param json {ACL_JSON*} json 对象
 */
ACL_API void acl_json_reset(ACL_JSON *json);

/*------------------------- in acl_json_parse.c ---------------------------*/

/**
 * 解析 json 数据, 并持续地自动生成 json 结点树
 * @param json {ACL_JSON*} json 对象
 * @param data {const char*} 以 '\0' 结尾的数据字符串, 可以是完整的 json 数据;
 *  也可以是不完整的 json 数据, 允许循环调用此函数, 将不完整数据持续地输入
 */
ACL_API void acl_json_update(ACL_JSON *json, const char *data);

/*------------------------- in acl_json_util.c ----------------------------*/

/**
 * 释放由 acl_json_getElementsByTagName, acl_json_getElementsByName,
 * 等函数返回的动态数组对象, 因为该动态数组中的
 * 元素都是 ACL_JSON 对象中元素的引用, 所以释放掉该动态数组后, 只要 ACL_JSON
 * 对象不释放, 则原来存于该数组中的元素依然可以使用.
 * 但并不释放里面的 xml 结点元素
 * @param a {ACL_ARRAY*} 动态数组对象
 */
ACL_API void acl_json_free_array(ACL_ARRAY *a);

/**
 * 从 json 对象中获得所有的与所给标签名相同的 json 结点的集合
 * @param json {ACL_JSON*} json 对象
 * @param tag {const char*} 标签名称
 * @return {ACL_ARRAY*} 符合条件的 json 结点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 json 结点, 非空值需要调用 acl_json_free_array 释放
 */
ACL_API ACL_ARRAY *acl_json_getElementsByTagName(ACL_JSON *json, const char *tag);

/**
 * 从 json 对象中获得所有的与给定多级标签名相同的 json 结点的集合
 * @param json {ACL_JSON*} json 对象
 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 json 数据：
 *  { 'root': [
 *      'first': { 'second': { 'third': 'test1' } },
 *      'first': { 'second': { 'third': 'test2' } },
 *      'first': { 'second': { 'third': 'test3' } }
 *    ]
 *  }
 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的结点
 * @return {ACL_ARRAY*} 符合条件的 json 结点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 json 结点, 非空值需要调用 acl_json_free_array 释放
 */
ACL_API ACL_ARRAY *acl_json_getElementsByTags(ACL_JSON *json, const char *tags);

/**
 * 构建 json 对象时创建 json 叶结点
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @param name {const char*} 标签名，非空
 * @param value {const char*} 标签值，非空
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_text(ACL_JSON *json,
	const char *name, const char *value);
#define acl_json_create_leaf acl_json_create_text

/**
 * 构建 json 对象时创建 json 布尔类型的叶结点
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @param name {const char*} 标签名，非空
 * @param value {int} 布尔类型值
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_bool(ACL_JSON *json,
	const char *name, int value);

/**
 * 构建 json 对象时创建 json int 类型的叶结点
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @param name {const char*} 标签名，非空
 * @param value {acl_int64} 有符号整形值
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_int64(ACL_JSON *json,
	const char *name, acl_int64 value);

/**
 * 构建 json 对象的字符串结点，按 json 规范，该结点只能加入至数组对象中
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @param text {const char*}
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_text(ACL_JSON *json,
	const char *text);
ACL_API ACL_JSON_NODE *acl_json_create_string(ACL_JSON *json,
	const char *text);

/**
 * 构建 json 对象的数值结点，按 json 规范，该结点只能加入至数组对象中
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @param text {const char*}
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_int64(ACL_JSON *json,
	acl_int64 value);

/**
 * 构建 json 对象的布尔结点，按 json 规范，该结点只能加入至数组对象中
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @param value {int}
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_array_bool(ACL_JSON *json, int value);

/**
 * 构建 json 对象时创建 json 对象(即仅包含 {} 的对象)
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_obj(ACL_JSON *json);

/**
 * 构建 json 对象时创建 json 数组对象(即仅包含 [] 的对象)
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_array(ACL_JSON *json);

/**
 * 构建 json 对象时创建 json 结点对象(即 tagname: ACL_JSON_NODE)
 * @param json {ACL_JSON*} 由 acl_json_alloc / acl_json_alloc1 创建
 * @param name {const char*} json 结点的标签名
 * @param value {ACL_JSON_NODE*} json 结点对象作为标签值
 * @return {ACL_JSON_NODE*} 新创建的结点对象，在释放 ACL_JSON 对象时
 *  一起被释放，所以不需要单独释放
 */
ACL_API ACL_JSON_NODE *acl_json_create_node(ACL_JSON *json,
	const char *name, ACL_JSON_NODE *value);

/**
 * 构建 json 对象时，向一个由 acl_json_create_obj 或 acl_json_create_array
 * 创建的 json 结点添加子结点，该子结点可以是由如下接口创建的结点:
 * acl_json_create_leaf, acl_json_create_obj, acl_json_create_array
 */
ACL_API void acl_json_node_append_child(ACL_JSON_NODE *parent,
	ACL_JSON_NODE *child);

/**
 * 将 json 对象的一个 JSON 结点转成字符串内容
 * @param node {ACL_JSON_NODE*} json 结点对象
 * @param buf {ACL_VSTRING*} 存储结果集的缓冲区，当该参数为空时则函数内部会
 *  自动分配一段缓冲区，应用用完后需要释放掉；非空函数内部会直接将结果存储其中
 * @return {ACL_VSTRING*} json 结点对象转换成字符串后的存储缓冲区，
 *  该返回值永远非空，使用者可以通过 ACL_VSTRING_LEN(x) 宏来判断内容是否为空，
 *  返回的 ACL_VSTRING 指针如果为该函数内部创建的，则用户名必须用
 *  acl_vstring_free 进行释放
 */
ACL_API ACL_VSTRING *acl_json_node_build(ACL_JSON_NODE *json, ACL_VSTRING *buf);

/**
 * 将 json 对象转成字符串内容
 * @param json {ACL_JSON*} json 对象
 * @param buf {ACL_VSTRING*} 存储结果集的缓冲区，当该参数为空时则函数内部会
 *  自动分配一段缓冲区，应用用完后需要释放掉；非空函数内部会直接将结果存储其中
 * @return {ACL_VSTRING*} json 对象转换成字符串后的存储缓冲区，该返回值永远非空，
 *  使用者可以通过 ACL_VSTRING_LEN(x) 宏来判断内容是否为空，返回的 ACL_VSTRING
 *  指针如果为该函数内部创建的，则用户名必须用 acl_vstring_free 进行释放
 */
ACL_API ACL_VSTRING *acl_json_build(ACL_JSON *json, ACL_VSTRING *buf);

/**
 * 流式 JSON 对象转字符串处理过程，即该函数在将 JSON 对象转为字符串的过程中，
 * 一边转换一边将数据通过回调函数输出给调用者，调用者可以限定长度限定调用回
 * 调函数的时机；该处理过程适应于当JSON对象转成的字符串非常长时(如超过100 MB),
 * 因为采用流式转换方式，所以并不需要分配一个大内存
 * @param json {ACL_JSON*} json 对象
 * @param length {size_t} 在转换为字符串的过程中如果缓冲区长度超过该长度则回调
 *  用户设定的回调函数
 * @param callback {int (*)(ACL_JSON*, ACL_VSTRING*, void*)} 用户设定的回调
 *  函数，当回调函数给的第二个参数为 NULL 时表示处理完毕；如果用户在该回调
 *  的某次被调用后返回值 < 0 则停止处理过程
 * @param ctx {void*} callback 函数的最后一个参数
 */
ACL_API void acl_json_building(ACL_JSON *json, size_t length,
	int (*callback)(ACL_JSON *, ACL_VSTRING *, void *), void *ctx);

#ifdef __cplusplus
}
#endif

#endif
