#ifndef MBEDTLS_VERSION_SELECTOR_H
#define MBEDTLS_VERSION_SELECTOR_H

# ifdef USE_MBEDTLS3
#  include "version3.6.5.h"
# else
#  include "version2.7.12.h"
# endif

#endif /* version.h */
