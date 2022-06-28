/*************************************************************************************************
 * The fixed-length database API of Tokyo Cabinet
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


#ifndef _TCFDB_H                         /* duplication check */
#define _TCFDB_H

#if defined(__cplusplus)
#define __TCFDB_CLINKAGEBEGIN extern "C" {
#define __TCFDB_CLINKAGEEND }
#else
#define __TCFDB_CLINKAGEBEGIN
#define __TCFDB_CLINKAGEEND
#endif
__TCFDB_CLINKAGEBEGIN


#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <tcutil.h>



/*************************************************************************************************
 * API
 *************************************************************************************************/


typedef struct {                         /* type of structure for a fixed-length database */
  void *mmtx;                            /* mutex for method */
  void *amtx;                            /* mutex for attribute */
  void *rmtxs;                           /* mutexes for records */
  void *tmtx;                            /* mutex for transaction */
  void *wmtx;                            /* mutex for write ahead logging */
  void *eckey;                           /* key for thread specific error code */
  char *rpath;                           /* real path for locking */
  uint8_t type;                          /* database type */
  uint8_t flags;                         /* additional flags */
  uint32_t width;                        /* width of the value of each record */
  uint64_t limsiz;                       /* limit size of the file */
  int wsiz;                              /* size of the width region */
  int rsiz;                              /* size of each record */
  uint64_t limid;                        /* limit ID number */
  char *path;                            /* path of the database file */
  int fd;                                /* file descriptor of the database file */
  uint32_t omode;                        /* open mode */
  uint64_t rnum;                         /* number of the records */
  uint64_t fsiz;                         /* size of the database file */
  uint64_t min;                          /* minimum ID number */
  uint64_t max;                          /* maximum ID number */
  uint64_t iter;                         /* ID number of the iterator */
  char *map;                             /* pointer to the mapped memory */
  unsigned char *array;                  /* pointer to the array region */
  int ecode;                             /* last happened error code */
  bool fatal;                            /* whether a fatal error occured */
  uint64_t inode;                        /* inode number */
  time_t mtime;                          /* modification time */
  bool tran;                             /* whether in the transaction */
  int walfd;                             /* file descriptor of write ahead logging */
  uint64_t walend;                       /* end offset of write ahead logging */
  int dbgfd;                             /* file descriptor for debugging */
  int64_t cnt_writerec;                  /* tesing counter for record write times */
  int64_t cnt_readrec;                   /* tesing counter for record read times */
  int64_t cnt_truncfile;                 /* tesing counter for file truncate times */
} TCFDB;

enum {                                   /* enumeration for additional flags */
  FDBFOPEN = 1 << 0,                     /* whether opened */
  FDBFFATAL = 1 << 1                     /* whetehr with fatal error */
};

enum {                                   /* enumeration for open modes */
  FDBOREADER = 1 << 0,                   /* open as a reader */
  FDBOWRITER = 1 << 1,                   /* open as a writer */
  FDBOCREAT = 1 << 2,                    /* writer creating */
  FDBOTRUNC = 1 << 3,                    /* writer truncating */
  FDBONOLCK = 1 << 4,                    /* open without locking */
  FDBOLCKNB = 1 << 5,                    /* lock without blocking */
  FDBOTSYNC = 1 << 6                     /* synchronize every transaction */
};

enum {                                   /* enumeration for ID constants */
  FDBIDMIN = -1,                         /* minimum number */
  FDBIDPREV = -2,                        /* less by one than the minimum */
  FDBIDMAX = -3,                         /* maximum number */
  FDBIDNEXT = -4                         /* greater by one than the miximum */
};


/* Get the message string corresponding to an error code.
   `ecode' specifies the error code.
   The return value is the message string of the error code. */
const char *tcfdberrmsg(int ecode);


/* Create a fixed-length database object.
   The return value is the new fixed-length database object. */
TCFDB *tcfdbnew(void);


/* Delete a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   If the database is not closed, it is closed implicitly.  Note that the deleted object and its
   derivatives can not be used anymore. */
void tcfdbdel(TCFDB *fdb);


/* Get the last happened error code of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
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
int tcfdbecode(TCFDB *fdb);


/* Set mutual exclusion control of a fixed-length database object for threading.
   `fdb' specifies the fixed-length database object which is not opened.
   If successful, the return value is true, else, it is false.
   Note that the mutual exclusion control is needed if the object is shared by plural threads and
   this function should should be called before the database is opened. */
bool tcfdbsetmutex(TCFDB *fdb);


/* Set the tuning parameters of a fixed-length database object.
   `fdb' specifies the fixed-length database object which is not opened.
   `width' specifies the width of the value of each record.  If it is not more than 0, the
   default value is specified.  The default value is 255.
   `limsiz' specifies the limit size of the database file.  If it is not more than 0, the default
   value is specified.  The default value is 268435456.
   If successful, the return value is true, else, it is false.
   Note that the tuning parameters should be set before the database is opened. */
bool tcfdbtune(TCFDB *fdb, int32_t width, int64_t limsiz);


/* Open a database file and connect a fixed-length database object.
   `fdb' specifies the fixed-length database object which is not opened.
   `path' specifies the path of the database file.
   `omode' specifies the connection mode: `FDBOWRITER' as a writer, `FDBOREADER' as a reader.
   If the mode is `FDBOWRITER', the following may be added by bitwise-or: `FDBOCREAT', which
   means it creates a new database if not exist, `FDBOTRUNC', which means it creates a new
   database regardless if one exists, `FDBOTSYNC', which means every transaction synchronizes
   updated contents with the device.  Both of `FDBOREADER' and `FDBOWRITER' can be added to by
   bitwise-or: `FDBONOLCK', which means it opens the database file without file locking, or
   `FDBOLCKNB', which means locking is performed without blocking.
   If successful, the return value is true, else, it is false. */
bool tcfdbopen(TCFDB *fdb, const char *path, int omode);


/* Close a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   If successful, the return value is true, else, it is false.
   Update of a database is assured to be written when the database is closed.  If a writer opens
   a database but does not close it appropriately, the database will be broken. */
bool tcfdbclose(TCFDB *fdb);


/* Store a record into a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDPREV', the number less by one than
   the minimum ID number of existing records is specified.  If it is `FDBIDMAX', the maximum ID
   number of existing records is specified.  If it is `FDBIDNEXT', the number greater by one than
   the maximum ID number of existing records is specified.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.  If the size of the value is greater
   than the width tuning parameter of the database, the size is cut down to the width.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcfdbput(TCFDB *fdb, int64_t id, const void *vbuf, int vsiz);


/* Store a record with a decimal key into a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kbuf' specifies the pointer to the region of the decimal key.  It should be more than 0.  If
   it is "min", the minimum ID number of existing records is specified.  If it is "prev", the
   number less by one than the minimum ID number of existing records is specified.  If it is
   "max", the maximum ID number of existing records is specified.  If it is "next", the number
   greater by one than the maximum ID number of existing records is specified.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.  If the size of the value is greater
   than the width tuning parameter of the database, the size is cut down to the width.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcfdbput2(TCFDB *fdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a string record with a decimal key into a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kstr' specifies the string of the decimal key.  It should be more than 0.  If it is "min",
   the minimum ID number of existing records is specified.  If it is "prev", the number less by
   one than the minimum ID number of existing records is specified.  If it is "max", the maximum
   ID number of existing records is specified.  If it is "next", the number greater by one than
   the maximum ID number of existing records is specified.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, it is overwritten. */
bool tcfdbput3(TCFDB *fdb, const char *kstr, const void *vstr);


/* Store a new record into a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDPREV', the number less by one than
   the minimum ID number of existing records is specified.  If it is `FDBIDMAX', the maximum ID
   number of existing records is specified.  If it is `FDBIDNEXT', the number greater by one than
   the maximum ID number of existing records is specified.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.  If the size of the value is greater
   than the width tuning parameter of the database, the size is cut down to the width.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcfdbputkeep(TCFDB *fdb, int64_t id, const void *vbuf, int vsiz);


/* Store a new record with a decimal key into a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kbuf' specifies the pointer to the region of the decimal key.  It should be more than 0.  If
   it is "min", the minimum ID number of existing records is specified.  If it is "prev", the
   number less by one than the minimum ID number of existing records is specified.  If it is
   "max", the maximum ID number of existing records is specified.  If it is "next", the number
   greater by one than the maximum ID number of existing records is specified.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.  If the size of the value is greater
   than the width tuning parameter of the database, the size is cut down to the width.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcfdbputkeep2(TCFDB *fdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Store a new string record with a decimal key into a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kstr' specifies the string of the decimal key.  It should be more than 0.  If it is "min",
   the minimum ID number of existing records is specified.  If it is "prev", the number less by
   one than the minimum ID number of existing records is specified.  If it is "max", the maximum
   ID number of existing records is specified.  If it is "next", the number greater by one than
   the maximum ID number of existing records is specified.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If a record with the same key exists in the database, this function has no effect. */
bool tcfdbputkeep3(TCFDB *fdb, const char *kstr, const void *vstr);


/* Concatenate a value at the end of the existing record in a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDPREV', the number less by one than
   the minimum ID number of existing records is specified.  If it is `FDBIDMAX', the maximum ID
   number of existing records is specified.  If it is `FDBIDNEXT', the number greater by one than
   the maximum ID number of existing records is specified.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.  If the size of the value is greater
   than the width tuning parameter of the database, the size is cut down to the width.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcfdbputcat(TCFDB *fdb, int64_t id, const void *vbuf, int vsiz);


/* Concatenate a value with a decimal key in a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kbuf' specifies the pointer to the region of the decimal key.  It should be more than 0.  If
   it is "min", the minimum ID number of existing records is specified.  If it is "prev", the
   number less by one than the minimum ID number of existing records is specified.  If it is
   "max", the maximum ID number of existing records is specified.  If it is "next", the number
   greater by one than the maximum ID number of existing records is specified.
   `ksiz' specifies the size of the region of the key.
   `vbuf' specifies the pointer to the region of the value.
   `vsiz' specifies the size of the region of the value.  If the size of the value is greater
   than the width tuning parameter of the database, the size is cut down to the width.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcfdbputcat2(TCFDB *fdb, const void *kbuf, int ksiz, const void *vbuf, int vsiz);


/* Concatenate a string value with a decimal key in a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kstr' specifies the string of the decimal key.  It should be more than 0.  If it is "min",
   the minimum ID number of existing records is specified.  If it is "prev", the number less by
   one than the minimum ID number of existing records is specified.  If it is "max", the maximum
   ID number of existing records is specified.  If it is "next", the number greater by one than
   the maximum ID number of existing records is specified.
   `vstr' specifies the string of the value.
   If successful, the return value is true, else, it is false.
   If there is no corresponding record, a new record is created. */
bool tcfdbputcat3(TCFDB *fdb, const char *kstr, const void *vstr);


/* Remove a record of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDMAX', the maximum ID number of
   existing records is specified.
   If successful, the return value is true, else, it is false. */
bool tcfdbout(TCFDB *fdb, int64_t id);


/* Remove a record with a decimal key of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kbuf' specifies the pointer to the region of the decimal key.  It should be more than 0.  If
   it is "min", the minimum ID number of existing records is specified.  If it is "max", the
   maximum ID number of existing records is specified.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false. */
bool tcfdbout2(TCFDB *fdb, const void *kbuf, int ksiz);


/* Remove a string record with a decimal key of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `kstr' specifies the string of the decimal key.  It should be more than 0.  If it is "min",
   the minimum ID number of existing records is specified.  If it is "max", the maximum ID number
   of existing records is specified.
   If successful, the return value is true, else, it is false. */
bool tcfdbout3(TCFDB *fdb, const char *kstr);


/* Retrieve a record in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDMAX', the maximum ID number of
   existing records is specified.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the corresponding
   record.  `NULL' is returned if no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcfdbget(TCFDB *fdb, int64_t id, int *sp);


/* Retrieve a record with a decimal key in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `kbuf' specifies the pointer to the region of the decimal key.  It should be more than 0.  If
   it is "min", the minimum ID number of existing records is specified.  If it is "max", the
   maximum ID number of existing records is specified.
   `ksiz' specifies the size of the region of the key.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the value of the corresponding
   record.  `NULL' is returned if no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
void *tcfdbget2(TCFDB *fdb, const void *kbuf, int ksiz, int *sp);


/* Retrieve a string record with a decimal key in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `kstr' specifies the string of the decimal key.  It should be more than 0.  If it is "min",
   the minimum ID number of existing records is specified.  If it is "max", the maximum ID number
   of existing records is specified.
   If successful, the return value is the string of the value of the corresponding record.
   `NULL' is returned if no record corresponds.
   Because an additional zero code is appended at the end of the region of the return value,
   the return value can be treated as a character string.  Because the region of the return
   value is allocated with the `malloc' call, it should be released with the `free' call when
   it is no longer in use. */
char *tcfdbget3(TCFDB *fdb, const char *kstr);


/* Retrieve a record in a fixed-length database object and write the value into a buffer.
   `fdb' specifies the fixed-length database object.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDMAX', the maximum ID number of
   existing records is specified.
   `vbuf' specifies the pointer to the buffer into which the value of the corresponding record is
   written.
   `max' specifies the size of the buffer.
   If successful, the return value is the size of the written data, else, it is -1.  -1 is
   returned if no record corresponds to the specified key.
   Note that an additional zero code is not appended at the end of the region of the writing
   buffer. */
int tcfdbget4(TCFDB *fdb, int64_t id, void *vbuf, int max);


/* Get the size of the value of a record in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDMAX', the maximum ID number of
   existing records is specified.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
int tcfdbvsiz(TCFDB *fdb, int64_t id);


/* Get the size of the value with a decimal key in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `kbuf' specifies the pointer to the region of the decimal key.  It should be more than 0.  If
   it is "min", the minimum ID number of existing records is specified.  If it is "max", the
   maximum ID number of existing records is specified.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
int tcfdbvsiz2(TCFDB *fdb, const void *kbuf, int ksiz);


/* Get the size of the string value with a decimal key in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `kstr' specifies the string of the decimal key.  It should be more than 0.  If it is "min",
   the minimum ID number of existing records is specified.  If it is "max", the maximum ID number
   of existing records is specified.
   If successful, the return value is the size of the value of the corresponding record, else,
   it is -1. */
int tcfdbvsiz3(TCFDB *fdb, const char *kstr);


/* Initialize the iterator of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   If successful, the return value is true, else, it is false.
   The iterator is used in order to access the key of every record stored in a database. */
bool tcfdbiterinit(TCFDB *fdb);


/* Get the next ID number of the iterator of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   If successful, the return value is the next ID number of the iterator, else, it is 0.  0 is
   returned when no record is to be get out of the iterator.
   It is possible to access every record by iteration of calling this function.  It is allowed to
   update or remove records whose keys are fetched while the iteration.  The order of this
   traversal access method is ascending of the ID number. */
uint64_t tcfdbiternext(TCFDB *fdb);


/* Get the next decimay key of the iterator of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `sp' specifies the pointer to the variable into which the size of the region of the return
   value is assigned.
   If successful, the return value is the pointer to the region of the next decimal key, else, it
   is `NULL'.  `NULL' is returned when no record is to be get out of the iterator.
   Because an additional zero code is appended at the end of the region of the return value, the
   return value can be treated as a character string.  Because the region of the return value is
   allocated with the `malloc' call, it should be released with the `free' call when it is no
   longer in use.  It is possible to access every record by iteration of calling this function.
   It is allowed to update or remove records whose keys are fetched while the iteration.  The
   order of this traversal access method is ascending of the ID number. */
void *tcfdbiternext2(TCFDB *fdb, int *sp);


/* Get the next decimay key string of the iterator of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   If successful, the return value is the string of the next decimal key, else, it is `NULL'.
   `NULL' is returned when no record is to be get out of the iterator.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use.  It is possible to access every
   record by iteration of calling this function.  It is allowed to update or remove records whose
   keys are fetched while the iteration.  The order of this traversal access method is ascending
   of the ID number. */
char *tcfdbiternext3(TCFDB *fdb);


/* Get range matching ID numbers in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `lower' specifies the lower limit of the range.  If it is `FDBIDMIN', the minimum ID is
   specified.
   `upper' specifies the upper limit of the range.  If it is `FDBIDMAX', the maximum ID is
   specified.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   `np' specifies the pointer to the variable into which the number of elements of the return
   value is assigned.
   If successful, the return value is the pointer to an array of ID numbers of the corresponding
   records.  `NULL' is returned on failure.  This function does never fail.  It returns an empty
   array even if no key corresponds.
   Because the region of the return value is allocated with the `malloc' call, it should be
   released with the `free' call when it is no longer in use. */
uint64_t *tcfdbrange(TCFDB *fdb, int64_t lower, int64_t upper, int max, int *np);


/* Get range matching decimal keys in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `lbuf' specifies the pointer to the region of the lower key.  If it is "min", the minimum ID
   number of existing records is specified.
   `lsiz' specifies the size of the region of the lower key.
   `ubuf' specifies the pointer to the region of the upper key.  If it is "max", the maximum ID
   number of existing records is specified.
   `usiz' specifies the size of the region of the upper key.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding decimal keys.  This function does never
   fail.  It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use.  Note that this function
   may be very slow because every key in the database is scanned. */
TCLIST *tcfdbrange2(TCFDB *fdb, const void *lbuf, int lsiz, const void *ubuf, int usiz, int max);


/* Get range matching decimal keys with strings in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `lstr' specifies the string of the lower key.  If it is "min", the minimum ID number of
   existing records is specified.
   `ustr' specifies the string of the upper key.  If it is "max", the maximum ID number of
   existing records is specified.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding decimal keys.  This function does never
   fail.  It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use.  Note that this function
   may be very slow because every key in the database is scanned. */
TCLIST *tcfdbrange3(TCFDB *fdb, const char *lstr, const char *ustr, int max);


/* Get keys with an interval notation in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `ibuf' specifies the pointer to the region of the interval notation.
   `isiz' specifies the size of the region of the interval notation.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding decimal keys.  This function does never
   fail.  It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use.  Note that this function
   may be very slow because every key in the database is scanned. */
TCLIST *tcfdbrange4(TCFDB *fdb, const void *ibuf, int isiz, int max);


/* Get keys with an interval notation string in a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `istr' specifies the pointer to the region of the interval notation string.
   `max' specifies the maximum number of keys to be fetched.  If it is negative, no limit is
   specified.
   The return value is a list object of the corresponding decimal keys.  This function does never
   fail.  It returns an empty list even if no key corresponds.
   Because the object of the return value is created with the function `tclistnew', it should be
   deleted with the function `tclistdel' when it is no longer in use.  Note that this function
   may be very slow because every key in the database is scanned. */
TCLIST *tcfdbrange5(TCFDB *fdb, const void *istr, int max);


/* Add an integer to a record in a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDPREV', the number less by one than
   the minimum ID number of existing records is specified.  If it is `FDBIDMAX', the maximum ID
   number of existing records is specified.  If it is `FDBIDNEXT', the number greater by one than
   the maximum ID number of existing records is specified.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is `INT_MIN'.
   If the corresponding record exists, the value is treated as an integer and is added to.  If no
   record corresponds, a new record of the additional value is stored. */
int tcfdbaddint(TCFDB *fdb, int64_t id, int num);


/* Add a real number to a record in a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDPREV', the number less by one than
   the minimum ID number of existing records is specified.  If it is `FDBIDMAX', the maximum ID
   number of existing records is specified.  If it is `FDBIDNEXT', the number greater by one than
   the maximum ID number of existing records is specified.
   `num' specifies the additional value.
   If successful, the return value is the summation value, else, it is Not-a-Number.
   If the corresponding record exists, the value is treated as a real number and is added to.  If
   no record corresponds, a new record of the additional value is stored. */
double tcfdbadddouble(TCFDB *fdb, int64_t id, double num);


/* Synchronize updated contents of a fixed-length database object with the file and the device.
   `fdb' specifies the fixed-length database object connected as a writer.
   If successful, the return value is true, else, it is false.
   This function is useful when another process connects to the same database file. */
bool tcfdbsync(TCFDB *fdb);


/* Optimize the file of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `width' specifies the width of the value of each record.  If it is not more than 0, the current
   setting is not changed.
   `limsiz' specifies the limit size of the database file.  If it is not more than 0, the current
   setting is not changed.
   If successful, the return value is true, else, it is false. */
bool tcfdboptimize(TCFDB *fdb, int32_t width, int64_t limsiz);


/* Remove all records of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   If successful, the return value is true, else, it is false. */
bool tcfdbvanish(TCFDB *fdb);


/* Copy the database file of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `path' specifies the path of the destination file.  If it begins with `@', the trailing
   substring is executed as a command line.
   If successful, the return value is true, else, it is false.  False is returned if the executed
   command returns non-zero code.
   The database file is assured to be kept synchronized and not modified while the copying or
   executing operation is in progress.  So, this function is useful to create a backup file of
   the database file. */
bool tcfdbcopy(TCFDB *fdb, const char *path);


/* Begin the transaction of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   If successful, the return value is true, else, it is false.
   The database is locked by the thread while the transaction so that only one transaction can be
   activated with a database object at the same time.  Thus, the serializable isolation level is
   assumed if every database operation is performed in the transaction.  All updated regions are
   kept track of by write ahead logging while the transaction.  If the database is closed during
   transaction, the transaction is aborted implicitly. */
bool tcfdbtranbegin(TCFDB *fdb);


/* Commit the transaction of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   If successful, the return value is true, else, it is false.
   Update in the transaction is fixed when it is committed successfully. */
bool tcfdbtrancommit(TCFDB *fdb);


/* Abort the transaction of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   If successful, the return value is true, else, it is false.
   Update in the transaction is discarded when it is aborted.  The state of the database is
   rollbacked to before transaction. */
bool tcfdbtranabort(TCFDB *fdb);


/* Get the file path of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the path of the database file or `NULL' if the object does not connect to
   any database file. */
const char *tcfdbpath(TCFDB *fdb);


/* Get the number of records of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the number of records or 0 if the object does not connect to any database
   file. */
uint64_t tcfdbrnum(TCFDB *fdb);


/* Get the size of the database file of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the size of the database file or 0 if the object does not connect to any
   database file. */
uint64_t tcfdbfsiz(TCFDB *fdb);



/*************************************************************************************************
 * features for experts
 *************************************************************************************************/


/* Set the error code of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `ecode' specifies the error code.
   `file' specifies the file name of the code.
   `line' specifies the line number of the code.
   `func' specifies the function name of the code. */
void tcfdbsetecode(TCFDB *fdb, int ecode, const char *filename, int line, const char *func);


/* Set the file descriptor for debugging output.
   `fdb' specifies the fixed-length database object.
   `fd' specifies the file descriptor for debugging output. */
void tcfdbsetdbgfd(TCFDB *fdb, int fd);


/* Get the file descriptor for debugging output.
   `fdb' specifies the fixed-length database object.
   The return value is the file descriptor for debugging output. */
int tcfdbdbgfd(TCFDB *fdb);


/* Check whether mutual exclusion control is set to a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   If mutual exclusion control is set, it is true, else it is false. */
bool tcfdbhasmutex(TCFDB *fdb);


/* Synchronize updating contents on memory of a fixed-length database object.
   `fdb' specifies the fixed-length database object connected as a writer.
   `phys' specifies whether to synchronize physically.
   If successful, the return value is true, else, it is false. */
bool tcfdbmemsync(TCFDB *fdb, bool phys);


/* Get the minimum ID number of records of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the minimum ID number of records or 0 if the object does not connect to
   any database file. */
uint64_t tcfdbmin(TCFDB *fdb);


/* Get the maximum ID number of records of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the maximum ID number of records or 0 if the object does not connect to
   any database file. */
uint64_t tcfdbmax(TCFDB *fdb);


/* Get the width of the value of each record of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the width of the value of each record or 0 if the object does not connect
   to any database file. */
uint32_t tcfdbwidth(TCFDB *fdb);


/* Get the limit file size of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the limit file size or 0 if the object does not connect to any database
   file. */
uint64_t tcfdblimsiz(TCFDB *fdb);


/* Get the limit ID number of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the limit ID number or 0 if the object does not connect to any database
   file. */
uint64_t tcfdblimid(TCFDB *fdb);


/* Get the inode number of the database file of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the inode number of the database file or 0 if the object does not connect
   to any database file. */
uint64_t tcfdbinode(TCFDB *fdb);


/* Get the modification time of the database file of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the inode number of the database file or 0 if the object does not connect
   to any database file. */
time_t tcfdbmtime(TCFDB *fdb);


/* Get the connection mode of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the connection mode. */
int tcfdbomode(TCFDB *fdb);


/* Get the database type of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the database type. */
uint8_t tcfdbtype(TCFDB *fdb);


/* Get the additional flags of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the additional flags. */
uint8_t tcfdbflags(TCFDB *fdb);


/* Get the pointer to the opaque field of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   The return value is the pointer to the opaque field whose size is 128 bytes. */
char *tcfdbopaque(TCFDB *fdb);


/* Store a record into a fixed-length database object with a duplication handler.
   `fdb' specifies the fixed-length database object connected as a writer.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDPREV', the number less by one than
   the minimum ID number of existing records is specified.  If it is `FDBIDMAX', the maximum ID
   number of existing records is specified.  If it is `FDBIDNEXT', the number greater by one than
   the maximum ID number of existing records is specified.
   `vbuf' specifies the pointer to the region of the value.  `NULL' means that record addition is
   ommited if there is no corresponding record.
   `vsiz' specifies the size of the region of the value.  If the size of the value is greater
   than the width tuning parameter of the database, the size is cut down to the width.
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
bool tcfdbputproc(TCFDB *fdb, int64_t id, const void *vbuf, int vsiz, TCPDPROC proc, void *op);


/* Move the iterator to the record corresponding a key of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `id' specifies the ID number.  It should be more than 0.  If it is `FDBIDMIN', the minimum ID
   number of existing records is specified.  If it is `FDBIDMAX', the maximum ID number of
   existing records is specified.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record corresponding the condition. */
bool tcfdbiterinit2(TCFDB *fdb, int64_t id);


/* Move the iterator to the decimal record of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `kbuf' specifies the pointer to the region of the decimal key.  It should be more than 0.  If
   it is "min", the minimum ID number of existing records is specified.  If it is "max", the
   maximum ID number of existing records is specified.
   `ksiz' specifies the size of the region of the key.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record corresponding the condition. */
bool tcfdbiterinit3(TCFDB *fdb, const void *kbuf, int ksiz);


/* Move the iterator to the decimal string record of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
   `kstr' specifies the string of the decimal key.  It should be more than 0.  If it is "min",
   the minimum ID number of existing records is specified.  If it is "max", the maximum ID number
   of existing records is specified.
   If successful, the return value is true, else, it is false.  False is returned if there is
   no record corresponding the condition. */
bool tcfdbiterinit4(TCFDB *fdb, const char *kstr);


/* Process each record atomically of a fixed-length database object.
   `fdb' specifies the fixed-length database object.
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
bool tcfdbforeach(TCFDB *fdb, TCITER iter, void *op);


/* Generate the ID number from arbitrary binary data.
   `kbuf' specifies the pointer to the region of the key.
   `ksiz' specifies the size of the region of the key.
   The return value is the ID number. */
int64_t tcfdbkeytoid(const char *kbuf, int ksiz);



__TCFDB_CLINKAGEEND
#endif                                   /* duplication check */


/* END OF FILE */
