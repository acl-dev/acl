#ifndef __LIB_ICMP_TYPE_INCLUDE_H__
#define __LIB_ICMP_TYPE_INCLUDE_H__

typedef struct ICMP_CHAT ICMP_CHAT;
typedef struct ICMP_STAT ICMP_STAT;
typedef struct ICMP_HOST ICMP_HOST;
typedef struct ICMP_PKT ICMP_PKT;
typedef struct ICMP_PKT_STATUS ICMP_PKT_STATUS;

/**< ICMP 通信过程中每个主机的 PING 响应状态信息汇总 */
struct ICMP_STAT {
	double tmin;                    /**< 最短时间 */
	double tmax;                    /**< 最长时间 */
	double tsum;                    /**< 总时间 */
	double tave;                    /**< 平均时间 */
	size_t nsent;                   /**< 已经发送的包个数 */
	size_t nreceived;               /**< 已经收到的包个数 */
	double loss;					/**< 丢失的包个数 */
};

/**< ICMP 所发送的每个 PING 包之后的主机状态应答 */
struct ICMP_PKT_STATUS {
	size_t reply_len;               /**< 包回复的数据长度 */
	char  frome_ip[32];             /**< 源地址 */

	double rtt;                     /**< 往返时间(毫秒)(Round Trip Time) */
	unsigned short seq;             /**< 序列号(seq no) */
	unsigned char ttl;              /**< 生存时间(time to live) */
	char status;
#define ICMP_STATUS_OK          0
#define ICMP_STATUS_UNREACH     1
#define ICMP_STATUS_TIMEOUT     2
};

/**< 目的主机信息结构 */
struct ICMP_HOST {
	ICMP_STAT icmp_stat;        /**< ICMP 通信过程中的状态 */
	char dest_ip[32];           /**< 目的主机IP地址 */
	char domain[64];            /**< 目的主机IP所对应的域名 */
	struct sockaddr_in dest;    /**< 发送包时目的主机地址 */
	struct sockaddr_in from;    /**< 接收包时源主机地址 */
	int   from_len;             /**< 接收包时存储在 from 中的地址长度 */
	int   delay;                /**< 间隔发送PING包的时间，单位为秒 */
	int   timeout;              /**< 超时时间 */
	size_t dlen;                /**< 每个发送包的大小(字节) */
	size_t npkt;                /**< 设置的向该目的主机发送包的个数 */
	size_t nsent;               /**< 已经发送给该目的主机包的个数 */

	char  enable_log;           /**< 是否将响应包的信息记日志 */
	ACL_RING host_ring;         /**< 由此链入 ICMP_CHAT->host_head 链中 */
	ACL_RING pkt_head;          /**< 发送给目的主机数据包的链的链头 */
	ICMP_CHAT *chat;            /**< 所属的通信对象 */

	/**< 汇报发送包的响应包状态 */
	void (*stat_respond)(ICMP_PKT_STATUS*, void*);

	/**< 该发送包的响应包超时 */
	void (*stat_timeout)(ICMP_PKT_STATUS*, void*);

	/**< 该主机不可达 */
	void (*stat_unreach)(ICMP_PKT_STATUS*, void*);

	/**< 当主机的包发完时的回调函数 */
	void (*stat_finish)(ICMP_HOST*, void*);

	void *arg;                  /**< 应用传递的私有参数地址 */
};

#endif
