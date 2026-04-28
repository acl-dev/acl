#include <stdio.h>
#include <string.h>
#include "lib_acl.h"

int main(void) {
    /* verify key bytes stored correctly */
    const char *key = "k\x02ey";
    int klen = (int)strlen(key);
    fprintf(stderr, "key len=%d bytes:", klen);
    for (int i = 0; i < klen; i++) fprintf(stderr, " %02X", (unsigned char)key[i]);
    fprintf(stderr, "\n");

    /* test minimal escape logic */
    ACL_VSTRING *b = acl_vstring_alloc(64);
    const unsigned char *p = (const unsigned char *) key;
    char tmp[8];

    ACL_VSTRING_ADDCH(b, '"');
    while (*p) {
        if (*p < 0x20) {
            snprintf(tmp, sizeof(tmp), "\\u%04X", (unsigned)*p);
            fprintf(stderr, "appending escape for 0x%02X: [%s]\n", (unsigned)*p, tmp);
            acl_vstring_strcat(b, tmp);
        } else {
            ACL_VSTRING_ADDCH(b, *p);
        }
        p++;
    }
    ACL_VSTRING_ADDCH(b, '"');
    ACL_VSTRING_TERMINATE(b);

    const char *out = acl_vstring_str(b);
    int olen = (int)strlen(out);
    fprintf(stderr, "out len=%d bytes:", olen);
    for (int i = 0; i < olen; i++) fprintf(stderr, " %02X", (unsigned char)out[i]);
    fprintf(stderr, "\n");

    acl_vstring_free(b);
    return 0;
}
