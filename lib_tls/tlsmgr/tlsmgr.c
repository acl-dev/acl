#include "lib_acl.h"
/* TLS library. */

#ifdef USE_TLS
#include "tls.h"
#include "tls_params.h"

/* main - the main program, non-TLS version */

int     main(int argc, char **argv)
{
    var_tlsmgr_stand_alone = 1;
    tlsmgr_alone_start(argc, argv);
    return (0);
}

#endif
