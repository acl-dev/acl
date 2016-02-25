#ifndef ACL_XML_INCLUDE_H
#define ACL_XML_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_array.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_htable.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_dbuf_pool.h"

typedef struct ACL_XML	ACL_XML;
typedef struct ACL_XML_NODE	ACL_XML_NODE;
typedef struct ACL_XML_ATTR	ACL_XML_ATTR;

struct ACL_XML_ATTR {
	ACL_XML_NODE *node;             /**< 所属节点 */
	ACL_VSTRING *name;              /**< 属性名 */
	ACL_VSTRING *value;             /**< 属性值 */

	/* private */
	int   quote;                    /**< 非 0 表示 ' 或 " */
	int   backslash;                /**< 转义字符 \ */
	int   slash;                    /**< 是否有 '/' 标志位设定 */
};

struct ACL_XML_NODE {
	ACL_VSTRING *ltag;              /**< 左标签名 */
	ACL_VSTRING *rtag;              /**< 右标签名 */
	const ACL_VSTRING *id;          /**< ID标识符, 只有 xml->id_table
					 存在的节点的 id 才非空 */
	ACL_VSTRING *text;              /**< 文本显示内容 */
	ACL_ARRAY *attr_list;           /**< 属性(ACL_XML_ATTR)列表 */
	ACL_XML_NODE *parent;           /**< 父节点 */
	ACL_RING children;              /**< 子节点集合 */
	int  depth;                     /**< 当前节点的深度 */

	/* private */
	ACL_XML *xml;                   /**< xml 对象 */
	ACL_RING node;                  /**< 当前节点 */
	ACL_XML_ATTR *curr_attr;        /**< 当前正在解析的属性 */
	int   quote;                    /**< 非 0 表示 ' 或 " */
	int   last_ch;                  /**< 所记录本节点的前一个字节值 */
	int   nlt;                      /**< '<' 个数 */
	char  meta[3];                  /**< 元数据临时缓冲区 */
	unsigned int flag;
#define	ACL_XML_F_META_QM	(1 << 0)    /**< '?' flag */
#define	ACL_XML_F_META_CM	(1 << 1)    /**< '!--' flag */
#define	ACL_XML_F_META_EM	(1 << 2)    /**< only '!' flag */
#define ACL_XML_F_SELF_CL	(1 << 3)    /**< self closed flag */
#define	ACL_XML_F_LEAF		(1 << 4)    /**< leaf node has no child node */
#define	ACL_XML_F_CDATA		(1 << 5)    /**< CDATA data */

/**< 是否是元数据 */
#define	ACL_XML_F_META		\
	(ACL_XML_F_META_QM | ACL_XML_F_META_CM | ACL_XML_F_META_EM)

#define	ACL_XML_IS_COMMENT(x)	(((x)->flag & ACL_XML_F_META_CM))

#define	ACL_XML_IS_CDATA(x)	(((x)->flag & ACL_XML_F_CDATA))

	int   status;                   /**< 状态机当前解析状态 */
#define ACL_XML_S_NXT		0       /**< 下一个节点 */
#define ACL_XML_S_LLT		1       /**< 左边 '<' */
#define ACL_XML_S_LGT		2       /**< 右边 '>' */
#define	ACL_XML_S_LCH		3       /**< 左边 '<' 后第一个字节 */
#define ACL_XML_S_LEM		4       /**< 左边 '<' 后的 '!' */
#define ACL_XML_S_LTAG		5       /**< 左边的标签名 */
#define ACL_XML_S_RLT		6       /**< 右边的 '<' */
#define ACL_XML_S_RGT		7       /**< 右边的 '>' */
#define ACL_XML_S_RTAG		8       /**< 右边的标签名 */
#define ACL_XML_S_ATTR		9       /**< 标签属性名 */
#define ACL_XML_S_AVAL		10      /**< 标签属性值 */
#define ACL_XML_S_TXT		11      /**< 节点文本 */
#define ACL_XML_S_MTAG		12      /**< 元数据标签 */
#define ACL_XML_S_MTXT		13      /**< 元数据文本 */
#define ACL_XML_S_MCMT		14      /**< 元数据注释 */
#define ACL_XML_S_MEND		15      /**< 元数据结束 */
#define	ACL_XML_S_CDATA		16      /**< CDATA 数据 */

	/* public: for acl_iterator, 通过 acl_foreach 列出该节点的一级子节点 */

	/* 取迭代器头函数 */
	ACL_XML_NODE *(*iter_head)(ACL_ITER*, ACL_XML_NODE*);
	/* 取迭代器下一个函数 */
	ACL_XML_NODE *(*iter_next)(ACL_ITER*, ACL_XML_NODE*);
	/* 取迭代器尾函数 */
	ACL_XML_NODE *(*iter_tail)(ACL_ITER*, ACL_XML_NODE*);
	/* 取迭代器上一个函数 */
	ACL_XML_NODE *(*iter_prev)(ACL_ITER*, ACL_XML_NODE*);
};

struct ACL_XML {
	/* public */
	int   depth;                    /**< 最大深度 */
	int   node_cnt;                 /**< 节点总数, 包括 root 节点 */
	int   root_cnt;                 /**< 根节点个数 */
	int   attr_cnt;                 /**< 属性总数 */
	ACL_XML_NODE *root;             /**< XML 根节点 */

	/* private */
	ACL_HTABLE *id_table;           /**< id 标识符哈希表 */
	ACL_XML_NODE *curr_node;        /**< 当前正在处理的 XML 节点 */
	ACL_DBUF_POOL *dbuf;            /**< 内存池对象 */
	ACL_DBUF_POOL *dbuf_inner;      /**< 内部分布的内存池对象 */
	size_t dbuf_keep;               /**< 内存池中保留的长度 */
	size_t space;                   /**< 在创建 xml 对象时已分配的内存大小 */

	unsigned flag;                  /**< 标志位: ACL_XML_FLAG_xxx */ 

	/**< 是否允许一个 xml 文档中有多个根节点，内部缺省为允许 */
#define	ACL_XML_FLAG_MULTI_ROOT	(1 << 0)

	/**< 是否兼容单节点中没有 '/' 情况 */
#define	ACL_XML_FLAG_IGNORE_SLASH	(1 << 1)

	/**< 是否需要对文本数据进行 xml 解码  */
#define	ACL_XML_FLAG_XML_DECODE		(1 << 2)

	/**< 创建 xml 对象时是否需要对数据进行 xml 编码 */
#define ACL_XML_FLAG_XML_ENCODE		(1 << 3)

	ACL_VSTRING *decode_buf;        /**< 当需要进行 xml 解码时非空 */

	/* public: for acl_iterator, 通过 acl_foreach 可以列出所有子节点 */

	/* 取迭代器头函数 */
	ACL_XML_NODE *(*iter_head)(ACL_ITER*, ACL_XML*);
	/* 取迭代器下一个函数 */
	ACL_XML_NODE *(*iter_next)(ACL_ITER*, ACL_XML*);
	/* 取迭代器尾函数 */
	ACL_XML_NODE *(*iter_tail)(ACL_ITER*, ACL_XML*);
	/* 取迭代器上一个函数 */
	ACL_XML_NODE *(*iter_prev)(ACL_ITER*, ACL_XML*);
};

/***************************************************************************/
/*                  公共函数接口，用户可以放心使用该接口集                 */
/***************************************************************************/

/*----------------------------- in acl_xml.c ------------------------------*/

/**
 * 判断 xml 对象是否闭合的, 即是否所解析的数据是否完整, 如果该 xml 对象里的
 * xml 节点元素为空, 则也认为不是闭合的
 * @param xml {ACL_XML*} xml 对象
 * @return {int} 0: 否; 1: 是
 */
ACL_API int acl_xml_is_closure(ACL_XML *xml);

/**
 * 根据指定标签名判断 xml 解析已经完成, 当该标签与 xml 对象中 root 一级子节点
 * 中的最后一个 xml 节点的标签相同时, 则认为 xml 解析完成, 为保证判断的正确性,
 * 数据源应保证最外层的根节点只有一个, 即 xml->root 的一级子节点只有一个, 否则
 * 会造成误判
 * @param xml {ACL_XML*} xml 对象
 * @param tag {const char*} 用户给定标签名, 内部在匹配时不区分大小写
 * @return {int} 0: 否; 1: 是
 */
ACL_API int acl_xml_is_complete(ACL_XML *xml, const char *tag);

/**
 * 创建一个 xml 对象
 * @return {ACL_XML*} 新创建的 xml 对象
 */
ACL_API ACL_XML *acl_xml_alloc(void);

/**
 * 创建一个 xml 对象，该 xml 对象及所有的内部内存分配都在该内存池上进行分配
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，当该针对非 NULL 时，则 xml 对象
 *  及所属节点内存在其基础上进行分配，否则，内部自动创建隶属于 xml 的内存池
 * @return {ACL_XML*} 新创建的 xml 对象
 */
ACL_API ACL_XML *acl_xml_dbuf_alloc(ACL_DBUF_POOL *dbuf);

/**
 * 获得当前 xml 对象内部已经分配的内存空间大小
 * @param xml {ACL_XML*}
 * @return {size_t} 当前 xml 对象内部已分配的内存大小
 */
ACL_API size_t acl_xml_space(ACL_XML *xml);

/**
 * 将 xml 对象内部记录内存大小的变量清 0
 * @param xml {ACL_XML*}
 */
ACL_API void acl_xml_space_clear(ACL_XML *xml);

/**
 * 将某一个 ACL_XML_NODE 节点作为一个 XML 对象的根节点，从而可以方便地遍历出该
 * 节点各级子节点(在遍历过程中的所有节点不含本节点自身)，该遍历方式有别于单独
 * 遍历某一个 ACL_XML_NODE 节点时仅能遍历其一级子节点的情形
 * @param xml {ACL_XML*} xml 对象
 * @param node {ACL_XML_NODE*} AXL_XML_NODE 节点
 */
ACL_API void acl_xml_foreach_init(ACL_XML *xml, ACL_XML_NODE *node);

/**
 * 设置一个 xml 文档中是否允许有多个根 xml 节点，内部缺省支持多个根节点
 * @param xml {ACL_XML*} xml 对象
 * @param on {int} 非 0 则允许，为 0 表示不允许，当禁止有多个根 xml 节点时，
 *  则在解析时当遇到第一个根节点结束时便返回剩余的数据
 */
ACL_API void acl_xml_multi_root(ACL_XML *xml, int on);

/**
 * 对于 XML 单节点的情况, 是否允许可以没有 /, 如:
 * <test id=111>, <test id=111 />, 当可以允许没有 / 则这两种写法
 * 都是合法的，否则只有第二个写法是合法的，如果允许这种兼容性，则
 * 会造成一定的性能损失
 * @param xml {ACL_XML*} xml 对象
 * @param ignore {int} 如果非 0 表示单节点必须有 /
 */
ACL_API void acl_xml_slash(ACL_XML *xml, int ignore);

/**
 * 解析 xml 对象时是否对属性值及文本值进行 xml 解码，内部缺省解码
 * @param xml {ACL_XML*}
 * @param on {int} 非 0 表示进行 xml 解码
 */
ACL_API void acl_xml_decode_enable(ACL_XML *xml, int on);

/**
 * 创建 xml 对象时是否对属性值及文本值进行 xml 编码，内部缺省编码
 */
ACL_API void acl_xml_encode_enable(ACL_XML *xml, int on);

/**
 * 释放一个 xml 对象, 同时释放该对象里容纳的所有 xml 节点
 * @param xml {ACL_XML*} xml 对象
 * @return {int} 返回释放的 xml 节点个数
 */
ACL_API int acl_xml_free(ACL_XML *xml);

/**
 * 重置 XML 解析器对象
 * @param xml {ACL_XML*} xml 对象
 */
ACL_API void acl_xml_reset(ACL_XML *xml);

/*------------------------- in acl_xml_parse.c ----------------------------*/

/**
 * 解析 xml 数据, 并持续地自动生成 xml 节点树
 * @param xml {ACL_XML*} xml 对象
 * @param data {const char*} 以 '\0' 结尾的数据字符串, 可以是完整的 xml 数据;
 *  也可以是不完整的 xml 数据, 允许循环调用此函数, 将不完整数据持续地输入
 * @return {const char*} 当通过 acl_xml_multi_root 允许一个 xml 文档中在在
 *  多个根 xml 节点时，该函数返回的地址的字节为 '\0'; 否则返回剩余的数据地址
 *  包含非空字符串
 *  注：也可以通过 acl_xml_is_complete 判断是否解析完毕
 */
ACL_API const char *acl_xml_update(ACL_XML *xml, const char *data);
#define	acl_xml_parse	acl_xml_update

/*------------------------- in acl_xml_util.c -----------------------------*/

/**
 * 初始化类似于 input, br, hr 等的自闭合标签, 形成自闭合标签树, 以便于
 * acl_xml_tag_selfclosed 查询该树, 检查所给标签是否是保留的自闭合标签,
 * 该函数只能被初始化一次, 也可以不初始化
 */
ACL_API void acl_xml_tag_init(void);

/**
 * 允许用户自己添加一些非自闭合的标签
 * @param tag {const char*} 标签名，注意标签长度不得大于 254 个字节
 */
ACL_API void acl_xml_tag_add(const char *tag);

/**
 * 当调用 acl_xml_tag_init 初始化保留的自闭合标签树后, 可以调用此函数判断所给
 * 标签是否属于自闭合标签, 如果未调用 acl_xml_tag_init, 则该函数永远返回 0
 * @parma tag {const char*} 标签名称
 * @return {int} 0: 表示否, 1: 表示是
 */
ACL_API int  acl_xml_tag_selfclosed(const char *tag);

/**
 * 判断标签所属 xml 节点是否是叶节点, 叶节点没有子节点
 * @param tag {const char*} 标签名
 * @return {int} 0: 不是叶节点; 1: 是叶节点
 */
ACL_API int  acl_xml_tag_leaf(const char *tag);

/**
 * 释放由 acl_xml_getElementsByTagName, acl_xml_getElementsByName,
 * acl_xml_getElementsByAttr 等函数返回的动态数组对象, 因为该动态数组中的
 * 元素都是 ACL_XML 对象中元素的引用, 所以释放掉该动态数组后, 只要 ACL_XML
 * 对象不释放, 则原来存于该数组中的元素依然可以使用.
 * 但并不释放里面的 xml 节点元素
 * @param a {ACL_ARRAY*} 动态数组对象
 */
ACL_API void acl_xml_free_array(ACL_ARRAY *a);

/**
 * 从 xml 对象中获得与所给标签名相同的 xml 第一个节点
 * @param xml {ACL_XML*} xml 对象
 * @param tag {const char*} 标签名称
 * @return {ACL_XML_NODE*} 符合条件的 xml 节点, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点
 */
ACL_API ACL_XML_NODE *acl_xml_getFirstElementByTagName(
	ACL_XML *xml, const char *tag);

/**
 * 从 xml 对象中获得所有的与所给标签名相同的 xml 节点的集合
 * @param xml {ACL_XML*} xml 对象
 * @param tag {const char*} 标签名称
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByTagName(
	ACL_XML *xml, const char *tag);

/**
 * 从 xml 对象中获得所有的与给定多级标签名相同的 xml 节点的集合
 * @param xml {ACL_XML*} xml 对象
 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first>
 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first>
 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first>
 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的节点
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByTags(ACL_XML *xml, const char *tags);

/**
 * 从 xml 对象中获得所有的与给定属性名 name 的属性值相同的 xml 节点元素集合
 * @param xml {ACL_XML*} xml 对象
 * @param value {const char*} 属性名为 name 的属性值
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByName(ACL_XML *xml, const char *value);

/**
 * 从 xml 对象中获得所有给定属性名及属性值的 xml 节点元素集合
 * @param xml {ACL_XML*} xml 对象
 * @param name {const char*} 属性名
 * @param value {const char*} 属性值
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml_getElementsByAttr(ACL_XML *xml,
	const char *name, const char *value);

/**
 * 从 xml 对象中获得指定 id 值的 xml 节点元素的某个属性对象
 * @param xml {ACL_XML*} xml 对象
 * @param id {const char*} id 值
 * @return {ACL_XML_ATTR*} 某 xml 节点的某个属性对象, 若返回 NULL 则表示
 *  没有符合条件的属性, 返回值不需要释放
 */
ACL_API ACL_XML_ATTR *acl_xml_getAttrById(ACL_XML *xml, const char *id);

/**
 * 从 xml 对象中获得指定 id 值的 xml 节点元素的某个属性值
 * @param xml {ACL_XML*} xml 对象
 * @param id {const char*} id 值
 * @return {const char*} 某 xml 节点的某个属性值, 若返回 NULL 则表示没有符合
 *  条件的属性
 */
ACL_API const char *acl_xml_getAttrValueById(ACL_XML *xml, const char *id);

/**
 * 从 xml 对象中获得指定 id 值的 xml 节点元素
 * @param xml {ACL_XML*} xml 对象
 * @param id {const char*} id 值
 * @return {ACL_XML_NODE*} xml 节点元素, 若返回 NULL 则表示没有符合
 *  条件的 xml 节点, 返回值不需要释放
 */
ACL_API ACL_XML_NODE *acl_xml_getElementById(ACL_XML *xml, const char *id);

/**
 * 从 xml 对象中提取有在 ? ! 等开头的节点
 * @param xml {ACL_XML*} xml 对象
 * @param tag {const char*} 标签名
 * @return {ACL_XML_NODE*} xml 节点元素, 若返回 NULL 则表示没有符合
 *  条件的 xml 节点, 返回值不需要释放
 */
ACL_API ACL_XML_NODE *acl_xml_getElementMeta(ACL_XML *xml, const char *tag);

/**
 * 获得 xml 的字符集编码格式
 * @param xml {ACL_XML*} xml 对象
 * @return {const char*} 返回字符集编码格式，返回 NULL 时表示没有该属性
 */
ACL_API const char *acl_xml_getEncoding(ACL_XML *xml);

/**
 * 获得 xml 数据的类型，如：text/xsl
 * @param xml {ACL_XML*} xml 对象
 * @return {const char*} 返回 NULL 表示没有该属性
 */
ACL_API const char *acl_xml_getType(ACL_XML *xml);

/**
 * 从 xml 节点中获得指定属性名的属性对象
 * @param node {ACL_XML_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @return {ACL_XML_ATTR*} 属性对象, 为空表示不存在, 返回值不需要释放
 */
ACL_API ACL_XML_ATTR *acl_xml_getElementAttr(ACL_XML_NODE *node, const char *name);

/**
 * 从 xml 节点中获得指定属性名的属性值
 * @param node {ACL_XML_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @return {const char*} 属性值, 为空表示不存在
 */
ACL_API const char *acl_xml_getElementAttrVal(ACL_XML_NODE *node, const char *name);

/**
 * 从 xml 节点删除某个属性对象, 如果该属性为 id 属性, 则同时会从 xml->id_table 中删除
 * @param node {ACL_XML_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @return {int} 0 表示删除成功, -1: 表示删除失败(有可能是该属性不存在)
 */
ACL_API int acl_xml_removeElementAttr(ACL_XML_NODE *node, const char *name);

/**
 * 给 xml 节点添加属性, 如果该属性名已存在, 则用新的属性值替换其属性值, 否则
 * 创建并添加新的属性对象
 * @param node {ACL_XML_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @param value {const char*} 属性值
 * @return {ACL_XML_ATTR*} 返回该属性对象(有可能是原来的, 也有可能是新的), 
 *  返回值不需释放
 */
ACL_API ACL_XML_ATTR *acl_xml_addElementAttr(ACL_XML_NODE *node,
        const char *name, const char *value);

/**
 * 将标签名及节点文本做为参数创建 xml 节点，该函数主要用在构建 xml 对象时
 * @param xml {ACL_XML*} xml 对象，该对象应该是由 acl_xml_alloc 创建的
 * @param tagname {const char*} 标签名，必须非空且字符串长度大于 0
 * @param text {const char*} 节点的文本内容，可以为空
 * @return {ACL_XML_NODE*} 新创建的 xml 节点，该返回永远返回非空，如果输入
 *  参数非法则会导致内部自动产生断言
 */
ACL_API ACL_XML_NODE *acl_xml_create_node(ACL_XML *xml,
	const char* tagname, const char* text);

/**
 * 创建 xml 节点，使用文件流做为节点的文本内容项，同时会自动进行 XML 编码处理
 * @param xml {ACL_XML*} xml 对象
 * @param tag {const char*} 标签名，非 NULL 字符串
 * @param in {ACL_VSTREAM *} 输入流，非 NULL 时，其中内容将做为 xml 节点的文本内容
 * @param off {size_t} 当 in 为文件流时指定所拷贝内容在文件中的起始位置
 * @param len {size_t} 指定从输入流中拷贝的最大数据长度，当为 0 时则一直拷贝至流结束
 * @return {ACL_XML_NODE*} 返回新创建的 xml 节点，永远返回非 NULL 对象，
 *  如果输入参数非法则内部产生断言
 */
ACL_API ACL_XML_NODE *acl_xml_create_node_with_text_stream(ACL_XML *xml,
	const char *tag, ACL_VSTREAM *in, size_t off, size_t len);

/**
 * 给一个 xml 节点添加属性，该函数主要用在构建 xml 对象时
 * @param node {ACL_XML_NODE*} 由 acl_xml_create_node 创建的节点
 * @param name {const char*} 属性名，必须为非空字符串且字符串长度大于 0
 * @param value {const char*} 属性值，可以为空
 * @return {ACL_XML_ATTR*} xml 节点的属性对象，当输入参数非法时该函数
 *  内部自动产生断言
 */
ACL_API ACL_XML_ATTR *acl_xml_node_add_attr(ACL_XML_NODE *node,
	const char *name, const char *value);

/**
 * 给一个 xml 节点添加一组属性，该函数主要用在构建 xml 对象时
 * @param node {ACL_XML_NODE*} 由 acl_xml_create_node 创建的节点
 * @param ... 一组属性，遇到 NULL 时表示结束，如：
 *  {name1}, {value1}, {name2}, {value2}, ... NULL
 */
ACL_API void acl_xml_node_add_attrs(ACL_XML_NODE *node, ...);

/**
 * 给一个 xml 节点添加文本内容，该函数主要用在构建 xml 对象时，当该节点之前有文本内容时
 * 则用新文本覆盖原文本
 * @param node {ACL_XML_NODE*} 由 acl_xml_create_node 创建的节点
 * @param text {const char*} 文本内容
 */
ACL_API void acl_xml_node_set_text(ACL_XML_NODE *node, const char *text);

/**
* 给一个 xml 节点的文本追加内容，该函数主要用在构建 xml 对象时，在该节点的文本内容上
* 追加新的文本内容
* @param node {ACL_XML_NODE*} 由 acl_xml_create_node 创建的节点
* @param text {const char*} 文本内容 
 */
ACL_API void acl_xml_node_add_text(ACL_XML_NODE *node, const char *text);

/**
 * 用文件流中的内容给一个 xml 节点添加文本内容
 * @param node {ACL_XML_NODE*} 由 acl_xml_create_node 创建的节点 
 * @param in {ACL_VSTREAM*} 输入流对象
 * @param off {size_t} 当 in 为文件流，指定在文件中的起始位置
 * @param len {size_t} 要拷贝的最大数据长度，当为 0 时则一直拷贝至流结束
 */
ACL_API void acl_xml_node_set_text_stream(ACL_XML_NODE *node,
	ACL_VSTREAM *fp, size_t off, size_t len);

/**
 * 将 xml 对象转成字符串内容
 * @param xml {ACL_XML*} xml 对象
 * @param buf {ACL_VSTRING*} 存储结果集的缓冲区，当该参数为空时则函数内部会
 *  自动分配一段缓冲区，应用用完后需要释放掉；非空函数内部会直接将结果存储其中
 * @return {ACL_VSTRING*} xml 对象转换成字符串后的存储缓冲区，该返回值永远非空，
 *  使用者可以通过 ACL_VSTRING_LEN(x) 宏来判断内容是否为空，返回的 ACL_VSTRING
 *  指针如果为该函数内部创建的，则用户名必须用 acl_vstring_free 进行释放
 */
ACL_API ACL_VSTRING* acl_xml_build(ACL_XML* xml, ACL_VSTRING *buf);

/**
 * 将 xml 对象转储于指定流中，注：该转储信息仅为调试用的数据
 * @param xml {ACL_XML*} xml 对象
 * @param fp {ACL_VSTREAM*} 流对象
 */
ACL_API void acl_xml_dump(ACL_XML *xml, ACL_VSTREAM *fp);

/**
 * 将 xml 对象转存于指定缓冲区中，注：该转储信息仅为调试用的数据
 * @param xml {ACL_XML*} xml 对象
 * @param buf {ACL_VSTRING*} 缓冲区, 需要用户自己分配空间
 */
ACL_API void acl_xml_dump2(ACL_XML *xml, ACL_VSTRING *buf);

/***************************************************************************/
/*          以下为更为低级的接口, 用户可以根据需要调用以下接口             */
/***************************************************************************/

/*----------------------------- in acl_xml.c ------------------------------*/

/**
 * 创建 xml 节点的属性
 * @param node {ACL_XML_NODE*} xml 节点
 * @return {ACL_XML_ATTR*} 新创建的节点属性
 */
ACL_API ACL_XML_ATTR *acl_xml_attr_alloc(ACL_XML_NODE *node);

/**
 * 创建一个 xml 节点
 * @param xml {ACL_XML*} xml 对象
 * @return {ACL_XML_NODE*} xml 节点对象
 */
ACL_API ACL_XML_NODE *acl_xml_node_alloc(ACL_XML *xml);

/**
 * 将某个 xml 节点及其子节点从 xml 对象中删除, 并释放该节点及其子节点所占空间
 * 函数来释放该 xml 节点所占内存
 * @param node {ACL_XML_NODE*} xml 节点
 * @return {int} 返回删除的节点个数
 */
ACL_API int acl_xml_node_delete(ACL_XML_NODE *node);

/**
 * 向某个 xml 节点添加兄弟节点(该兄弟节点必须是独立的 xml 节点)
 * @param node1 {ACL_XML_NODE*} 向本节点添加 xml 节点
 * @param node2 {ACL_XML_NODE*} 新添加的兄弟 xml 节点
 */
ACL_API void acl_xml_node_append(ACL_XML_NODE *node1, ACL_XML_NODE *node2);

/**
 * 将某个 xml 节点作为子节点加入某父 xml 节点中
 * @param parent {ACL_XML_NODE*} 父节点
 * @param child {ACL_XML_NODE*} 子节点
 */
ACL_API void acl_xml_node_add_child(ACL_XML_NODE *parent, ACL_XML_NODE *child);

/**
 * 获得某个 xml 节点的父节点
 * @param node {ACL_XML_NODE*} xml 节点
 * @return {ACL_XML_NODE*} 父节点, 如果为 NULL 则表示其父节点不存在
 */
ACL_API ACL_XML_NODE *acl_xml_node_parent(ACL_XML_NODE *node);

/**
 * 获得某个 xml 节点的后一个兄弟节点
 * @param node {ACL_XML_NODE*} xml 节点
 * @return {ACL_XML_NODE*} 给定 xml 节点的后一个兄弟节点, 若为NULL则表示不存在
 */
ACL_API ACL_XML_NODE *acl_xml_node_next(ACL_XML_NODE *node);

/**
 * 获得某个 xml 节点的前一个兄弟节点
 * @param node {ACL_XML_NODE*} xml 节点
 * @return {ACL_XML_NODE*} 给定 xml 节点的前一个兄弟节点, 若为NULL则表示不存在
 */
ACL_API ACL_XML_NODE *acl_xml_node_prev(ACL_XML_NODE *node);

#ifdef __cplusplus
}
#endif
#endif
