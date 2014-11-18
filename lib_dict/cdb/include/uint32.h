#ifndef UINT32_H
#define UINT32_H

typedef unsigned long uint32;

extern void uint32_pack(char *,uint32);
extern void uint32_pack_big(char *,uint32);
extern void uint32_unpack(char *,uint32 *);
extern void uint32_unpack_big(char *,uint32 *);

#endif
