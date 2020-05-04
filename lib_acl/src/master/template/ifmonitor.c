#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#endif

#ifdef ACL_LINUX

#include <stdlib.h>
#include <string.h>
#include <linux/netlink.h>
//#include <linux/route.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_iostuff.h"
#include "ifmonitor.h"

typedef struct NETLINK_CTX {
	monitor_callback callback;
	void *ctx;
} NETLINK_CTX;

static int netlink_changed(struct nlmsghdr *nh, unsigned int dlen)
{
	int  changed = 0;

	for (; NLMSG_OK(nh, dlen); nh = NLMSG_NEXT(nh, dlen)) {
		switch (nh->nlmsg_type) {
		case RTM_NEWADDR:
			//acl_msg_info("%s: RTM_NEWADDR", __FUNCTION__);
		case RTM_DELADDR:
			//acl_msg_info("%s: RTM_DELADDR", __FUNCTION__);
		case RTM_NEWROUTE:
			//acl_msg_info("%s: RTM_NEWROUTE", __FUNCTION__);
		case RTM_DELROUTE:
			//acl_msg_info("%s: RTM_DELROUTE", __FUNCTION__);
			changed++;
			break;
		default:
			break;
		}
	}

//	if (changed > 0)
//		acl_msg_info("%s: changed=%d", __FUNCTION__, changed);
	return changed;
}

static void netlink_callback(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream, void *context)
{
	NETLINK_CTX *nc = (NETLINK_CTX *) context;
	char buf[4096];
	int  ret;

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type: %d",
			__FILE__, __FUNCTION__, __LINE__, event_type);

	ret = acl_vstream_read(stream, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s, %s(%d): read error %s",
			__FILE__, __FUNCTION__, __LINE__, acl_last_serror());
	} else if (ret < (int) sizeof(struct nlmsghdr)) {
		acl_msg_error("%s, %s(%d): invalid read length=%d",
			__FILE__, __FUNCTION__, __LINE__, ret);
	} else {
		struct nlmsghdr *nh = (struct nlmsghdr *) buf;
		if (netlink_changed(nh, (unsigned) ret))
			nc->callback(nc->ctx);
	}
}

/* create monitor watching the network's changing status */
static ACL_VSTREAM *netlink_open(void)
{
	struct sockaddr_nl sa;
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	ACL_VSTREAM *stream;

	if (fd < 0) {
		acl_msg_error("%s(%d), %s: create raw socket error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return NULL;
	}

	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE |
		RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE;
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
		acl_msg_error("%s(%d), %s: bind raw socket error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		close(fd);
		return NULL;
	}

	stream = acl_vstream_fdopen(fd, O_RDWR, 8192, 0, ACL_VSTREAM_TYPE_SOCK);
	return stream;
}

void netlink_monitor(ACL_EVENT *event, monitor_callback callback, void *ctx)
{
	ACL_VSTREAM *stream = netlink_open();
	NETLINK_CTX *nc;
	if (stream == NULL)
		return;

	nc = (NETLINK_CTX *) acl_mycalloc(1, sizeof(NETLINK_CTX));
	nc->callback = callback;
	nc->ctx      = ctx;

	acl_event_enable_read(event, stream, 0, netlink_callback, nc);
}

#endif /* ACL_LINUX */
