#ifndef SCAN_H
#define SCAN_H

extern unsigned int scan_uint(char *,unsigned int *);
extern unsigned int scan_xint(char *,unsigned int *);
extern unsigned int scan_nbbint(char *,unsigned int,unsigned int,unsigned int,unsigned int *);
extern unsigned int scan_ushort(char *,unsigned short *);
extern unsigned int scan_xshort(char *,unsigned short *);
extern unsigned int scan_nbbshort(char *,unsigned int,unsigned int,unsigned int,unsigned short *);
extern unsigned int scan_ulong(char *,unsigned long *);
extern unsigned int scan_xlong(char *,unsigned long *);
extern unsigned int scan_nbblong(char *,unsigned int,unsigned int,unsigned int,unsigned long *);

extern unsigned int scan_plusminus(char *,int *);
extern unsigned int scan_0x(char *,unsigned int *);

extern unsigned int scan_whitenskip(char *,unsigned int);
extern unsigned int scan_nonwhitenskip(char *,unsigned int);
extern unsigned int scan_charsetnskip(char *,char *,unsigned int);
extern unsigned int scan_noncharsetnskip(char *,char *,unsigned int);

extern unsigned int scan_strncmp(char *,char *,unsigned int);
extern unsigned int scan_memcmp(char *,char *,unsigned int);

extern unsigned int scan_long(char *,long *);
extern unsigned int scan_8long(char *,unsigned long *);

#endif
