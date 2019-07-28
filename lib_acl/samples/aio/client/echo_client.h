#ifndef	__AIO_ECHO_CLIENT_INCLUDE_H__
#define	__AIO_ECHO_CLIENT_INCLUDE_H__

#define	ECHO_CTL_END		0
#define	ECHO_CTL_SERV_ADDR	1
#define	ECHO_CTL_MAX_CONNECT	2
#define	ECHO_CTL_MAX_LOOP	3
#define	ECHO_CTL_TIMEOUT	4
#define	ECHO_CTL_DATA_LEN	5
#define	ECHO_CTL_NCONN_PERSEC	6
#define	ECHO_CTL_EVENT_MODE	7

void echo_client_init(int name, ...);
void echo_client_start(int use_slice);

#endif

