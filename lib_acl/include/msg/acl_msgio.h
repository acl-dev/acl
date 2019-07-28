#ifndef ACL_MSGIO_INCLUDE_H
#define ACL_MSGIO_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstring.h"
#include "../stdlib/acl_ring.h"
#include "../aio/acl_aio.h"

typedef struct ACL_MSGIO ACL_MSGIO;
typedef struct ACL_MSGIO_INFO ACL_MSGIO_INFO;

typedef int (*ACL_MSGIO_NOTIFY_FN)(int msg_type, ACL_MSGIO *peer,
				   const ACL_MSGIO_INFO *info, void *arg);

/**
 * 消息类型定义
 */
#define ACL_MSGIO_OK				0
#define ACL_MSGIO_TIMEOUT			1	/* 超时消息 */
#define ACL_MSGIO_EXCEPT			2	/* 异常消息 */
#define ACL_MSGIO_CONNECT			3	/* 连接成功消息 */
#define ACL_MSGIO_CONNECT_TIMEOUT		4	/* 连接超时消息 */
#define ACL_MSGIO_QUIT				5	/* 退出消息 */
#define	ACL_MSGIO_OPEN				6	/* 数据流打开消息 */

#define ACL_MSGIO_USER				1000

struct ACL_MSGIO_INFO {
	struct {
		int   type;
		int   dlen;
	} hdr;
	struct {
		ACL_VSTRING *buf;
	} body;
};

ACL_API void acl_msgio_init(void);
ACL_API void acl_msgio_close(ACL_MSGIO *mio);
ACL_API void acl_msgio_reg(ACL_MSGIO *mio, int id,
	ACL_MSGIO_NOTIFY_FN callback, void *arg);
ACL_API void acl_msgio_listen_reg(ACL_MSGIO *mio, int id,
	ACL_MSGIO_NOTIFY_FN callback, void *arg, int inherit);
ACL_API void acl_msgio_unreg(ACL_MSGIO *mio, int id, ACL_MSGIO_NOTIFY_FN callback);
ACL_API void acl_msgio_unreg_id(ACL_MSGIO *mio, int id);
ACL_API void acl_msgio_unreg_all(ACL_MSGIO *mio);

ACL_API int acl_msgio_wait(ACL_MSGIO *mio);
ACL_API ACL_MSGIO *acl_msgio_listen(ACL_AIO *aio, const char *addr);
ACL_API ACL_MSGIO *acl_msgio_accept(ACL_MSGIO *listener);
ACL_API ACL_MSGIO *acl_msgio_connect(ACL_AIO *aio, const char *addr, int timeout);
ACL_API void acl_msgio_set_noblock(ACL_AIO *aio, ACL_MSGIO *mio);

ACL_API int acl_msgio_send(ACL_MSGIO *mio, int type, void *data, int dlen);
/* void acl_msgio_timer(ACL_MSGIO *mio, ACL_MSGIO_NOTIFY_FN callback, void *arg); */

ACL_API void acl_msgio_addr(const ACL_MSGIO *mio, char *buf, size_t size);
ACL_API ACL_AIO *acl_msgio_aio(ACL_MSGIO *mio);
ACL_API ACL_VSTREAM *acl_msgio_vstream(ACL_MSGIO *mio);
ACL_API ACL_ASTREAM *acl_msgio_astream(ACL_MSGIO *mio);

#define ACL_MSGIO_ON_MSG(id, callback, arg) do { \
	acl_msgio_reg(NULL, id, callback, arg); \
} while(0)

#ifdef __cplusplus
}
#endif

#endif
