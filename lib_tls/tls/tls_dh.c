/*++
 * NAME
 *	tls_dh
 * SUMMARY
 *	Diffie-Hellman parameter support
 * SYNOPSIS
 *	#include <tls.h>
 *	#include <tls_private.h>
 *
 *	void	tls_set_dh_from_file(path, bits)
 *	const char *path;
 *	int	bits;
 *
 *	int	tls_set_eecdh_curve(server_ctx, grade)
 *	SSL_CTX	*server_ctx;
 *	const char *grade;
 *
 *	DH	*tls_tmp_dh_cb(ssl, export, keylength)
 *	SSL	*ssl;  acl_unused
 *	int	export;
 *	int	keylength;
 * DESCRIPTION
 *	This module maintains parameters for Diffie-Hellman key generation.
 *
 *	tls_tmp_dh_cb() is a call-back routine for the
 *	SSL_CTX_set_tmp_dh_callback() function.
 *
 *	tls_set_dh_from_file() overrides compiled-in DH parameters
 *	with those specified in the named files. The file format
 *	is as expected by the PEM_read_DHparams() routine. The
 *	"bits" argument must be 512 or 1024.
 *
 *	tls_set_eecdh_curve() enables ephemeral Elliptic-Curve DH
 *	key exchange algorithms by instantiating in the server SSL
 *	context a suitable curve (corresponding to the specified
 *	EECDH security grade) from the set of named curves in RFC
 *	4492 Section 5.1.1. Errors generate warnings, but do not
 *	disable TLS, rather we continue without EECDH. A zero
 *	result indicates that the grade is invalid or the corresponding
 *	curve could not be used.
 * DIAGNOSTICS
 *	In case of error, tls_set_dh_from_file() logs a warning and
 *	ignores the request.
 * LICENSE
 * .ad
 * .fi
 *	This software is free. You can do with it whatever you want.
 *	The original author kindly requests that you acknowledge
 *	the use of his software.
 * AUTHOR(S)
 *	Originally written by:
 *	Lutz Jaenicke
 *	BTU Cottbus
 *	Allgemeine Elektrotechnik
 *	Universitaetsplatz 3-4
 *	D-03044 Cottbus, Germany
 *
 *	Updated by:
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

#include "StdAfx.h"

#ifdef USE_TLS
#include <stdio.h>

/* TLS library. */

#include "tls.h"
#include "tls_params.h"
#include "tls_private.h"

/* Application-specific. */

 /*
  * Compiled-in EDH primes (the compiled-in generator is always 2). These are
  * used when no parameters are explicitly loaded from a site-specific file.
  * 
  * 512-bit parameters are used for export ciphers, and 1024-bit parameters are
  * used for non-export ciphers. An ~80-bit strong EDH key exchange is really
  * too weak to protect 128+ bit keys, but larger DH primes are
  * computationally expensive. When greater security is required, use EECDH.
  */

 /*
  * Generated via "openssl dhparam -2 -noout -C 512 2>/dev/null" TODO:
  * generate at compile-time.
  */
static unsigned char dh512_p[] = {
    0x88, 0x3F, 0x00, 0xAF, 0xFC, 0x0C, 0x8A, 0xB8, 0x35, 0xCD, 0xE5, 0xC2,
    0x0F, 0x55, 0xDF, 0x06, 0x3F, 0x16, 0x07, 0xBF, 0xCE, 0x13, 0x35, 0xE4,
    0x1C, 0x1E, 0x03, 0xF3, 0xAB, 0x17, 0xF6, 0x63, 0x50, 0x63, 0x67, 0x3E,
    0x10, 0xD7, 0x3E, 0xB4, 0xEB, 0x46, 0x8C, 0x40, 0x50, 0xE6, 0x91, 0xA5,
    0x6E, 0x01, 0x45, 0xDE, 0xC9, 0xB1, 0x1F, 0x64, 0x54, 0xFA, 0xD9, 0xAB,
    0x4F, 0x70, 0xBA, 0x5B,
};

 /*
  * Generated via "openssl dhparam -2 -noout -C 1024 2>/dev/null" TODO:
  * generate at compile-time.
  */
static unsigned char dh1024_p[] = {
    0xB0, 0xFE, 0xB4, 0xCF, 0xD4, 0x55, 0x07, 0xE7, 0xCC, 0x88, 0x59, 0x0D,
    0x17, 0x26, 0xC5, 0x0C, 0xA5, 0x4A, 0x92, 0x23, 0x81, 0x78, 0xDA, 0x88,
    0xAA, 0x4C, 0x13, 0x06, 0xBF, 0x5D, 0x2F, 0x9E, 0xBC, 0x96, 0xB8, 0x51,
    0x00, 0x9D, 0x0C, 0x0D, 0x75, 0xAD, 0xFD, 0x3B, 0xB1, 0x7E, 0x71, 0x4F,
    0x3F, 0x91, 0x54, 0x14, 0x44, 0xB8, 0x30, 0x25, 0x1C, 0xEB, 0xDF, 0x72,
    0x9C, 0x4C, 0xF1, 0x89, 0x0D, 0x68, 0x3F, 0x94, 0x8E, 0xA4, 0xFB, 0x76,
    0x89, 0x18, 0xB2, 0x91, 0x16, 0x90, 0x01, 0x99, 0x66, 0x8C, 0x53, 0x81,
    0x4E, 0x27, 0x3D, 0x99, 0xE7, 0x5A, 0x7A, 0xAF, 0xD5, 0xEC, 0xE2, 0x7E,
    0xFA, 0xED, 0x01, 0x18, 0xC2, 0x78, 0x25, 0x59, 0x06, 0x5C, 0x39, 0xF6,
    0xCD, 0x49, 0x54, 0xAF, 0xC1, 0xB1, 0xEA, 0x4A, 0xF9, 0x53, 0xD0, 0xDF,
    0x6D, 0xAF, 0xD4, 0x93, 0xE7, 0xBA, 0xAE, 0x9B,
};

 /*
  * Cached results.
  */
static __thread DH *dh_1024 = 0;
static __thread DH *dh_512 = 0;

/* free_dh_cb - call-back for free DH */

static void free_dh_cb(void *arg)
{
    DH *dh_tmp = (DH*) arg;

    DH_free(dh_tmp);
}

/* tls_set_dh_from_file - set Diffie-Hellman parameters from file */

void    tls_set_dh_from_file(const char *path, int bits)
{
    const char *myname = "tls_set_dh_from_file";
    FILE   *paramfile;
    DH    **dhPtr = 0;

    switch (bits) {
    case 512:
	dhPtr = &dh_512;
	break;
    case 1024:
	dhPtr = &dh_1024;
	break;
    default:
	acl_msg_panic("Invalid DH parameters size %d, file %s", bits, path);
    }

    if (*dhPtr != 0)
	return;

    if ((paramfile = fopen(path, "r")) != 0) {
	if ((*dhPtr = PEM_read_DHparams(paramfile, 0, 0, 0)) == 0) {
	    acl_msg_warn("%s: cannot load %d-bit DH parameters from file %s"
		    " -- using compiled-in defaults", myname, bits, path);
	    tls_print_errors();
	}
	(void) fclose(paramfile);		/* 200411 */
    } else {
	acl_msg_warn("%s: cannot load %d-bit DH parameters from file %s: %s"
		" -- using compiled-in defaults", myname, bits, path, acl_last_serror());
    }
}

/* tls_get_dh - get compiled-in DH parameters */

static DH *tls_get_dh(const unsigned char *p, int plen)
{
    const char *myname = "tls_get_dh";
    DH     *dh;
    static unsigned char g[] = {0x02,};

    /* Use the compiled-in parameters. */
    if ((dh = DH_new()) == 0) {
	acl_msg_warn("%s: cannot create DH parameter set: %s",
		myname, acl_last_serror());	/* 200411 */
	return (0);
    }
    dh->p = BN_bin2bn(p, plen, (BIGNUM *) 0);
    dh->g = BN_bin2bn(g, 1, (BIGNUM *) 0);
    if ((dh->p == 0) || (dh->g == 0)) {
	acl_msg_warn("%s: cannot load compiled-in DH parameters", myname);	/* 200411 */
	DH_free(dh);				/* 200411 */
	return (0);
    }
    return (dh);
}

/* tls_tmp_dh_cb - call-back for Diffie-Hellman parameters */

DH     *tls_tmp_dh_cb(SSL *unused_ssl acl_unused, int export, int keylength)
{
    DH     *dh_tmp;

    if (export && keylength == 512) {		/* 40-bit export cipher */
	if (dh_512 == 0) {
	    dh_512 = tls_get_dh(dh512_p, (int) sizeof(dh512_p));
	    if (dh_512)
		acl_pthread_atexit_add(dh_512, free_dh_cb);
	}
	dh_tmp = dh_512;
    } else {					/* ADH, DHE-RSA or DSA */
	if (dh_1024 == 0) {
	    dh_1024 = tls_get_dh(dh1024_p, (int) sizeof(dh1024_p));
	    if (dh_1024)
		acl_pthread_atexit_add(dh_1024, free_dh_cb);
	}
	dh_tmp = dh_1024;
    }
    return (dh_tmp);
}

int     tls_set_eecdh_curve(SSL_CTX *server_ctx acl_unused, const char *grade acl_unused)
{
#if OPENSSL_VERSION_NUMBER >= 0x00909000 && !defined(OPENSSL_NO_ECDH)
    const char *myname = "tls_set_eecdh_curve";
    int     nid;
    EC_KEY *ecdh;
    const char *curve;
    int     g;

#define TLS_EECDH_INVALID	0
#define TLS_EECDH_NONE		1
#define TLS_EECDH_STRONG	2
#define TLS_EECDH_ULTRA		3
    static NAME_CODE eecdh_table[] = {
	"none", TLS_EECDH_NONE,
	"strong", TLS_EECDH_STRONG,
	"ultra", TLS_EECDH_ULTRA,
	0, TLS_EECDH_INVALID,
    };

    switch (g = name_code(eecdh_table, NAME_CODE_FLAG_NONE, grade)) {
    default:
	acl_msg_panic("%s: Invalid eecdh grade code: %d", myname, g);
    case TLS_EECDH_INVALID:
	acl_msg_warn("%s: Invalid TLS eecdh grade \"%s\": EECDH disabled", myname, grade);
	return (0);
    case TLS_EECDH_NONE:
	return (1);
    case TLS_EECDH_STRONG:
	curve = var_tls_eecdh_strong;
	break;
    case TLS_EECDH_ULTRA:
	curve = var_tls_eecdh_ultra;
	break;
    }

    /*
     * Elliptic-Curve Diffie-Hellman parameters are either "named curves"
     * from RFC 4492 section 5.1.1, or explicitly described curves over
     * binary fields. OpenSSL only supports the "named curves", which provide
     * maximum interoperability. The recommended curve for 128-bit
     * work-factor key exchange is "prime256v1" a.k.a. "secp256r1" from
     * Section 2.7 of http://www.secg.org/download/aid-386/sec2_final.pdf
     */

    if ((nid = OBJ_sn2nid(curve)) == NID_undef) {
	acl_msg_warn("%s: unknown curve \"%s\": disabling EECDH support", myname, curve);
	return (0);
    }
    ERR_clear_error();
    if ((ecdh = EC_KEY_new_by_curve_name(nid)) == 0
	    || SSL_CTX_set_tmp_ecdh(server_ctx, ecdh) == 0) {
	acl_msg_warn("%s: unable to use curve \"%s\": disabling EECDH support", myname, curve);
	tls_print_errors();
	return (0);
    }
#endif
    return (1);
}

#ifdef TEST

int     main(int unused_argc, char **unused_argv)
{
    tls_tmp_dh_cb(0, 1, 512);
    tls_tmp_dh_cb(0, 1, 1024);
    tls_tmp_dh_cb(0, 1, 2048);
    tls_tmp_dh_cb(0, 0, 512);
    return (0);
}

#endif

#endif
