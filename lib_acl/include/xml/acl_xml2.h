#ifndef ACL_XML2_INCLUDE_H
#define ACL_XML2_INCLUDE_H

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

typedef struct ACL_XML2	ACL_XML2;
typedef struct ACL_XML2_NODE	ACL_XML2_NODE;
typedef struct ACL_XML2_ATTR	ACL_XML2_ATTR;

struct ACL_XML2_ATTR {
	ACL_XML2_NODE *node;            /**< 所属节点 */
	char *name;                     /**< 属性名 */
	char *value;                    /**< 属性值 */

	size_t name_size;               /**< 属性名长度 */
	size_t value_size;              /**< 属性值长度 */

	/* private */
	int   quote;                    /**< 非 0 表示 ' 或 " */
	int   backslash;                /**< 转义字符 \ */
	int   slash;                    /**< 是否有 '/' 标志位设定 */
};

struct ACL_XML2_NODE {
	char *ltag;                     /**< 左标签名 */
	char *rtag;                     /**< 右标签名 */
	size_t ltag_size;               /**< 左标签名长度 */
	size_t rtag_size;               /**< 右标签名长度 */
	const char *id;                 /**< ID标识符, 只有在 xml->id_table
					  存在的节点的 id 才非空 */
	char *text;                     /**< 文本显示内容 */
	size_t text_size;               /**< 文件数据长度 */

	ACL_ARRAY *attr_list;           /**< 属性(ACL_XML2_ATTR)列表 */
	ACL_XML2_NODE *parent;          /**< 父节点 */
	ACL_RING children;              /**< 子节点集合 */
	int  depth;                     /**< 当前节点的深度 */

	/* private */
	ACL_XML2 *xml;                  /**< xml 对象 */
	ACL_RING node;                  /**< 当前节点 */
	ACL_XML2_ATTR *curr_attr;       /**< 当前正在解析的属性 */
	int   quote;                    /**< 非 0 表示 ' 或 " */
	int   last_ch;                  /**< 所记录本节点的前一个字节值 */
	int   nlt;                      /**< '<' 个数 */
	char  meta[3];                  /**< 元数据临时缓冲区 */

	unsigned int flag;
#define	ACL_XML2_F_META_QM	(1 << 0)    /**< '?' flag */
#define	ACL_XML2_F_META_CM	(1 << 1)    /**< '!--' flag */
#define	ACL_XML2_F_META_EM	(1 << 2)    /**< only '!' flag */
#define ACL_XML2_F_SELF_CL	(1 << 3)    /**< self closed flag */
#define	ACL_XML2_F_LEAF		(1 << 4)    /**< leaf node has no child node */
#define	ACL_XML2_F_CDATA	(1 << 5)    /**< CDATA data */

/**< 是否是元数据 */
#define	ACL_XML2_F_META		\
	(ACL_XML2_F_META_QM | ACL_XML2_F_META_CM | ACL_XML2_F_META_EM)

#define	ACL_XML2_IS_COMMENT(x)	(((x)->flag & ACL_XML2_F_META_CM))

#define	ACL_XML2_IS_CDATA(x)	(((x)->flag & ACL_XML2_F_CDATA))

	int   status;                   /**< 状态机当前解析状态 */
#define ACL_XML2_S_NXT		0       /**< 下一个节点 */
#define ACL_XML2_S_LLT		1       /**< 左边 '<' */
#define ACL_XML2_S_LGT		2       /**< 右边 '>' */
#define	ACL_XML2_S_LCH		3       /**< 左边 '<' 后第一个字节 */
#define ACL_XML2_S_LEM		4       /**< 左边 '<' 后的 '!' */
#define ACL_XML2_S_LTAG		5       /**< 左边的标签名 */
#define ACL_XML2_S_RLT		6       /**< 右边的 '<' */
#define ACL_XML2_S_RGT		7       /**< 右边的 '>' */
#define ACL_XML2_S_RTAG		8       /**< 右边的标签名 */
#define ACL_XML2_S_ATTR		9       /**< 标签属性名 */
#define ACL_XML2_S_AVAL		10      /**< 标签属性值 */
#define ACL_XML2_S_TXT		11      /**< 节点文本 */
#define ACL_XML2_S_MTAG		12      /**< 元数据标签 */
#define ACL_XML2_S_MTXT		13      /**< 元数据文本 */
#define ACL_XML2_S_MCMT		14      /**< 元数据注释 */
#define ACL_XML2_S_MEND		15      /**< 元数据结束 */
#define	ACL_XML2_S_CDATA	16      /**< CDATA 数据 */

	/* for acl_iterator, 通过 acl_foreach 列出该节点的一级子节点 */

	/* 取迭代器头函数 */
	ACL_XML2_NODE *(*iter_head)(ACL_ITER*, ACL_XML2_NODE*);
	/* 取迭代器下一个函数 */
	ACL_XML2_NODE *(*iter_next)(ACL_ITER*, ACL_XML2_NODE*);
	/* 取迭代器尾函数 */
	ACL_XML2_NODE *(*iter_tail)(ACL_ITER*, ACL_XML2_NODE*);
	/* 取迭代器上一个函数 */
	ACL_XML2_NODE *(*iter_prev)(ACL_ITER*, ACL_XML2_NODE*);
};

struct ACL_XML2 {
	/* public */
					
	int   depth;                    /**< 最大深度 */
	int   node_cnt;                 /**< 节点总数, 包括 root 节点 */
	int   root_cnt;                 /**< 根节点个数 */
	ACL_XML2_NODE *root;            /**< XML 根节点 */

	/* private */
	char *addr;                     /**< 内存起始地址 */
	char *ptr;                      /**< 内存地址 */
	size_t size;                    /**< addr 内存映射区的大小 */
	size_t len;                     /**< addr 内存映射区剩余大小 */

	char *mm_file;                  /**< 非空时指定内存映射文件名 */
	char *mm_addr;                  /**< 内存映射起始地址 */
	ACL_FILE_HANDLE fd;             /**< 采用内存映射方式时的文件句柄 */
	size_t off;                     /**< 当前内存映射文件的实际映射大小 */
	size_t block;                   /**< len 自增时的自增块长度大小 */
	int   keep_open;                /**< 文件句柄是否一直打开 */

	ACL_HTABLE *id_table;           /**< id 标识符哈希表 */
	ACL_XML2_NODE *curr_node;       /**< 当前正在处理的 XML 节点 */
	ACL_DBUF_POOL *dbuf;            /**< 内存池对象 */
	ACL_DBUF_POOL *dbuf_inner;      /**< 内部分布的内存池对象 */
	size_t dbuf_keep;               /**< 内存池中保留的长度 */

	unsigned flag;                  /**< 标志位: ACL_XML2_FLAG_xxx */ 

	/**< 是否允许一个 xml 文档中有多个根节点，内部缺省为允许 */
#define	ACL_XML2_FLAG_MULTI_ROOT	(1 << 0)

	/**< 是否兼容单节点中没有 '/' 情况 */
#define	ACL_XML2_FLAG_IGNORE_SLASH	(1 << 1)

	/**< 是否需要对文本数据进行 xml 解码  */
#define	ACL_XML2_FLAG_XML_DECODE	(1 << 2)

	/* for acl_iterator, 通过 acl_foreach 可以列出所有子节点 */

	/* 取迭代器头函数 */
	ACL_XML2_NODE *(*iter_head)(ACL_ITER*, ACL_XML2*);
	/* 取迭代器下一个函数 */
	ACL_XML2_NODE *(*iter_next)(ACL_ITER*, ACL_XML2*);
	/* 取迭代器尾函数 */
	ACL_XML2_NODE *(*iter_tail)(ACL_ITER*, ACL_XML2*);
	/* 取迭代器上一个函数 */
	ACL_XML2_NODE *(*iter_prev)(ACL_ITER*, ACL_XML2*);
};

/****************************************************************************/
/*                  公共函数接口，用户可以放心使用该接口集                  */
/****************************************************************************/

/*----------------------------- in acl_xml2.c ------------------------------*/

/**
 * 判断 xml 对象是否闭合的, 即是否所解析的数据是否完整, 如果该 xml 对象里的
 * xml 节点元素为空, 则也认为不是闭合的
 * @param xml {ACL_XML2*} xml 对象
 * @return {int} 0: 否; 1: 是
 */
ACL_API int acl_xml2_is_closure(ACL_XML2 *xml);

/**
 * 根据指定标签名判断 xml 解析已经完成, 当该标签与 xml 对象中 root 一级子节点
 * 中的最后一个 xml 节点的标签相同时, 则认为 xml 解析完成, 为保证判断的正确性,
 * 数据源应保证最外层的根节点只有一个, 即 xml->root 的一级子节点只有一个, 否则
 * 会造成误判
 * @param xml {ACL_XML2*} xml 对象
 * @param tag {const char*} 用户给定标签名, 内部在匹配时不区分大小写
 * @return {int} 0: 否; 1: 是
 */
ACL_API int acl_xml2_is_complete(ACL_XML2 *xml, const char *tag);

/**
 * 创建一个 xml 对象
 * @param addr {char*} 内存映射起始地址
 * @param size {ssize_t} addr 内存映射地址大小
 * @return {ACL_XML2*} 新创建的 xml 对象
 */
ACL_API ACL_XML2 *acl_xml2_alloc(char *addr, size_t size);

/**
 * 创建一个 xml 对象，该 xml 对象及所有的内部内存分配都在该内存池上进行分配
 * @param addr {char*} 内存映射起始地址
 * @param size {ssize_t} addr 内存映射地址大小
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，当该针对非 NULL 时，则 xml 对象
 *  及所属节点内存在其基础上进行分配，否则，内部自动创建隶属于 xml 的内存池
 * @return {ACL_XML2*} 新创建的 xml 对象
 */
ACL_API ACL_XML2 *acl_xml2_dbuf_alloc(char *addr, size_t size,
		ACL_DBUF_POOL *dbuf);

/**
 * 创建一个 xml 对象，其 xml 节点的创建的内存区建立在内存映射文件上
 * @param filepath {const char*} 内存映射文件的文件名
 * @param size {size_t} 所映射的文件大小
 * @param block {size_t} 每次进行空间大小扩充时的单位长度，在扩充时最大长度
 *  不会超过指定的 size 大小
 * @param keep_open {int} 是否一直保持文件被打开至 xml 对象释放，如果否，则
 *  当映射文件自动扩充时需要重新反复打开，会影响解析效率
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，当该针对非 NULL 时，则 xml 对象
 *  及所属节点内存在其基础上进行分配，否则，内部自动创建隶属于 xml 的内存池
 * @return {ACL_XML2*} 新创建的 xml 对象
 */
ACL_API ACL_XML2 *acl_xml2_mmap_file(const char *filepath, size_t size,
		size_t block, int keep_open, ACL_DBUF_POOL *dbuf);

/**
 * 创建一个 xml 对象，其 xml 节点的创建的内存区建立在内存映射文件上
 * @param fd {ACL_FILE_HANDLE} 内存映射文件的文件句柄，当创建 xml 对象成功后，该句柄
 *  需在 xml 对象释放后才关闭
 * @param size {size_t} 所映射的文件大小
 * @param block {size_t} 每次进行空间大小扩充时的单位长度，在扩充时最大长度
 *  不会超过指定的 size 大小
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，当该针对非 NULL 时，则 xml 对象
 *  及所属节点内存在其基础上进行分配，否则，内部自动创建隶属于 xml 的内存池
 * @return {ACL_XML2*} 新创建的 xml 对象
 */
ACL_API ACL_XML2 *acl_xml2_mmap_fd(ACL_FILE_HANDLE fd, size_t size,
		size_t block, ACL_DBUF_POOL *dbuf);

/**
 * 当采用内存文件映射方式时，此函数用来扩充映射文件的空间大小，因为在初始化时
 * 仅分配较小的空间，在使用过程中如果发现空间不足，则内部自动调用此函数扩展
 * 文件大小，这样既可以满足实际需求，又可以节省磁盘空间
 * @param xml {ACL_XML2*} 采用 acl_xml2_mmap_alloc 方式创建的 xml 对象
 * @return {size_t} 扩充后新增加的空间大小，如果返回值为 0，则表示出错或已经
 *  达到空间分配上限购
 */
ACL_API size_t acl_xml2_mmap_extend(ACL_XML2 *xml);

/**
 * 将某一个 ACL_XML2_NODE 节点作为一个 XML 对象的根节点，从而可以方便地遍历出该
 * 节点各级子节点(在遍历过程中的所有节点不含本节点自身)，该遍历方式有别于单独
 * 遍历某一个 ACL_XML2_NODE 节点时仅能遍历其一级子节点的情形
 * @param xml {ACL_XML2*} xml 对象
 * @param node {ACL_XML2_NODE*} AXL_XML_NODE 节点
 */
ACL_API void acl_xml2_foreach_init(ACL_XML2 *xml, ACL_XML2_NODE *node);

/**
 * 设置一个 xml 文档中是否允许有多个根 xml 节点，内部缺省支持多个根节点
 * @param xml {ACL_XML2*} xml 对象
 * @param on {int} 非 0 则允许，为 0 表示不允许，当禁止有多个根 xml 节点时，
 *  则在解析时当遇到第一个根节点结束时便返回剩余的数据
 */
ACL_API void acl_xml2_multi_root(ACL_XML2 *xml, int on);

/**
 * 对于 XML 单节点的情况, 是否允许可以没有 /, 如:
 * <test id=111>, <test id=111 />, 当可以允许没有 / 则这两种写法
 * 都是合法的，否则只有第二个写法是合法的，如果允许这种兼容性，则
 * 会造成一定的性能损失
 * @param xml {ACL_XML2*} xml 对象
 * @param ignore {int} 如果非 0 表示单节点必须有 /
 */
ACL_API void acl_xml2_slash(ACL_XML2 *xml, int ignore);

/**
 * 设置是否需要对 xml 对象中的属性值及文本值进行 xml 解码，内部缺省为不解
 * @param xml {ACL_XML2*}
 * @param on {int} 非 0 表示进行 xml 解码
 */
ACL_API void acl_xml2_decode_enable(ACL_XML2 *xml, int on);

/**
 * 释放一个 xml 对象, 同时释放该对象里容纳的所有 xml 节点
 * @param xml {ACL_XML2*} xml 对象
 * @return {int} 返回释放的 xml 节点个数
 */
ACL_API int acl_xml2_free(ACL_XML2 *xml);

/**
 * 重置 XML 解析器对象
 * @param xml {ACL_XML2*} xml 对象
 */
ACL_API void acl_xml2_reset(ACL_XML2 *xml);

/*------------------------- in acl_xml2_parse.c ----------------------------*/

/**
 * 解析 xml 数据, 并持续地自动生成 xml 节点树
 * @param xml {ACL_XML2*} xml 对象
 * @param data {const char*} 以 '\0' 结尾的数据字符串, 可以是完整的 xml 数据;
 *  也可以是不完整的 xml 数据, 允许循环调用此函数, 将不完整数据持续地输入
 * @return {const char*} 当通过 acl_xml2_multi_root 允许一个 xml 文档中在在
 *  多个根 xml 节点时，该函数返回的地址的字节为 '\0'; 否则返回剩余的数据地址
 *  包含非空字符串
 *  注：也可以通过 acl_xml2_is_complete 判断是否解析完毕
 */
ACL_API const char *acl_xml2_update(ACL_XML2 *xml, const char *data);
#define	acl_xml2_parse	acl_xml2_update

/*------------------------- in acl_xml2_util.c -----------------------------*/

/**
 * 初始化类似于 input, br, hr 等的自闭合标签, 形成自闭合标签树, 以便于
 * acl_xml2_tag_selfclosed 查询该树, 检查所给标签是否是保留的自闭合标签,
 * 该函数只能被初始化一次, 也可以不初始化
 */
ACL_API void acl_xml2_tag_init(void);

/**
 * 允许用户自己添加一些非自闭合的标签
 * @param tag {const char*} 标签名，注意标签长度不得大于 254 个字节
 */
ACL_API void acl_xml2_tag_add(const char *tag);

/**
 * 当调用 acl_xml2_tag_init 初始化保留的自闭合标签树后, 可以调用此函数判断所给
 * 标签是否属于自闭合标签, 如果未调用 acl_xml2_tag_init, 则该函数永远返回 0
 * @parma tag {const char*} 标签名称
 * @return {int} 0: 表示否, 1: 表示是
 */
ACL_API int  acl_xml2_tag_selfclosed(const char *tag);

/**
 * 判断标签所属 xml 节点是否是叶节点, 叶节点没有子节点
 * @param tag {const char*} 标签名
 * @return {int} 0: 不是叶节点; 1: 是叶节点
 */
ACL_API int  acl_xml2_tag_leaf(const char *tag);

/**
 * 释放由 acl_xml2_getElementsByTagName, acl_xml2_getElementsByName,
 * acl_xml2_getElementsByAttr 等函数返回的动态数组对象, 因为该动态数组中的
 * 元素都是 ACL_XML2 对象中元素的引用, 所以释放掉该动态数组后, 只要 ACL_XML2
 * 对象不释放, 则原来存于该数组中的元素依然可以使用.
 * 但并不释放里面的 xml 节点元素
 * @param a {ACL_ARRAY*} 动态数组对象
 */
ACL_API void acl_xml2_free_array(ACL_ARRAY *a);

/**
 * 从 xml 对象中获得与所给标签名相同的 xml 第一个节点
 * @param xml {ACL_XML2*} xml 对象
 * @param tag {const char*} 标签名称
 * @return {ACL_XML2_NODE*} 符合条件的 xml 节点, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点
 */
ACL_API ACL_XML2_NODE *acl_xml2_getFirstElementByTagName(
	ACL_XML2 *xml, const char *tag);

/**
 * 从 xml 对象中获得所有的与所给标签名相同的 xml 节点的集合
 * @param xml {ACL_XML2*} xml 对象
 * @param tag {const char*} 标签名称
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml2_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByTagName(
	ACL_XML2 *xml, const char *tag);

/**
 * 从 xml 对象中获得所有的与给定多级标签名相同的 xml 节点的集合
 * @param xml {ACL_XML2*} xml 对象
 * @param tags {const char*} 多级标签名，由 '/' 分隔各级标签名，如针对 xml 数据：
 *  <root> <first> <second> <third name="test1"> text1 </third> </second> </first>
 *  <root> <first> <second> <third name="test2"> text2 </third> </second> </first>
 *  <root> <first> <second> <third name="test3"> text3 </third> </second> </first>
 *  可以通过多级标签名：root/first/second/third 一次性查出所有符合条件的节点
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml2_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByTags(ACL_XML2 *xml, const char *tags);

/**
 * 从 xml 对象中获得所有的与给定属性名 name 的属性值相同的 xml 节点元素集合
 * @param xml {ACL_XML2*} xml 对象
 * @param value {const char*} 属性名为 name 的属性值
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml2_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByName(ACL_XML2 *xml, const char *value);

/**
 * 从 xml 对象中获得所有给定属性名及属性值的 xml 节点元素集合
 * @param xml {ACL_XML2*} xml 对象
 * @param name {const char*} 属性名
 * @param value {const char*} 属性值
 * @return {ACL_ARRAY*} 符合条件的 xml 节点集合, 存于 动态数组中, 若返回 NULL 则
 *  表示没有符合条件的 xml 节点, 非空值需要调用 acl_xml2_free_array 释放
 */
ACL_API ACL_ARRAY *acl_xml2_getElementsByAttr(ACL_XML2 *xml,
	const char *name, const char *value);

/**
 * 从 xml 对象中获得指定 id 值的 xml 节点元素的某个属性对象
 * @param xml {ACL_XML2*} xml 对象
 * @param id {const char*} id 值
 * @return {ACL_XML2_ATTR*} 某 xml 节点的某个属性对象, 若返回 NULL 则表示
 *  没有符合条件的属性, 返回值不需要释放
 */
ACL_API ACL_XML2_ATTR *acl_xml2_getAttrById(ACL_XML2 *xml, const char *id);

/**
 * 从 xml 对象中获得指定 id 值的 xml 节点元素的某个属性值
 * @param xml {ACL_XML2*} xml 对象
 * @param id {const char*} id 值
 * @return {const char*} 某 xml 节点的某个属性值, 若返回 NULL 则表示没有符合
 *  条件的属性
 */
ACL_API const char *acl_xml2_getAttrValueById(ACL_XML2 *xml, const char *id);

/**
 * 从 xml 对象中获得指定 id 值的 xml 节点元素
 * @param xml {ACL_XML2*} xml 对象
 * @param id {const char*} id 值
 * @return {ACL_XML2_NODE*} xml 节点元素, 若返回 NULL 则表示没有符合
 *  条件的 xml 节点, 返回值不需要释放
 */
ACL_API ACL_XML2_NODE *acl_xml2_getElementById(ACL_XML2 *xml, const char *id);

/**
 * 从 xml 对象中提取有在 ? ! 等开头的节点
 * @param xml {ACL_XML2*} xml 对象
 * @param tag {const char*} 标签名
 * @return {ACL_XML2_NODE*} xml 节点元素, 若返回 NULL 则表示没有符合
 *  条件的 xml 节点, 返回值不需要释放
 */
ACL_API ACL_XML2_NODE *acl_xml2_getElementMeta(ACL_XML2 *xml, const char *tag);

/**
 * 获得 xml 的字符集编码格式
 * @param xml {ACL_XML2*} xml 对象
 * @return {const char*} 返回字符集编码格式，返回 NULL 时表示没有该属性
 */
ACL_API const char *acl_xml2_getEncoding(ACL_XML2 *xml);

/**
 * 获得 xml 数据的类型，如：text/xsl
 * @param xml {ACL_XML2*} xml 对象
 * @return {const char*} 返回 NULL 表示没有该属性
 */
ACL_API const char *acl_xml2_getType(ACL_XML2 *xml);

/**
 * 从 xml 节点中获得指定属性名的属性对象
 * @param node {ACL_XML2_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @return {ACL_XML2_ATTR*} 属性对象, 为空表示不存在, 返回值不需要释放
 */
ACL_API ACL_XML2_ATTR *acl_xml2_getElementAttr(ACL_XML2_NODE *node, const char *name);

/**
 * 从 xml 节点中获得指定属性名的属性值
 * @param node {ACL_XML2_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @return {const char*} 属性值, 为空表示不存在
 */
ACL_API const char *acl_xml2_getElementAttrVal(ACL_XML2_NODE *node, const char *name);

/**
 * 从 xml 节点删除某个属性对象, 如果该属性为 id 属性, 则同时会从 xml->id_table 中删除
 * @param node {ACL_XML2_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @return {int} 0 表示删除成功, -1: 表示删除失败(有可能是该属性不存在)
 */
ACL_API int acl_xml2_removeElementAttr(ACL_XML2_NODE *node, const char *name);

/**
 * 给 xml 节点添加属性, 如果该属性名已存在, 则用新的属性值替换其属性值, 否则
 * 创建并添加新的属性对象
 * @param node {ACL_XML2_NODE*} xml 节点
 * @param name {const char*} 属性名称
 * @param value {const char*} 属性值
 * @return {ACL_XML2_ATTR*} 返回该属性对象(有可能是原来的, 也有可能是新的), 
 *  返回值不需释放
 */
ACL_API ACL_XML2_ATTR *acl_xml2_addElementAttr(ACL_XML2_NODE *node,
        const char *name, const char *value);

/**
 * 将标签名及节点文本做为参数创建 xml 节点，该函数主要用在构建 xml 对象时
 * @param xml {ACL_XML2*} xml 对象，该对象应该是由 acl_xml2_alloc 创建的
 * @param tagname {const char*} 标签名，必须非空且字符串长度大于 0
 * @param text {const char*} 节点的文本内容，可以为空
 * @return {ACL_XML2_NODE*} 新创建的 xml 节点，该返回永远返回非空，如果输入
 *  参数非法则会导致内部自动产生断言
 */
ACL_API ACL_XML2_NODE *acl_xml2_create_node(ACL_XML2 *xml,
	const char* tagname, const char* text);

/**
 * 给一个 xml 节点添加属性，该函数主要用在构建 xml 对象时
 * @param node {ACL_XML2_NODE*} 由 acl_xml2_create_node 创建的节点
 * @param name {const char*} 属性名，必须为非空字符串且字符串长度大于 0
 * @param value {const char*} 属性值，可以为空
 * @return {ACL_XML2_ATTR*} xml 节点的属性对象，当输入参数非法时该函数
 *  内部自动产生断言
 */
ACL_API ACL_XML2_ATTR *acl_xml2_node_add_attr(ACL_XML2_NODE *node,
	const char *name, const char *value);

/**
 * 给一个 xml 节点添加一组属性，该函数主要用在构建 xml 对象时
 * @param node {ACL_XML2_NODE*} 由 acl_xml2_create_node 创建的节点
 * @param ... 一组属性，遇到 NULL 时表示结束，如：
 *  {name1}, {value1}, {name2}, {value2}, ... NULL
 */
ACL_API void acl_xml2_node_add_attrs(ACL_XML2_NODE *node, ...);

/**
 * 给一个 xml 节点添加文本内容，该函数主要用在构建 xml 对象时
 * @param node {ACL_XML2_NODE*} 由 acl_xml2_create_node 创建的节点
 * @param text {const char*} 文本内容
 */
ACL_API void acl_xml2_node_set_text(ACL_XML2_NODE *node, const char *text);

/**
 * 将 xml 对象转成字符串内容
 * @param xml {ACL_XML2*} xml 对象
 * @return {const char*}
 */
ACL_API const char *acl_xml2_build(ACL_XML2* xml);

/**
 * 将 xml 对象转储于指定流中，注：该转储信息仅为调试用的数据
 * @param xml {ACL_XML2*} xml 对象
 * @param fp {ACL_VSTREAM*} 流对象
 */
ACL_API void acl_xml2_dump(ACL_XML2 *xml, ACL_VSTREAM *fp);

/**
 * 将 xml 对象转存于指定缓冲区中，注：该转储信息仅为调试用的数据
 * @param xml {ACL_XML2*} xml 对象
 * @param buf {ACL_VSTRING*} 缓冲区, 需要用户自己分配空间
 */
ACL_API void acl_xml2_dump2(ACL_XML2 *xml, ACL_VSTRING *buf);

/***************************************************************************/
/*          以下为更为低级的接口, 用户可以根据需要调用以下接口             */
/***************************************************************************/

/*----------------------------- in acl_xml2.c ------------------------------*/

/**
 * 创建 xml 节点的属性
 * @param node {ACL_XML2_NODE*} xml 节点
 * @return {ACL_XML2_ATTR*} 新创建的节点属性
 */
ACL_API ACL_XML2_ATTR *acl_xml2_attr_alloc(ACL_XML2_NODE *node);

/**
 * 创建一个 xml 节点
 * @param xml {ACL_XML2*} xml 对象
 * @return {ACL_XML2_NODE*} xml 节点对象
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_alloc(ACL_XML2 *xml);

/**
 * 将某个 xml 节点及其子节点从 xml 对象中删除, 并释放该节点及其子节点所占空间
 * 函数来释放该 xml 节点所占内存
 * @param node {ACL_XML2_NODE*} xml 节点
 * @return {int} 返回删除的节点个数
 */
ACL_API int acl_xml2_node_delete(ACL_XML2_NODE *node);

/**
 * 向某个 xml 节点添加兄弟节点(该兄弟节点必须是独立的 xml 节点)
 * @param node1 {ACL_XML2_NODE*} 向本节点添加 xml 节点
 * @param node2 {ACL_XML2_NODE*} 新添加的兄弟 xml 节点
 */
ACL_API void acl_xml2_node_append(ACL_XML2_NODE *node1, ACL_XML2_NODE *node2);

/**
 * 将某个 xml 节点作为子节点加入某父 xml 节点中
 * @param parent {ACL_XML2_NODE*} 父节点
 * @param child {ACL_XML2_NODE*} 子节点
 */
ACL_API void acl_xml2_node_add_child(ACL_XML2_NODE *parent, ACL_XML2_NODE *child);

/**
 * 获得某个 xml 节点的父节点
 * @param node {ACL_XML2_NODE*} xml 节点
 * @return {ACL_XML2_NODE*} 父节点, 如果为 NULL 则表示其父节点不存在
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_parent(ACL_XML2_NODE *node);

/**
 * 获得某个 xml 节点的后一个兄弟节点
 * @param node {ACL_XML2_NODE*} xml 节点
 * @return {ACL_XML2_NODE*} 给定 xml 节点的后一个兄弟节点, 若为NULL则表示不存在
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_next(ACL_XML2_NODE *node);

/**
 * 获得某个 xml 节点的前一个兄弟节点
 * @param node {ACL_XML2_NODE*} xml 节点
 * @return {ACL_XML2_NODE*} 给定 xml 节点的前一个兄弟节点, 若为NULL则表示不存在
 */
ACL_API ACL_XML2_NODE *acl_xml2_node_prev(ACL_XML2_NODE *node);

#ifdef __cplusplus
}
#endif
#endif
