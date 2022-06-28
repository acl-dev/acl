/*
 * tpllib
 *
 * C library of functions for text template processing.
 * Copyright (C) 2003-2007 Niels Wojciech Tadeusz Andersen <haj@zhat.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __C_TEMPLATE_ENGINE_H__
#define __C_TEMPLATE_ENGINE_H__

#include "lib_acl.h"

/*
 * Markup:
 *	Field: @@SOMETHING@@
 *	Section: <!-- @@SOMETHING_ELSE@@ -->
 *		  Content of section
 *		  perhaps with some @@FIELD@@
 *		 <!-- @@SOMETHING_ELSE@@ -->
 *
 * Sections can be nested to any depth.
 */

#define DELIMITER_LEFT "$<"
#define DELIMITER_RIGHT ">"
#define SECTIONTAG_HEAD "<!-- " DELIMITER_LEFT
#define SECTIONTAG_TAIL DELIMITER_RIGHT " -->"
 
/* The object, holding template data */
typedef struct template_s tpl_t;


#if defined(__cplusplus)
extern "C" {
#endif

/* Allocates and initializes new template object */
tpl_t* tpl_alloc(void);

/* Releases a template object allocated with tpl_alloc() */
void tpl_free(tpl_t* tpl);

/*
 * Copy the original template into the clone template
 * The clone parameter must be a newly initialized template object
 */
void tpl_copy(tpl_t* c, const tpl_t* original);

/* Initialize template object */
void tpl_init(tpl_t* tpl);

/* Release memory used by data in template strycture */
void tpl_release(tpl_t* tpl);

/* 
 * Constructs template data from specified template file
 * returns TPL_OK if successful
 */
int tpl_load(tpl_t* tpl, const char* filename);

/*
 * Construct template from string
 * returns TPL_OK if successful
 */
int tpl_from_string(tpl_t* tpl, const char* data, int len);

/* Clear fields, delete added content */
void tpl_reset(tpl_t* tpl);

/* Set field in currently selected section */
void tpl_set_field(tpl_t* tpl, const char* field, const char* val, int len);

/* Uses printf() formating */
void tpl_set_field_fmt(tpl_t* tpl, const char* field, const char* format, ...);

/* Output a number to a field */
void tpl_set_field_int(tpl_t* tpl, const char* field, int val);
void tpl_set_field_uint(tpl_t* tpl, const char* field, unsigned int val);
void tpl_set_field_double(tpl_t* tpl, const char* field, double val);

/*
 * Sets field to contents of a file
 * returns TPL_OK unless the file cannot be read
 */
int tpl_set_field_from_file(tpl_t* tpl, const char* field, const char* filename);

/* Sets a field everywhere in the document */
void tpl_set_field_global(tpl_t* tpl, const char* field, const char* val, int len);
void tpl_set_field_fmt_global(tpl_t* tpl, const char* field, const char* format, ...);
void tpl_set_field_int_global(tpl_t* tpl, const char* field, int val);
void tpl_set_field_uint_global(tpl_t* tpl, const char* field, unsigned int val);
void tpl_set_field_double_global(tpl_t* tpl, const char* field, double val);

/*
 * Selects section in currently selected section or in the document
 * if no section is selected yet
 * returns TPL_OK if sections is selected successfully
 */
int tpl_select_section(tpl_t* tpl, const char* section);

/*
 * Selects section containing current section (selects parent section)
 * returns TPL_OK if successfull.
 */
int tpl_deselect_section(tpl_t* tpl);

/* 
 * Sets content of specified section
 * returns TPL_OK if successful
 */
int tpl_set_section(tpl_t* tpl, const char* section, const char* val, int len);
int tpl_set_section_from_file(tpl_t* tpl, const char* section, const char* filename);

/*
 * Add a static copy of currently selected section's content 
 * before beginning tag of this section but after already "appended" content 
 * returns TPL_OK if successful
 */
int tpl_append_section(tpl_t* tpl);

/* returns length of current content not including the trailing \0 byte */
int tpl_length(const tpl_t* tpl);

/* returns length of currently selected section's content */
int tpl_section_length(const tpl_t* tpl);

/*
 * Copy content into supplied buffer, which must be large enough 
 * Use tpl_length() to obtain length and allocate correct buffer 
 * with the size length + 1 (to include the terminating \0 byte)
 */
void tpl_get_content(const tpl_t* tpl, char* buffer);

/*
 * Same as tpl_get_content() but with the contents of
 * the currently selected section.
 * If no section is selected then a call to this function is
 * equivalent to a call to tpl_get_content()
 */
void tpl_get_section_content(const tpl_t* tpl, char* buffer);

/*
 * Save to file
 * returns TPL_OK if succeessful
 */
int tpl_save_as(const tpl_t* tpl, const char* filename);

/*
 * Write content to a file descriptor (might be a socket)
 * returns TPL_OK if write succeeds
 */
int tpl_write(const tpl_t* tpl, int fd);

/*
 * Write content as simple HTTP-response, for use in nph- CGI programs
 * returns TPL_OK if write succeeds
 */
int tpl_http_write(const tpl_t* tpl, int fd);

/*
 * Write content of the tpl to out stream
 */
void tpl_out(tpl_t *tpl, ACL_VSTREAM *out);

#if defined(__cplusplus)
}
#endif


/* Return values for non-void functions */
#define TPL_OPEN_ERROR -7
#define TPL_WRITE_ERROR -6
#define TPL_READ_ERROR -5
#define TPL_NOT_FOUND_ERROR -4
#define TPL_SYNTAX_ERROR -3
#define TPL_NO_SECTION_SELECTED_ERROR -2
#define TPL_SECTION_NOT_FOUND_ERROR -1
#define TPL_OK 0



/* Header string for a minimalistic HTTP response */
#define TPL_NPH_HTTP_HEADER_START \
   "HTTP/1.1 200 OK\r\n"\
   "Content-Type: text/html\r\n"\
   "Content-Length: "

#define TPL_NPH_HTTP_HEADER_END \
    "\r\n\r\n"


#define HASH_TABLE_SIZE   29

/* Do not set this to less than the number of characters
   needed to represent any number you might want to output */
#define INITIAL_FIELD_LEN 31


/* Hash bucket for field entry */
typedef struct tpl_fcell_s
{
    char *key;
    char *val;
    int len;
    struct tpl_fcell_s *next;
} tpl_fcell_t;


/* Content of template is a linked list of those */
typedef struct tpl_node_s
{
    char *val;
    int len;
    struct tpl_fcell_s *fval;
    struct tpl_node_s *next;
} tpl_node_t;


/* Hash bucket structure for section entry */
typedef struct tpl_tcell_t
{
    char *key;
    tpl_t *tpl;
    tpl_node_t **preceding;
    struct tpl_tcell_t *_next;
    struct tpl_tcell_t *next;
} tpl_tcell_t;


/* The template structure definition */
typedef struct template_s
{
    tpl_node_t *head;
    tpl_node_t *added_head;
    tpl_node_t **added_tail;
    tpl_tcell_t *first;
    tpl_fcell_t *fields[HASH_TABLE_SIZE];
    tpl_tcell_t *sections[HASH_TABLE_SIZE];
    tpl_t *tpl;
    tpl_t *parent;
} template_t;

#endif
