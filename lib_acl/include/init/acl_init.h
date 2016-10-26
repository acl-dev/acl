#ifndef ACL_INIT_INCLUDE_H
#define ACL_INIT_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

/**
 * ��ʼ������ACL��
 */
ACL_API void acl_lib_init(void);

/**
 * ��������ACL��
 */
ACL_API void acl_lib_end(void);

/**
 * �Ƿ�����ʹ�� poll ���� select
 * @param yesno {int} �� 0 ʱ��ʾ����ʹ�� poll
 */
ACL_API void acl_poll_prefered(int yesno);

/**
 * ��õ�ǰ acl ��İ汾��Ϣ
 * @return {const char*} ��ǰ acl ��汾��Ϣ
 */
ACL_API const char *acl_version(void);


/**
 * ������̵߳��̺߳�
 * @return {unsigned int}
 */
ACL_API unsigned long acl_main_thread_self(void);

#ifdef __cplusplus
}
#endif
#endif
