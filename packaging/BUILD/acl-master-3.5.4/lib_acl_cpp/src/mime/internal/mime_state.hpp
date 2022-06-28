#ifndef __MIME_STATE_INCLUDE__
#define __MIME_STATE_INCLUDE__

#if !defined(ACL_MIME_DISABLE)

#include "lib_acl.h"
#include "acl_cpp/mime/mime_define.hpp"
#include "header_token.hpp"

typedef struct MIME_NODE MIME_NODE;
typedef struct MIME_STATE MIME_STATE;
typedef struct MAIL_ADDR MAIL_ADDR;

struct MAIL_ADDR 
{
	char *addr;
	char *comment;
};

struct MIME_NODE
{
	ACL_RING children;                      /**< 子结点集合 */
	int  depth;                             /**< 当前结点的深度 */
	MIME_NODE *parent;                      /**< 父结点 */
	MIME_STATE *state;                      /**< MIME_STATE 对象 */

	/* 邮件头 */
	ACL_FIFO *header_list;                  /**< HEADER_NV 集合 */
	ACL_FIFO *header_to_list;               /**< MAIL_ADDR 集合 */
	ACL_FIFO *header_cc_list;               /**< MAIL_ADDR 集合 */
	ACL_FIFO *header_bcc_list;              /**< MAIL_ADDR 集合 */
	char *header_sender;
	char *header_from;
	char *header_replyto;
	char *header_returnpath;
	char *header_subject;

	/* multipart 头 */
	char *header_filename;

	/* 通用头 */
	int   ctype;                            /**< MIME_CTYPE_XXX */
	int   stype;                            /**< MIME_STYPE_XXX */
	char *ctype_s;
	char *stype_s;
	char *charset;
	char *header_name;

	int   domain;
	int   encoding;                         /**< MIME_ENC_XXX */
	int   valid_line;
	char  last_ch;                          /**< 分析过程中记录的前一个字节 */
	char  last_lf;                          /**< 分析头部每行数据时前一个 \n */
	off_t last_cr_pos;                      /**< 上一个 \r 的偏移位置 */
	off_t last_lf_pos;                      /**< 上一个 \n 的偏移位置 */
	ACL_VSTRING *boundary;                  /**< 当是 multipart 邮件时存储分隔符 */

	/**< 当是 multipart 邮件时记录分隔符的下一个匹配位置，
	  当该值指向分隔符尾部时说明完全匹配完毕 */
	const char *bound_ptr;

	char  bound_term[3];
	ACL_VSTRING *buffer;                    /**< headers, quoted-printable body */
	ACL_RING node;                          /**< 当前结点 */

	off_t header_begin;			/**< 结点头开始位置 */
	off_t header_end;			/**< 结点头结束位置 */
	off_t body_begin;			/**< 结点体开始位置 */
	off_t body_end;				/**< 结点体结束位置 */
	off_t body_data_end;			/**< 结点数据体结束位置 */
	off_t bound_end;			/**< 分隔体结束位置 */

	/* for acl_iterator, 通过 acl_foreach 列出该结点的一级子结点 */

	/* 取迭代器头函数 */
	MIME_NODE *(*iter_head)(ACL_ITER*, MIME_NODE*);
	/* 取迭代器下一个函数 */
	MIME_NODE *(*iter_next)(ACL_ITER*, MIME_NODE*);
	/* 取迭代器尾函数 */
	MIME_NODE *(*iter_tail)(ACL_ITER*, MIME_NODE*);
	/* 取迭代器上一个函数 */
	MIME_NODE *(*iter_prev)(ACL_ITER*, MIME_NODE*);
};

#define MIME_MAX_TOKEN		3	/* tokens per attribute */

struct MIME_STATE
{
	int   depth;                    /**< 最大深度 */
	int   node_cnt;			/**< 结点总数, 包括 root 结点 */
	MIME_NODE *root;		/**< MIME_NODE 根结点 */
	int   use_crlf;			/**< 是用 \r\n 还是用 \n 做为换行符 */

	/* private */

	MIME_NODE *curr_node;           /**< 当前正在处理的 MIME_NODE 结点 */
	const char *curr_bound;         /**< 针对 multipart 邮件, 当前的分隔符 */
	off_t curr_off;                 /**< 针对邮件的当前偏移, 它总是会指向下一个位置 */
	int   curr_status;              /**< 状态机当前解析状态 */
#define MIME_S_HEAD                     0
#define MIME_S_BODY                     1
#define MIME_S_BODY_BOUND_CRLF          2
#define MIME_S_MULTI_BOUND              3
#define MIME_S_MULTI_BOUND_CRLF         4
#define MIME_S_TERM                     5

	HEADER_TOKEN token[MIME_MAX_TOKEN]; /* header token array */
	ACL_VSTRING *token_buffer;      /* header parser scratch buffer */
	ACL_VSTRING *key_buffer;

	/* for acl_iterator, 通过 acl_foreach 可以列出所有子结点 */

	/* 取迭代器头函数 */
	MIME_NODE *(*iter_head)(ACL_ITER*, MIME_STATE*);
	/* 取迭代器下一个函数 */
	MIME_NODE *(*iter_next)(ACL_ITER*, MIME_STATE*);
	/* 取迭代器尾函数 */
	MIME_NODE *(*iter_tail)(ACL_ITER*, MIME_STATE*);
	/* 取迭代器上一个函数 */
	MIME_NODE *(*iter_prev)(ACL_ITER*, MIME_STATE*);
};

MIME_STATE *mime_state_alloc(void);
void mime_state_foreach_init(MIME_STATE *state);
int mime_state_free(MIME_STATE *state);
int mime_state_reset(MIME_STATE *state);
int mime_state_update(MIME_STATE *state, const char *data, int len);
int mime_state_head_finish(MIME_STATE *state);

MIME_NODE *mime_node_new(MIME_STATE *state);
int mime_node_delete(MIME_NODE *node);
void mime_node_add_child(MIME_NODE *parent, MIME_NODE *child);
const char *mime_ctype_name(size_t ctype);
const char *mime_stype_name(size_t stype);
const char *mime_head_value(MIME_NODE *node, const char *name);

#endif // !defined(ACL_MIME_DISABLE)
#endif
