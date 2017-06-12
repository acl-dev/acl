#ifndef ACL_MASTER_MANAGER_INCLUDE_H
#define ACL_MASTER_MANAGER_INCLUDE_H

typedef struct MASTER_CONN {
	ACL_ASTREAM *conn;
	int status;
#define HTTP_S_HEAD_LINE	0
#define HTTP_S_HEAD_ENTRY	1
#define HTTP_S_BODY		2
} MASTER_CONN;

#endif
