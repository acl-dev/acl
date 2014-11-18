/*************************************************************************************************
 * The B+ tree database API of Tokyo Cabinet
 *                                                      Copyright (C) 2006-2009 Mikio Hirabayashi
 * This file is part of Tokyo Cabinet.
 * Tokyo Cabinet is free software; you can redistribute it and/or modify it under the terms of
 * the GNU Lesser General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License or any later version.  Tokyo Cabinet is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * You should have received a copy of the GNU Lesser General Public License along with Tokyo
 * Cabinet; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA.
 *************************************************************************************************/


#ifndef _TCBDB_H                         /* duplication check */
#define _TCBDB_H

#if defined(__cplusplus)
#define __TCBDB_CLINKAGEBEGIN extern "C" {
#define __TCBDB_CLINKAGEEND }
#else
#define __TCBDB_CLINKAGEBEGIN
#define __TCBDB_CLINKAGEEND
#endif
__TCBDB_CLINKAGEBEGIN


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <tcutil.h>
#include <tchdb.h>



/*************************************************************************************************
 * API
 *************************************************************************************************/


typedef struct {                         /* type of structure for a B+ tree database */
  void *mmtx;                            /* mutex for method */
  void *cmtx;                            /* mutex for cache */
  TCHDB *hdb;                            /* internal database object */
  char *opaque;                          /* opaque buffer */
  bool open;                             /* whether the internal database is opened */
  bool wmode;                            /* whether to be writable */
  uint32_t lmemb;                        /* number of members in each leaf */
  uint32_t nmemb;                        /* number of members in each node */
  uint8_t opts;                          /* options */
  uint64_t root;                         /* ID number of the root page */
  uint64_t first;                        /* ID number of the first leaf */
  uint64_t last;                         /* ID number of the last leaf */
  uint64_t lnum;                         /* number of leaves */
  uint64_t nnum;                         /* number of nodes */
  uint64_t rnum;                         /* number of records */
  TCMAP *leafc;                          /* cache for leaves */
  TCMAP *nodec;                          /* cache for nodes */
  TCCMP cmp;                             /* pointer to the comparison function */
  void *cmpop;                           /* opaque object for the comparison function */
  uint32_t lcnum;                        /* maximum number of cached leaves */
  uint32_t ncnum;                        /* maximum number of cached nodes */
  uint32_t lsmax;                        /* maximum size of each leaf */
  uint32_t lschk;                        /* counter for leaf size checking */
  uint64_t capnum;                       /* capacity number of records */
  uint64_t *hist;                        /* history array of visited nodes */
  int hnum;                              /* number of element of the history array */
  uint64_t hleaf;                        /* ID number of the leaf referred by the history */
  uint64_t lleaf;                        /* ID number of the last visited leaf */
  bool tran;                             /* whether in the transaction */
  char *rbopaque;                        /* opaque for rollback */
  uint64_t clock;                        /* logical clock */
  int64_t cnt_saveleaf;                  /* tesing counter for leaf save times */
  int64_t cnt_loadleaf;                  /* tesing counter for leaf load times */
  int64_t cnt_killleaf;                  /* tesing counter for leaf kill times */
  int64_t cnt_adjleafc;                  /* tesing counter for node cache adjust times */
  int64_t cnt_savenode;                  /* tesing counter for node save times */
  int64_t cnt_loadnode;                  /* tesing counter for node load times */
  int64_t cnt_adjnodec;                  /* tesing counter for node cache adjust times */
} TCBDB;

enum {                                   /* enumeration for additional flags */
  BDBFOPEN = HDBFOPEN,                   /* whether opened */
  BDBFFATAL = HDBFFATAL                  /* whetehr with fatal error */
};

enum {                                   /* enumeration for tuning options */
  BDBTLARGE = 1 << 0,                    /* use 64-bit bucket array */
  BDBTDEFLATE = 1 << 1,                  /* compress each page with Deflate */
  BDBTBZIP = 1 << 2,                     /* compress each record with BZIP2 */
  BDBTTCBS = 1 << 3,                     /* compress each page with TCBS */
  BDBTEXCODEC = 1 << 4                   /* compress each record with outer functions */
};

enum {                                   /* enumeration for open modes */
  BDBOREADER = 1 << 0,                   /* open as a reader */
  BDBOWRITER = 1 << 1,                   /* open as a writer */
  BDBOCREAT = 1 << 2,                    /* writer creating */
  BDBOTRUNC = 1 << 3,                    /* writer truncating */
  BDBONOLCK = 1 << 4,                    /* open without locking */
  BDBOLCKNB = 1 << 5,                    /* lock without blocking */
  BDBOTSYNC = 1 << 6                     /* synchronize every transaction */
};

typedef struct {                         /* type of structure for a B+ tree cursor */
  TCBDB *bdb;                            /* database object */
  uint64_t clock;                        /* logical clock */
  uint64_t id;                           /* ID number of the leaf */
  int32_t kidx;                          /* number of the key */
  int32_t vidx;                          /* number of the value */
} BDBCUR;

enum {                                   /* enumeration for cursor put mode */
  BDBCPCURRENT,                          /* current */
  BDBCPBEFORE,                           /* before */
  BDBCPAFTER                             /* after */
};


/* Get the message string corresponding to an error code.
   `ecode' specifies the error code.
   The return value is the message string of the error code. */
const char *tcbdberrmsg(int ecode);


/* Create a B+ tree database object.
   The return value is the new B+ tree database object. */
TCBDB *tcbdbnew(void);


/* Delete a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   If the database is not closed, it is closed implicitly.  Note that the deleted object and its
   derivatives can not be used anymore. */
void tcbdbdel(TCBDB *bdb);


/* Get the last happened error code of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the last happened error code.
   The following error codes are defined: `TCESUCCESS' for success, `TCETHREAD' for threading
   error, `TCEINVALID' for invalid operation, `TCENOFILE' for file not found, `TCENOPERM' for no
   permission, `TCEMETA' for invalid meta data, `TCERHEAD' for invalid record header, `TCEOPEN'
   for open error, `TCECLOSE' for close error, `TCETRUNC' for trunc error, `TCESYNC' for sync
   error, `TCESTAT' for stat error, `TCESEEK' for seek error, `TCEREAD' for read error,
   `TCEWRITE' for write error, `TCEMMAP' for mmap error, `TCELOCK' for lock error, `TCEUNLINK'
   for unlink error, `TCERENAME' for rename error, `TCEMKDIR' for mkdir error, `TCERMDIR' for
   rmdir error, `TCEKEEP' for existing record, `TCENOREC' for no record found, and `TCEMISC' for
   miscellaneous error. */
int tcbdbecode(TCBDB *bdb);


/* Set mutual exclusion control of a B+ tree database object for threading.
   `bdb' specifies the B+ tree database object which is not opened.
   If successful, the return value is true, else, it is false.
   Note that the mutual exclusion control is needed if the object is shared by plural threads and
   this function should should be called before the database is opened. */
bool tcbdbsetmutex(TCBDB *bdb);


/* Set the custom comparison function of a B+ tree database object.
   `bdb' specifies the B+ tree database object which is not opened.
   `cmp' specifies the pointer to the custom comparison function.  It receives five parameters.
   The first parameter is the pointer to the region of one key.  The second parameter is the size
   of the region of one key.  The third parameter is the pointer to the region of the other key.
   The fourth parameter is the size of the region of the other key.  The fifth parameter is the
   pointer to the optional opaque object.  It returns positive if the former is big, negative if
   the latter is big, 0 if both are equivalent.
   `cmpop' specifies an arbitrary pointer to be given as a parameter of the comparison function.
   If it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false.
   The default comparison function compares keys of two records by lexical order.  The functions
   `tctccmplexical' (dafault), `tctccmpdecimal', `tctccmpint32', and `tctccmpint64' are built-in.
   Note that the comparison function should be set before the database is opened.  Moreover,
   user-defined comparison functions should be set every time the database is being opened. */
bool tcbdbsetcmpfunc(TCBDB *bdb, TCCMP cmp, void *cmpop);


/* Set the tuning parameters of a B+ tree database object.
   `bdb' specifies the B+ tree database object which is not opened.
   `lmemb' specifies the number of members in each leaf page.  If it is not more than 0, the
   default value is specified.  The default value is 128.
   `nmemb' specifies the number of members in each non-leaf page.  If it is not more than 0, the
   default value is specified.  The default value is 256.
   `bnum' specifies the number of elements of the bucket array.  If it is not more than 0, the
   default value is specified.  The default value is 32749.  Suggested size of the bucket array
   is about from 1 to 4 times of the number of all pages to be stored.
   `apow' specifies the size of record alignment by power of 2.  If it is negative, the default
   value is specified.  The default value is 8 standing for 2^8=256.
   `fpow' specifies the maximum number of elements of the free block pool by power of 2.  If it
   is negative, the default value is specified.  The default value is 10 standing for 2^10=1024.
   `opts' specifies options by bitwise-or: `BDBTLARGE' specifies that the size of the database
   can be larger than 2GB by using 64-bit bucket array, `BDBTDEFLATE' specifies that each page
   is compressed with Deflate encoding, `BDBTBZIP' specifies that each page is compressed with
   BZIP2 encoding, `BDBTTCBS' specifies that each page is compressed with TCBS encoding.
   If successful, the return value is true, else, it is false.
   Note that the tuning parameters should be set before the database is opened. */
bool tcbdbtune(TCBDB *bdb, int32_t lmemb, int32_t nmemb,
               int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts);


/* Set the caching parameters of a B+ tree database object.
   `bdb' specifies the B+ tree database object which is not opened.
   `lcnum' specifies the maximum number of leaf nodes to be cached.  If it is not more than 0,
   the default value is specified.  The default value is 1024.
   `ncnum' specifies the maximum number of non-leaf nodes to be cached.  If it is not more than 0,
   the default value is specified.  The default value is 512.
   If successful, the return value is true, else, it is false.
   Note that the caching parameters should be set before the database is opened. */
bool tcbdbsetcache(TCBDB *bdb, int32_t lcnum, int32_t ncnum);


/* Set the size of the extra mapped memory of a B+ tree database object.
   `bdb' specifies the B+ tree database object which is not opened.
   `xmsiz' specifies the size of the extra mapped memory.  If it is not more than 0, the extra
   mapped memory is disabled.  It is disabled by default.
   If successful, the return value is true, else, it is false.
   Note that the mapping parameters should be set before the database is opened. */
bool tcbdbsetxmsiz(TCBDB *bdb, int64_t xmsiz);


/* Set the unit step number of auto defragmentation of a B+ tree database object.
   `bdb' specifies the B+ tree database object which is not opened.
   `dfunit' specifie the unit step number.  If it is not more than 0, the auto defragmentation
   is disabled.  It is disabled by default.
   If successful, the return value is true, else, it is false.
   Note that the defragmentation parameter should be set before the database is opened. */
bool tcbdbsetdfunit(TCBDB *bdb, int32_t dfunit);


/* Open a database file and connect a B+ tree database object.
   `bdb' specifies the B+ tree database object which is not opened.
   `path' specifies the path of the database file.
   `omode' specifies the connection mode: `BDBOWRITER' as a writer, `BDBOREADER' as a reader.
   If the mode is `BDBOWRITER', the following may be added by bitwise-or: `BDBOCREAT', which
   means it creates a new database if not exist, `BDBOTRUNC', which means it creates a new
   database regardless if one exists, `BDBOTSYNC', which means every transaction synchronizes
   updated contents with the device.  Both of `BDBOREADER' and `BDBOWRITER' can be added to by
   bitwise-or: `BDBONOLCK', which means it opens the database file without file locking, or
   `BDBOLCKNB', which means locking is performed without blocking.
   If successful, the return value is true, else, it is false. */
bool tcbdbopen(TCBDB *bdb, const char *path, int omode);


/* Close a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   If successful, the return value is true, else, it is false.
   Update of a database is assured to be written when the database is closed.  If a writer opens
   a database but does not close it appropriately, the database will be broken. */
bool tcbdbclose(TCBDB *bdb);


/* Store a record into a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcbdbput(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a string record into a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcbdbput2(TCBDB *bdb, const char *kstr, const char *vstr);


/* Store a new record into a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcbdbputkeep(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new string record into a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcbdbputkeep2(TCBDB *bdb, const char *kstr, const char *vstr);


/* Concatenate a value at the end of the existing record in a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcbdbputcat(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a string value at the end of the existing record in a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcbdbputcat2(TCBDB *bdb, const char *kstr, const char *vstr);


/* Store a record into a B+ tree database object with allowing duplication of keys.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, the new record is placed after the
   existing one. */
bool tcbdbputdup(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a string record into a B+ tree database object with allowing duplication of keys.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, the new record is placed after the
   existing one. */
bool tcbdbputdup2(TCBDB *bdb, const char *kstr, const char *vstr);


/* Store records into a B+ tree database object with allowing duplication of keys.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the common key.
   `ksiz' specifies the size of the region of the common key.
   `vals' specifies a list object containing values.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, the new records are placed after the
   existing one. */
bool tcbdbputdup3(TCBDB *bdb, const void *kbuf, int ksiz, const TCLIST *vals);


/* Remove a record of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false.
   If the key of duplicated records is specified, the first one is selected. */
bool tcbdbout(TCBDB *bdb, const void *kbuf, int ksiz);


/* Remove a string record of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kstr' specifies the string of the key.
   If successful, the return value is true, else, it is false.
   If the key of duplicated records is specified, the first one is selected. */
bool tcbdbout2(TCBDB *bdb, const char *kstr);


/* Remove records of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false.
   If the key of duplicated records is specified, all of them are removed. */
bool tcbdbout3(TCBDB *bdb, const void *kbuf, int ksiz);


/* Retrieve a record in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the corresponding
   record.  `NULL' is returned if no record corresponds.
   If the key of duplicated records is specified, the first one is selected.  Because an
   additional zero code is appended at the end of the region of the return value, the return
   value can be treated as a character string.  Because the region of the return value is
   allocated with the `malloc' call, it should be released with the `free' call when it is no
   longer in use. */
void *tcbdbget(TCBDB *bdb, const void *kbuf, int ksiz, int *sp);


/* Retrieve a string record in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `kstr' specifies the string of the key.
   If successful, the return value is the string of the value of the corresponding record.
   `NULL' is returned if no record corresponds.
   If the key of duplicated records is specified, the first one is selected.  Because the region
   of the return value is allocated with the `malloc' call, it should be released with the `free'
   call when it is no longer in use. */
char *tcbdbget2(TCBDB *bdb, const char *kstr);


/* Retrieve a record in a B+ tree database object as a volatile buffer.
   `bdb' specifies the B+ tree database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the corresponding
   record.  `NULL' is returned if no record corresponds.
   If the key of duplicated records is specified, the first one is selected.  Because an
   additional zero code is appended at the end of the region of the return value, the return
   value can be treated as a character string.  Because the region of the return value is
   volatile and it may be spoiled by another operation of the database, the data should be copied
   into another involatile buffer immediately. */
const void *tcbdbget3(TCBDB *bdb, const void *kbuf, int ksiz, int *sp);


/* Retrieve records in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is a list object of the values of the corresponding records.
   `NULL' is returned if no record corresponds.
   Because the object of the return value is created with the function `tclistnew', it should
   be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcbdbget4(TCBDB *bdb, const void *kbuf, int ksiz);


/* Get the number of records corresponding a key in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is the number of the corresponding records, else, it is 0. */
int tcbdbvnum(TCBDB *bdb, const void *kbuf, int ksiz);


/* Get the number of records corresponding a string key in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `kstr' specifies the string of the key.
   If successful, the return value is the number of the corresponding records, else, it is 0. */
int tcbdbvnum2(TCBDB *bdb, const char *kstr);


/* Get the size of the value of a record in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1.
   If the key of duplicated records is specified, the first one is selected. */
int tcbdbvsiz(TCBDB *bdb, const void *kbuf, int ksiz);


/* Get the size of the value of a string record in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `kstr' specifies the string of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1.
   If the key of duplicated records is specified, the first one is selected. */
int tcbdbvsiz2(TCBDB *bdb, const char *kstr);


/* Get keys of ranged records in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `bkbuf' specifies the pointer to the region of the key of the beginning border.  If it is
   `NULL', the first record is specified.
   `bksiz' specifies the size of the region of the beginning key.
   `binc' specifies whether the beginning border is inclusive or not.
   `ekbuf' specifies the pointer to the region of the key of the ending border.  If it is `NULL',
   the last record is specified.
   `eksiz' specifies the size of the region of the ending key.
   `einc' specifies whether the ending border is inclusive or not.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the keys of the corresponding records.  This function
   does never fail.  It returns an empty list even if no record corresponds.
   Because the object of the return value is created with the function `tclistnew', it should
   be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcbdbrange(TCBDB *bdb, const void *bkbuf, int bksiz, bool binc,
                   const void *ekbuf, int eksiz, bool einc, int max);


/* Get string keys of ranged records in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `bkstr' specifies the string of the key of the beginning border.  If it is `NULL', the first
   record is specified.
   `binc' specifies whether the beginning border is inclusive or not.
   `ekstr' specifies the string of the key of the ending border.  If it is `NULL', the last
   record is specified.
   `einc' specifies whether the ending border is inclusive or not.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the keys of the corresponding records.  This function
   does never fail.  It returns an empty list even if no record corresponds.
   Because the object of the return value is created with the function `tclistnew', it should
   be deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcbdbrange2(TCBDB *bdb, const char *bkstr, bool binc,
                    const char *ekstr, bool einc, int max);


/* Get forward matching keys in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `pbuf' specifies the pointer to the region of the prefix.
   `psiz' specifies the size of the region of the prefix.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding keys.  This function does never fail.
   It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcbdbfwmkeys(TCBDB *bdb, const void *pbuf, int psiz, int max);


/* Get forward matching string keys in a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `pstr' specifies the string of the prefix.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding keys.  This function does never fail.
   It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use. */
TCLIST *tcbdbfwmkeys2(TCBDB *bdb, const char *pstr, int max);


/* Add an integer to a record in a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is `INT_MIN'.
   If the corresponding record exists, the value is treated as an integer and is added to.  If no
   record corresponds, a new record of the additional value is stored. */
int tcbdbaddint(TCBDB *bdb, const void *kbuf, int ksiz, int num);


/* Add a real number to a record in a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is Not-a-Number.
   If the corresponding record exists, the value is treated as a real number and is added to.  If
   no record corresponds, a new record of the additional value is stored. */
double tcbdbadddouble(TCBDB *bdb, const void *kbuf, int ksiz, double num);


/* Synchronize updated contents of a B+ tree database object with the file and the device.
   `bdb' specifies the B+ tree database object connected as a writer.
   If successful, the return value is true, else, it is false.
   This function is useful when another process connects to the same database file. */
bool tcbdbsync(TCBDB *bdb);


/* Optimize the file of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `lmemb' specifies the number of members in each leaf page.  If it is not more than 0, the
   current setting is not changed.
   `nmemb' specifies the number of members in each non-leaf page.  If it is not more than 0, the
   current setting is not changed.
   `bnum' specifies the number of elements of the bucket array.  If it is not more than 0, the
   default value is specified.  The default value is two times of the number of pages.
   `apow' specifies the size of record alignment by power of 2.  If it is negative, the current
   setting is not changed.
   `fpow' specifies the maximum number of elements of the free block pool by power of 2.  If it
   is negative, the current setting is not changed.
   `opts' specifies options by bitwise-or: `BDBTLARGE' specifies that the size of the database
   can be larger than 2GB by using 64-bit bucket array, `BDBTDEFLATE' specifies that each record
   is compressed with Deflate encoding, `BDBTBZIP' specifies that each page is compressed with
   BZIP2 encoding, `BDBTTCBS' specifies that each page is compressed with TCBS encoding.  If it
   is `UINT8_MAX', the current setting is not changed.
   If successful, the return value is true, else, it is false.
   This function is useful to reduce the size of the database file with data fragmentation by
   successive updating. */
bool tcbdboptimize(TCBDB *bdb, int32_t lmemb, int32_t nmemb,
                   int64_t bnum, int8_t apow, int8_t fpow, uint8_t opts);


/* Remove all records of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   If successful, the return value is true, else, it is false. */
bool tcbdbvanish(TCBDB *bdb);


/* Copy the database file of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `path' specifies the path of the destination file.  If it begins with `@', the trailing
   substring is executed as a command line.
   If successful, the return value is true, else, it is false.  False is returned if the executed
   command returns non-zero code.
   The database file is assured to be kept synchronized and not modified while the copying or
   executing operation is in progress.  So, this function is useful to create a backup file of
   the database file. */
bool tcbdbcopy(TCBDB *bdb, const char *path);


/* Begin the transaction of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   If successful, the return value is true, else, it is false.
   The database is locked by the thread while the transaction so that only one transaction can be
   activated with a database object at the same time.  Thus, the serializable isolation level is
   assumed if every database operation is performed in the transaction.  Because all pages are
   cached on memory while the transaction, the amount of referred records is limited by the
   memory capacity.  If the database is closed during transaction, the transaction is aborted
   implicitly. */
bool tcbdbtranbegin(TCBDB *bdb);


/* Commit the transaction of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   If successful, the return value is true, else, it is false.
   Update in the transaction is fixed when it is committed successfully. */
bool tcbdbtrancommit(TCBDB *bdb);


/* Abort the transaction of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   If successful, the return value is true, else, it is false.
   Update in the transaction is discarded when it is aborted.  The state of the database is
   rollbacked to before transaction. */
bool tcbdbtranabort(TCBDB *bdb);


/* Get the file path of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the path of the database file or `NULL' if the object does not connect to
   any database file. */
const char *tcbdbpath(TCBDB *bdb);


/* Get the number of records of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the number of records or 0 if the object does not connect to any database
   file. */
uint64_t tcbdbrnum(TCBDB *bdb);


/* Get the size of the database file of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the size of the database file or 0 if the object does not connect to any
   database file. */
uint64_t tcbdbfsiz(TCBDB *bdb);


/* Create a cursor object.
   `bdb' specifies the B+ tree database object.
   The return value is the new cursor object.
   Note that the cursor is available only after initialization with the `tcbdbcurfirst' or the
   `tcbdbcurjump' functions and so on.  Moreover, the position of the cursor will be indefinite
   when the database is updated after the initialization of the cursor. */
BDBCUR *tcbdbcurnew(TCBDB *bdb);


/* Delete a cursor object.
   `cur' specifies the cursor object. */
void tcbdbcurdel(BDBCUR *cur);


/* Move a cursor object to the first record.
   `cur' specifies the cursor object.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record in the database. */
bool tcbdbcurfirst(BDBCUR *cur);


/* Move a cursor object to the last record.
   `cur' specifies the cursor object.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record in the database. */
bool tcbdbcurlast(BDBCUR *cur);


/* Move a cursor object to the front of records corresponding a key.
   `cur' specifies the cursor object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record corresponding the condition.
   The cursor is set to the first record corresponding the key or the next substitute if
   completely matching record does not exist. */
bool tcbdbcurjump(BDBCUR *cur, const void *kbuf, int ksiz);


/* Move a cursor object to the front of records corresponding a key string.
   `cur' specifies the cursor object.
   `kstr' specifies the string of the key.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record corresponding the condition.
   The cursor is set to the first record corresponding the key or the next substitute if
   completely matching record does not exist. */
bool tcbdbcurjump2(BDBCUR *cur, const char *kstr);


/* Move a cursor object to the previous record.
   `cur' specifies the cursor object.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no previous record. */
bool tcbdbcurprev(BDBCUR *cur);


/* Move a cursor object to the next record.
   `cur' specifies the cursor object.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no next record. */
bool tcbdbcurnext(BDBCUR *cur);


/* Insert a record around a cursor object.
   `cur' specifies the cursor object of writer connection.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   `cpmode' specifies detail adjustment: `BDBCPCURRENT', which means that the value of the
   current record is overwritten, `BDBCPBEFORE', which means that the new record is inserted
   before the current record, `BDBCPAFTER', which means that the new record is inserted after the
   current record.
   If successful, the return value is true, else, it is false.  False is returned when the cursor
   is at invalid position.
   After insertion, the cursor is moved to the inserted record. */
bool tcbdbcurput(BDBCUR *cur, const void *vbuf, int vsiz, int cpmode);


/* Insert a string record around a cursor object.
   `cur' specifies the cursor object of writer connection.
   `vstr' specifies the string of the value.
   `cpmode' specifies detail adjustment: `BDBCPCURRENT', which means that the value of the
   current record is overwritten, `BDBCPBEFORE', which means that the new record is inserted
   before the current record, `BDBCPAFTER', which means that the new record is inserted after the
   current record.
   If successful, the return value is true, else, it is false.  False is returned when the cursor
   is at invalid position.
   After insertion, the cursor is moved to the inserted record. */
bool tcbdbcurput2(BDBCUR *cur, const char *vstr, int cpmode);


/* Remove the record where a cursor object is.
   `cur' specifies the cursor object of writer connection.
   If successful, the return value is true, else, it is false.  False is returned when the cursor
   is at invalid position.
   After deletion, the cursor is moved to the next record if possible. */
bool tcbdbcurout(BDBCUR *cur);


/* Get the key of the record where the cursor object is.
   `cur' specifies the cursor object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the key, else, it is `NULL'.
   `NULL' is returned when the cursor is at invalid position.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcbdbcurkey(BDBCUR *cur, int *sp);


/* Get the key string of the record where the cursor object is.
   `cur' specifies the cursor object.
   If successful, the return value is the string of the key, else, it is `NULL'.  `NULL' is
   returned when the cursor is at invalid position.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcbdbcurkey2(BDBCUR *cur);


/* Get the key of the record where the cursor object is, as a volatile buffer.
   `cur' specifies the cursor object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the key, else, it is `NULL'.
   `NULL' is returned when the cursor is at invalid position.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return value
   is volatile and it may be spoiled by another operation of the database, the data should be
   copied into another involatile buffer immediately. */
const void *tcbdbcurkey3(BDBCUR *cur, int *sp);


/* Get the value of the record where the cursor object is.
   `cur' specifies the cursor object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value, else, it is `NULL'.
   `NULL' is returned when the cursor is at invalid position.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcbdbcurval(BDBCUR *cur, int *sp);


/* Get the value string of the record where the cursor object is.
   `cur' specifies the cursor object.
   If successful, the return value is the string of the value, else, it is `NULL'.  `NULL' is
   returned when the cursor is at invalid position.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
char *tcbdbcurval2(BDBCUR *cur);


/* Get the value of the record where the cursor object is, as a volatile buffer.
   `cur' specifies the cursor object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value, else, it is `NULL'.
   `NULL' is returned when the cursor is at invalid position.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return value
   is volatile and it may be spoiled by another operation of the database, the data should be
   copied into another involatile buffer immediately. */
const void *tcbdbcurval3(BDBCUR *cur, int *sp);


/* Get the key and the value of the record where the cursor object is.
   `cur' specifies the cursor object.
   `kxstr' specifies the object into which the key is wrote down.
   `vxstr' specifies the object into which the value is wrote down.
   If successful, the return value is true, else, it is false.  False is returned when the cursor
   is at invalid position. */
bool tcbdbcurrec(BDBCUR *cur, TCXSTR *kxstr, TCXSTR *vxstr);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


/* Set the error code of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `ecode' specifies the error code.
   `file' specifies the file name of the code.
   `line' specifies the line number of the code.
   `func' specifies the function name of the code. */
void tcbdbsetecode(TCBDB *bdb, int ecode, const char *filename, int line, const char *func);


/* Set the file descriptor for debugging output.
   `bdb' specifies the B+ tree database object.
   `fd' specifies the file descriptor for debugging output. */
void tcbdbsetdbgfd(TCBDB *bdb, int fd);


/* Get the file descriptor for debugging output.
   `bdb' specifies the B+ tree database object.
   The return value is the file descriptor for debugging output. */
int tcbdbdbgfd(TCBDB *bdb);


/* Check whether mutual exclusion control is set to a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   If mutual exclusion control is set, it is true, else it is false. */
bool tcbdbhasmutex(TCBDB *bdb);


/* Synchronize updating contents on memory of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `phys' specifies whether to synchronize physically.
   If successful, the return value is true, else, it is false. */
bool tcbdbmemsync(TCBDB *bdb, bool phys);


/* Clear the cache of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   If successful, the return value is true, else, it is false. */
bool tcbdbcacheclear(TCBDB *bdb);


/* Get the comparison function of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the pointer to the comparison function. */
TCCMP tcbdbcmpfunc(TCBDB *bdb);


/* Get the opaque object for the comparison function of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the opaque object for the comparison function. */
void *tcbdbcmpop(TCBDB *bdb);


/* Get the maximum number of cached leaf nodes of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the maximum number of cached leaf nodes. */
uint32_t tcbdblmemb(TCBDB *bdb);


/* Get the maximum number of cached non-leaf nodes of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the maximum number of cached non-leaf nodes. */
uint32_t tcbdbnmemb(TCBDB *bdb);


/* Get the number of the leaf nodes of B+ tree database object.
   `bdb' specifies the B+ tree database object.
   If successful, the return value is the number of the leaf nodes or 0 if the object does not
   connect to any database file. */
uint64_t tcbdblnum(TCBDB *bdb);


/* Get the number of the non-leaf nodes of B+ tree database object.
   `bdb' specifies the B+ tree database object.
   If successful, the return value is the number of the non-leaf nodes or 0 if the object does
   not connect to any database file. */
uint64_t tcbdbnnum(TCBDB *bdb);


/* Get the number of elements of the bucket array of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the number of elements of the bucket array or 0 if the object does not
   connect to any database file. */
uint64_t tcbdbbnum(TCBDB *bdb);


/* Get the record alignment of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the record alignment or 0 if the object does not connect to any database
   file. */
uint32_t tcbdbalign(TCBDB *bdb);


/* Get the maximum number of the free block pool of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the maximum number of the free block pool or 0 if the object does not
   connect to any database file. */
uint32_t tcbdbfbpmax(TCBDB *bdb);


/* Get the inode number of the database file of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the inode number of the database file or 0 if the object does not connect
   to any database file. */
uint64_t tcbdbinode(TCBDB *bdb);


/* Get the modification time of the database file of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the inode number of the database file or 0 if the object does not connect
   to any database file. */
time_t tcbdbmtime(TCBDB *bdb);


/* Get the additional flags of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the additional flags. */
uint8_t tcbdbflags(TCBDB *bdb);


/* Get the options of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the options. */
uint8_t tcbdbopts(TCBDB *bdb);


/* Get the pointer to the opaque field of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the pointer to the opaque field whose size is 128 bytes. */
char *tcbdbopaque(TCBDB *bdb);


/* Get the number of used elements of the bucket array of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the number of used elements of the bucket array or 0 if the object does not
   connect to any database file. */
uint64_t tcbdbbnumused(TCBDB *bdb);


/* Set the maximum size of each leaf node.
   `bdb' specifies the B+ tree database object which is not opened.
   `lsmax' specifies the maximum size of each leaf node.  If it is not more than 0, the default
   value is specified.  The default value is 16386.
   If successful, the return value is true, else, it is false.
   Note that the tuning parameters of the database should be set before the database is opened. */
bool tcbdbsetlsmax(TCBDB *bdb, uint32_t lsmax);


/* Set the capacity number of records.
   `bdb' specifies the B+ tree database object which is not opened.
   `capnum' specifies the capacity number of records.  If it is not more than 0, the capacity is
   unlimited.
   If successful, the return value is true, else, it is false.
   When the number of records exceeds the capacity, forehand records are removed implicitly.
   Note that the tuning parameters of the database should be set before the database is opened. */
bool tcbdbsetcapnum(TCBDB *bdb, uint64_t capnum);


/* Set the custom codec functions of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `enc' specifies the pointer to the custom encoding function.  It receives four parameters.
   The first parameter is the pointer to the region.  The second parameter is the size of the
   region.  The third parameter is the pointer to the variable into which the size of the region
   of the return value is assigned.  The fourth parameter is the pointer to the optional opaque
   object.  It returns the pointer to the result object allocated with `malloc' call if
   successful, else, it returns `NULL'.
   `encop' specifies an arbitrary pointer to be given as a parameter of the encoding function.
   If it is not needed, `NULL' can be specified.
   `dec' specifies the pointer to the custom decoding function.
   `decop' specifies an arbitrary pointer to be given as a parameter of the decoding function.
   If it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false.
   Note that the custom codec functions should be set before the database is opened and should be
   set every time the database is being opened. */
bool tcbdbsetcodecfunc(TCBDB *bdb, TCCODEC enc, void *encop, TCCODEC dec, void *decop);


/* Get the unit step number of auto defragmentation of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   The return value is the unit step number of auto defragmentation. */
uint32_t tcbdbdfunit(TCBDB *bdb);


/* Perform dynamic defragmentation of a B+ tree database object.
   `bdb' specifies the B+ tree database object connected as a writer.
   `step' specifie the number of steps.  If it is not more than 0, the whole file is defragmented
   gradually without keeping a continuous lock.
   If successful, the return value is true, else, it is false. */
bool tcbdbdefrag(TCBDB *bdb, int64_t step);


/* Store a new record into a B+ tree database object with backward duplication.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, the new record is placed after the
   existing one. */
bool tcbdbputdupback(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new string record into a B+ tree database object with backward duplication.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kstr' specifies the string of the key.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, the new record is placed after the
   existing one. */
bool tcbdbputdupback2(TCBDB *bdb, const char *kstr, const char *vstr);


/* Store a record into a B+ tree database object with a duplication handler.
   `bdb' specifies the B+ tree database object connected as a writer.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.  `NULL' means that record addition is
   ommited if there is no corresponding record.
   `vsiz' specifies the size of the region of the value.
   `proc' specifies the pointer to the callback function to process duplication.  It receives
   four parameters.  The first parameter is the pointer to the region of the value.  The second
   parameter is the size of the region of the value.  The third parameter is the pointer to the
   variable into which the size of the region of the return value is assigned.  The fourth
   parameter is the pointer to the optional opaque object.  It returns the pointer to the result
   object allocated with `malloc'.  It is released by the caller.  If it is `NULL', the record is
   not modified.  If it is `(void *)-1', the record is removed.
   `op' specifies an arbitrary pointer to be given as a parameter of the callback function.  If
   it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false.
   Note that the callback function can not perform any database operation because the function
   is called in the critical section guarded by the same locks of database operations. */
bool tcbdbputproc(TCBDB *bdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz,
                  TCPDPROC proc, void *op);


/* Move a cursor object to the rear of records corresponding a key.
   `cur' specifies the cursor object.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record corresponding the condition.
   The cursor is set to the last record corresponding the key or the previous substitute if
   completely matching record does not exist. */
bool tcbdbcurjumpback(BDBCUR *cur, const void *kbuf, int ksiz);


/* Move a cursor object to the rear of records corresponding a key string.
   `cur' specifies the cursor object.
   `kstr' specifies the string of the key.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record corresponding the condition.
   The cursor is set to the last record corresponding the key or the previous substitute if
   completely matching record does not exist. */
bool tcbdbcurjumpback2(BDBCUR *cur, const char *kstr);


/* Process each record atomically of a B+ tree database object.
   `bdb' specifies the B+ tree database object.
   `iter' specifies the pointer to the iterator function called for each record.  It receives
   five parameters.  The first parameter is the pointer to the region of the key.  The second
   parameter is the size of the region of the key.  The third parameter is the pointer to the
   region of the value.  The fourth parameter is the size of the region of the value.  The fifth
   parameter is the pointer to the optional opaque object.  It returns true to continue iteration
   or false to stop iteration.
   `op' specifies an arbitrary pointer to be given as a parameter of the iterator function.  If
   it is not needed, `NULL' can be specified.
   If successful, the return value is true, else, it is false.
   Note that the callback function can not perform any database operation because the function
   is called in the critical section guarded by the same locks of database operations. */
bool tcbdbforeach(TCBDB *bdb, TCITER iter, void *op);



__TCBDB_CLINKAGEEND
#endif                                   /* duplication check */


/* END OF FILE */
