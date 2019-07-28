#ifndef __HOOK_HEAD_H__
#define __HOOK_HEAD_H__

extern struct dns_resolv_conf *var_dns_conf;
extern struct dns_hosts *var_dns_hosts;
extern struct dns_hints *var_dns_hints;

extern void dns_set_read_wait(int timeout);
extern void dns_init(void);

#endif
