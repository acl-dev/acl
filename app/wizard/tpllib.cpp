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
#include "stdafx.h"
#include "lib_acl.h"

#ifdef WIN32
#include <io.h>                 /* write() */
#else
#include <unistd.h>             /* write() */
#endif

#include <stdlib.h> 		/* exit() */
#include <stdio.h> 		/* fopen(), sprintf() ... */
#include <stdarg.h>		/* va_start(), va_end() */
#include <string.h> 		/* memset(), memcmp() ... */

/* Define ZTS when building a PHP extension */
/* Define WIN32 when compiling under MS Windows */

#ifdef WIN32
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#endif

#include "lib_tpl.h"

/* Length defines for markup elements */
#define DELIM_LEN_LEFT (sizeof(DELIMITER_LEFT) - 1)
#define DELIM_LEN_RIGHT (sizeof(DELIMITER_RIGHT) - 1)
#define SEC_HEAD_LEN (sizeof(SECTIONTAG_HEAD) - 1)
#define SEC_TAIL_LEN (sizeof(SECTIONTAG_TAIL) - 1)


#define M_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define M_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define M_NODE_LEN(n) (((n)->val) ? (n)->len : (n)->fval->len)
#define M_NODE_TEXT(n) (((n)->val) ? (n)->val : (n)->fval->val)

#define M_INT_TO_STRING(buffer, buffer_len, val, signed_v) \
    do \
    { \
        char rev[5 * sizeof(int) / 2]; \
        int i = 0, digits = 0; \
        if ((signed_v) && (val) < 0) \
        { \
            buffer[i] = '-'; \
            i++; \
        } \
        do \
        { \
            rev[digits++] = '0' + abs((val) % 10); \
            (val) /= 10; \
        } \
        while ((val) != 0); \
        for (; digits > 0 && i < (buffer_len); i++) \
            buffer[i] = rev[--digits]; \
        buffer[i] = 0; \
        (buffer_len) = i; \
    } while (0)


/* Binary safe string search */
static const char *tpl_strstr(register const char *haystack,
                              ssize_t haystack_len,
                              const char *needle,
                              int needle_len)
{
    ssize_t i;
    const char *p_toofar = haystack + haystack_len - needle_len + 1;

    if (p_toofar <= haystack)
        return NULL;

    do
    {
        if (*haystack == *needle)
        {
            for (i = 1; i != needle_len && haystack[i] == needle[i]; i++);

            if (i == needle_len)
                return haystack;
        }
        
        haystack++;
    }
    while (haystack != p_toofar);

    return NULL;
}


static tpl_node_t* create_node(int len)
{
    tpl_node_t* node = (tpl_node_t*)acl_mymalloc(len+1+sizeof(tpl_node_t));
    node->len = len;
    node->val = (char*)(node + 1);
    node->next = NULL;
    return node;
}


static void destroy_node(tpl_node_t* node)
{
    acl_myfree(node);
}


static tpl_fcell_t* create_fcell(const char *key, int key_len, int data_len)
{
    tpl_fcell_t* field = (tpl_fcell_t*)acl_mymalloc(key_len + 1
                                                + sizeof(tpl_fcell_t));
    
    field->key = (char *)(field + 1);

    (void)memcpy(field->key, key, key_len);
    field->key[key_len] = 0;
    field->val = (char *)acl_mymalloc(M_MAX(data_len, INITIAL_FIELD_LEN) + 1);
    field->len = 0;
    field->val[0] = 0;
    field->next = NULL;
    return field;
}


static void destroy_fcell(tpl_fcell_t* field)
{
    acl_myfree(field->val);
    acl_myfree(field);
}


static tpl_tcell_t* create_tcell(const char *key, int key_len)
{
    tpl_tcell_t* section = (tpl_tcell_t*)acl_mymalloc(key_len + 1
                                                  + sizeof(tpl_tcell_t)
                                                  + sizeof(tpl_t));
    section->tpl = (tpl_t *)(section + 1);
    tpl_init(section->tpl);

    section->key = (char *)(section->tpl + 1);
    (void)memcpy(section->key, key, key_len);

    section->key[key_len] = 0;
    section->next = NULL;
    section->_next = NULL;
    return section;
}


static void destroy_tcell(tpl_tcell_t* section)
{
    tpl_release(section->tpl);
    acl_myfree(section);
}


/* Hash function using multipy by 31 */
static unsigned int tpl_hash(const char *key, int key_len)
{
    const char *p_key_end = key + key_len;
    register unsigned int h = *key++;

    while (key != p_key_end && *key != 0)
    {
        unsigned int g;
        h = (h << 5) + *key++;
        if ((g = h & 0xf0000000) != 0)
            h = (h ^ (g >> 24) ^ g);
    }
    return h % HASH_TABLE_SIZE;
}
 
static tpl_fcell_t* tpl_produce_field(tpl_t *tpl,
                                      const char* key,
                                      int key_len,
                                      int data_len,
                                      int may_create)
{
    tpl_fcell_t **pfield = &tpl->fields[tpl_hash(key, key_len)];

    while (*pfield != NULL)
    {
        if (memcmp((*pfield)->key, key, key_len) == 0)
        {
            if (data_len > INITIAL_FIELD_LEN && data_len > (*pfield)->len)
                (*pfield)->val = (char*) acl_myrealloc((*pfield)->val, data_len);

            return *pfield;
        }

        pfield = &(*pfield)->next;
    }

    if (*pfield == NULL && may_create != 0)
    {
        *pfield = create_fcell(key, key_len, data_len);
        return *pfield;
    }

    return NULL;
}

#define tpl_get_field(tpl, key) \
    tpl_produce_field((tpl), (key), (int) strlen((key)), 0, 0)

#define tpl_new_field(tpl, key, key_len) \
    tpl_produce_field((tpl), (key), (key_len), 0, 1)

#define tpl_cpy_field(field, tpl, key, key_len, data, data_len) \
    do \
    { \
        (field) = tpl_produce_field((tpl), (key), (key_len), (data_len), 1); \
        if ((field)->len == 0 && (data_len) != 0) \
        { \
            (void)memcpy((field)->val, (data), (data_len));\
            (field)->val[(data_len)] = 0; \
            (field)->len = (data_len); \
        } \
    } while (0)


static tpl_tcell_t* tpl_produce_section(tpl_t *tpl,
                                        const char* key,
                                        int key_len,
                                        int must_create)
{
    tpl_tcell_t **psection = &tpl->sections[tpl_hash(key, key_len)];

    while (*psection != NULL)
    {
        if (memcmp((*psection)->key, key, key_len) == 0)
            return (must_create) ? NULL : *psection;

        psection = &(*psection)->_next;
    }
    if (*psection == NULL && must_create)
    {
        *psection = create_tcell(key, key_len);
        (*psection)->tpl->parent = tpl;
        return *psection;
    }

    return NULL;
}

#define tpl_get_section(tpl, key) \
    tpl_produce_section(tpl, key, (int) strlen(key), 0)

#define tpl_make_section(tpl, key, key_len) \
    tpl_produce_section(tpl, key, key_len, 1)


/* Big, ugly, horrible, quite possibly buggy...
   live with it or correct it and let me know */
static int tpl_construct(tpl_t *tpl, const char *p_last, const char *p_end)
{
    const char *p_begin = p_last, *p_curr, *p_next;
    tpl_node_t **tail = &tpl->head;
    tpl_tcell_t **last_section = &tpl->first;

    /* While a field delimiter can be found in what is left */
    while ((p_curr = tpl_strstr(p_last, (ssize_t) (p_end - p_last), 
                                DELIMITER_LEFT, DELIM_LEN_LEFT)) != NULL)
    {
        /* Advance to beginning of field/section name */
        p_curr += DELIM_LEN_LEFT;

        /* Find end delimiter of identifier or fail with syntax error */
        p_next = tpl_strstr(p_curr, (ssize_t) (p_end - p_curr),
		DELIMITER_RIGHT, DELIM_LEN_RIGHT);

        if (p_next == NULL)
        {
            *last_section = NULL;
            *tail = NULL;
            return TPL_SYNTAX_ERROR;
        }

        /* Section */
        if (p_curr + 1 >= p_begin + SEC_HEAD_LEN
             && p_end - p_next >= (int) SEC_TAIL_LEN
             && memcmp(SECTIONTAG_HEAD, p_curr - SEC_HEAD_LEN, 
                        SEC_HEAD_LEN) == 0
             && memcmp(SECTIONTAG_TAIL, p_next, SEC_TAIL_LEN) == 0)
        {
            tpl_tcell_t *section;

            if ((p_curr - SEC_HEAD_LEN - p_last) != 0)
            {
                int val_len = (int) (p_curr - SEC_HEAD_LEN - p_last);
                *tail = create_node(val_len);
                (*tail)->val[val_len] = 0;
                (void)memcpy((*tail)->val, p_last, val_len);
                tail = &(*tail)->next;
            }
        
            /* Create and chain in entry for section */
            section = tpl_make_section(tpl, p_curr, (int) (p_next - p_curr));

            if (section != NULL)
            {
                int code;
                const char *beginning, *ending;

                *last_section = section;
                last_section = &section->next;
                section->preceding = tail;

                /* Advance past the section tag */
                p_last = p_next + SEC_TAIL_LEN;
                beginning = p_next + SEC_TAIL_LEN;

                /* Find next occurrence of this section tag */
                ending = tpl_strstr(beginning, 
                                    (int) (p_end - beginning),
                                    p_curr - SEC_HEAD_LEN,
                                    (int) (beginning - p_curr + SEC_HEAD_LEN));

                p_last = ending + (SEC_HEAD_LEN + p_last - p_curr);
        
                /* If found and found before p_end */
                if (ending != NULL && p_last <= p_end)
                {
                    /* Construct recursively */
                    code = tpl_construct(section->tpl, beginning, ending);
                }
                else
                    code = TPL_SYNTAX_ERROR;
            
                if (code != TPL_OK)
                {
                    tpl_release(section->tpl);
                    tpl_init(section->tpl);
                    *last_section = NULL;
                    *tail = NULL;
                    return TPL_SYNTAX_ERROR;
                }
            }
            else
            {
                *last_section = NULL;
                *tail = NULL;
                return TPL_SYNTAX_ERROR;
            }
        }
        /* Field */
        else
        {
            if ((p_curr - DELIM_LEN_LEFT - p_last) != 0)
            {
                int val_len = (int) (p_curr - DELIM_LEN_LEFT - p_last);
                *tail = create_node(val_len);
                (*tail)->val[val_len] = 0;
                (void)memcpy((*tail)->val, p_last, val_len);
                tail = &(*tail)->next;
            }

            p_last = p_next + DELIM_LEN_RIGHT;

            /* Create node and set fval to new field cell */
            *tail = create_node(0);
            (*tail)->val = NULL;
            (*tail)->fval = tpl_new_field(tpl, p_curr, (int) (p_next - p_curr));
            tail = &(*tail)->next;
        }
    }

    /* Store rest of the text */
    if (p_last < p_end)
    {
        int val_len = (int) (p_end - p_last);
        *tail = create_node(val_len);
        (void)memcpy((*tail)->val, p_last, val_len);
        (*tail)->val[val_len] = 0;
        tail = &(*tail)->next;
    }
    *tail = NULL;
    *last_section = NULL;

    return TPL_OK;
}

void tpl_init(tpl_t *tpl)
{
    /* Set everything to NULL */
    (void)memset(tpl, 0, sizeof(tpl_t));
    
    tpl->tpl = tpl;
    tpl->added_tail = &tpl->added_head;
}

void tpl_release(tpl_t *tpl)
{
    register int i;

    /* Free nodes */
    while (tpl->head != NULL)
    {
        tpl_node_t *deleted = tpl->head;
        tpl->head = tpl->head->next;
        destroy_node(deleted);    
    }

    /* Free any added content */
    while (tpl->added_head != NULL)
    {
        tpl_node_t *deleted = tpl->added_head;
        tpl->added_head = tpl->added_head->next;
        destroy_node(deleted);
    }
    /* Free field cells */
    for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
        tpl_fcell_t *fc = tpl->fields[i];

        while (fc != NULL)
        {
            tpl_fcell_t *deleted = fc;
            fc = fc->next;
            destroy_fcell(deleted);
        }
    }
    /* Free sections including added content */
    while (tpl->first != NULL)
    {
        tpl_tcell_t *deleted = tpl->first;
        tpl->first = tpl->first->next;

        /* This will call tpl_release() recursively */
        destroy_tcell(deleted);
    }
}

tpl_t* tpl_alloc(void)
{
    tpl_t *tpl = (tpl_t*)acl_mymalloc(sizeof(tpl_t));
    tpl_init(tpl);
    return tpl;
}

void tpl_free(tpl_t *tpl)
{
    tpl_release(tpl);
    acl_myfree(tpl);
}

/* Load template file */
int tpl_load(tpl_t *tpl, const char *filename)
{
    char *buffer;
    int len;
    FILE *fp = fopen(filename, "rb");
    
    if (fp == NULL)
        return TPL_OPEN_ERROR;

    /* Find length of file content */
    (void)fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    (void) rewind(fp);
    
    /* Allocate buffer for data + 0 byte */
    buffer = (char*)acl_mymalloc(len+1);
    
    if (fread(buffer, 1, len, fp) < (unsigned)len)
    {
        (void)fclose(fp);
        acl_myfree(buffer);
        return TPL_READ_ERROR;
    }
    (void)fclose(fp);
    buffer[len] = 0;
    
    /* Use data in buffer to construct template */
    len = tpl_construct(tpl, buffer, buffer + len);
    if (len != TPL_OK)
    {
        tpl_release(tpl);
        tpl_init(tpl);
    }
    acl_myfree(buffer);
    return len;
}

/* Load from a string */
int tpl_from_string(tpl_t *tpl, const char *buffer, int len)
{
    return tpl_construct(tpl, buffer, buffer + len);
}

/* Recursively clear added content */
void tpl_reset(tpl_t* tpl)
{
    int i = HASH_TABLE_SIZE;
    tpl_tcell_t* section;
    
    /* Clear fields */
    while (i != 0)
    {
        tpl_fcell_t* field = tpl->fields[--i];
        while (field != NULL)
        {
            field->len = 0;
            *field->val = 0;
            field = field->next;
        }
    }
    /* Clear sections */
    for (section = tpl->first; section != NULL; section = section->next)
    {
        /* Clear added content */
        while (section->tpl->added_head != NULL)
        {
            tpl_node_t* n = section->tpl->added_head;
            section->tpl->added_head = section->tpl->added_head->next;
            destroy_node(n);
        }
        section->tpl->added_tail = &section->tpl->added_head;
    
        /* Clear this section */
        tpl_reset(section->tpl);
    }
}


/* Set up the selected section pointer to correspond to 
   that of the copied template */
static void tpl_adjust_selection(tpl_t* tpl,
                                 const tpl_t* srctpl,
                                 tpl_tcell_t* sec,
                                 const tpl_tcell_t* srcsec)
{
    const tpl_tcell_t *sec1 = srcsec;
    tpl_tcell_t *sec2 = sec;

    if (srctpl->tpl != srctpl)
    {
        for (; tpl->tpl == tpl && sec1 != NULL;
               sec1 = sec1->next, sec2 = sec2->next)
        {
            if (srctpl->tpl == sec1->tpl)
                tpl->tpl = sec2->tpl;
            else
            {
                tpl_adjust_selection(tpl,
                                     srctpl, 
                                     sec2->tpl->first,
                                     sec1->tpl->first);
            }
        }
    }
}


/* Recursive copy */
void tpl_copy(tpl_t* tpl, const tpl_t* srctpl)
{
    tpl_node_t **tail = &tpl->head;
    tpl_node_t *curr_node = srctpl->head;
    tpl_tcell_t **last_section = &tpl->first;
    tpl_tcell_t *curr_section = srctpl->first;

    for (; curr_section != NULL; curr_section = curr_section->next)
    {
        /* Copy text and fields until the current section's position */
        while (curr_node != *curr_section->preceding)
        {
            /* Text */
            if (curr_node->val != NULL)
            {
                *tail = create_node(curr_node->len);
                (void)memcpy((*tail)->val, curr_node->val, curr_node->len);
                (*tail)->val[curr_node->len] = 0;
            }
            /* Field */
            else
            {
                /* Create node and set fval to field cell */
                *tail = create_node(0);
                (*tail)->val = NULL;

                tpl_cpy_field((*tail)->fval, 
                              tpl,
                              curr_node->fval->key,
                              (int) strlen(curr_node->fval->key),
                              curr_node->fval->val,
                              curr_node->fval->len);

            }

            tail = &(*tail)->next;
            curr_node = curr_node->next;
        }

        /* Create emtpy section entry for current section */
        *last_section = tpl_make_section(tpl,
                                         curr_section->key,
                                         (int) strlen(curr_section->key));

        (*last_section)->preceding = tail;


        /* Consolidate added content into a single node in newly created
           section entry so that only one memory allocation is needed */
        if (curr_section->tpl->added_head != NULL)
        {
            tpl_node_t *some_node = curr_section->tpl->added_head;
            char *buffer;
            int len = 0;

            /* Get length of added content */
            for (; some_node!=NULL; some_node=some_node->next)
                len += some_node->len;

            /* Create node for added content */
            (*last_section)->tpl->added_head = create_node(len);
            (*last_section)->tpl->added_head->next = NULL;
            (*last_section)->tpl->added_tail = 
                &(*last_section)->tpl->added_head->next;

            /* Copy added content into new node */

            buffer = (*last_section)->tpl->added_head->val;

            for (some_node = curr_section->tpl->added_head;
                 some_node != NULL;
                 some_node = some_node->next)
            {
                (void)memcpy(buffer, some_node->val, some_node->len);
                buffer += some_node->len;
            }
            *buffer = 0;
        }


        /* Recursively copy section */
        tpl_copy((*last_section)->tpl, curr_section->tpl);

        last_section = &(*last_section)->next;
    }

    /* Copy the rest */
    while (curr_node != NULL)
    {
        /* Text */
        if (curr_node->val != NULL)
        {
            *tail = create_node(curr_node->len);
            (void)memcpy((*tail)->val, curr_node->val, curr_node->len + 1);
        }
        /* Field */
        else
        {
            *tail = create_node(0);
            (*tail)->val = NULL;

            tpl_cpy_field((*tail)->fval, 
                          tpl,
                          curr_node->fval->key,
                          (int) strlen(curr_node->fval->key),
                          curr_node->fval->val,
                          curr_node->fval->len);
        }

        tail = &(*tail)->next;
        curr_node = curr_node->next;
    }

    *tail = NULL;
    *last_section = NULL;
    tpl->tpl = tpl;

    tpl_adjust_selection(tpl, srctpl, tpl->first, srctpl->first);
}


void tpl_set_field(tpl_t* tpl, const char* key, const char* val, int len)
{
    tpl_fcell_t *field = tpl_get_field(tpl->tpl, key);
    if (field != NULL)
    {
        if (len > INITIAL_FIELD_LEN && len > field->len)
            field->val = (char*) acl_myrealloc(field->val, len + 1);

        field->len = len;
        (void)memcpy(field->val, val, len);
        field->val[len] = 0;
    }
}

void tpl_set_field_fmt(tpl_t* tpl, const char* key, const char* fmt, ...)
{
    tpl_fcell_t *field = tpl_get_field(tpl->tpl, key);

    if (field != NULL)
    {
        int n;

        if (field->len < INITIAL_FIELD_LEN)
            field->len = INITIAL_FIELD_LEN;

        while (1)
        {
            va_list ap;

            va_start(ap, fmt);
            n = vsnprintf(field->val, field->len + 1, fmt, ap);
            va_end(ap);

            if (n > -1 && n <= field->len)
                break;

            field->len *= 2;
            field->val = (char*) acl_myrealloc(field->val, field->len + 1);
        }
        field->len = n;
    }
}

void tpl_set_field_int(tpl_t* tpl, const char* key, int val)
{
    tpl_fcell_t *field = tpl_get_field(tpl->tpl, key);

    if (field != NULL)
    {
        if (field->len < INITIAL_FIELD_LEN)
            field->len = INITIAL_FIELD_LEN;

        M_INT_TO_STRING(field->val, field->len, val, 1);
    } 
}

void tpl_set_field_uint(tpl_t* tpl, const char* key, unsigned int val)
{
    tpl_fcell_t *field = tpl_get_field(tpl->tpl, key);

    if (field != NULL)
    {
	int  v = (int) val;

        if (field->len < INITIAL_FIELD_LEN)
            field->len = INITIAL_FIELD_LEN;

        M_INT_TO_STRING(field->val, field->len, v, 0);
    } 
}

void tpl_set_field_double(tpl_t* tpl, const char* key, double val)
{
    tpl_fcell_t *field = tpl_get_field(tpl->tpl, key);

    if (field != NULL)
    {
        if (field->len < INITIAL_FIELD_LEN)
            field->len = INITIAL_FIELD_LEN;

        field->len = snprintf(field->val, field->len, "%.2f", val);
    }
}

int tpl_set_field_from_file(tpl_t* tpl, const char* key, const char* filename)
{
    tpl_fcell_t *field = tpl_get_field(tpl->tpl, key);

    if (field != NULL)
    {
        int len;
        FILE *fp = fopen(filename, "rb");

        if (fp == NULL)
            return TPL_OPEN_ERROR;

        /* Find length of file content */
        (void)fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        (void)rewind(fp);

        if (len > INITIAL_FIELD_LEN && len > field->len)
            field->val = (char*) acl_myrealloc(field->val, len + 1);

        if (fread(field->val, 1, len, fp) < (unsigned)len)
        {
            (void)fclose(fp);
            return TPL_READ_ERROR;
        }
        (void)fclose(fp); 
    
        field->len = len;
        field->val[len] = 0;
    }
    return TPL_OK;
}

void tpl_set_field_global(tpl_t* tpl, const char* key, const char* val, int len)
{
    tpl_tcell_t *section = tpl->first;
    tpl_set_field(tpl, key, val, len);
    while (section != NULL)
    {
        /* Recursively set in all sections */
        tpl_set_field_global(section->tpl, key, val, len);
        section = section->next;
    }
}

void tpl_set_field_fmt_global(tpl_t* tpl, const char* key, const char* fmt, ...)
{
    int n;
    int len = 256;
    char *buf = (char*)acl_mymalloc(len);

    while (1)
    {
        va_list ap;

        va_start(ap, fmt);
        n = vsnprintf(buf, len, fmt, ap);
        va_end(ap);

        if (n > -1 && n < len)
            break;

        len *= 2;
        buf = (char*) acl_myrealloc(buf, len);
    }
    tpl_set_field_global(tpl, key, buf, n);
    acl_myfree(buf);
}

void tpl_set_field_int_global(tpl_t* tpl, const char* key, int val)
{
    tpl_tcell_t *section = tpl->first;
    tpl_set_field_int(tpl, key, val);
    while (section != NULL)
    {
        /* Recursively set in all sections */
        tpl_set_field_int_global(section->tpl, key, val);
        section = section->next;
    }
}

void tpl_set_field_uint_global(tpl_t* tpl, const char* key, unsigned int val)
{
    tpl_tcell_t *section = tpl->first;
    tpl_set_field_uint(tpl, key, val);
    while (section != NULL)
    {
        /* Recursively set in all sections */
        tpl_set_field_uint_global(section->tpl, key, val);
        section = section->next;
    }
}

void tpl_set_field_double_global(tpl_t* tpl, const char* key, double val)
{
    tpl_tcell_t *section = tpl->first;
    tpl_set_field_double(tpl, key, val);
    while (section != NULL)
    {
        /* Recursively set in all sections */
        tpl_set_field_double_global(section->tpl, key, val);
        section = section->next;
    }
}

int tpl_select_section(tpl_t *tpl, const char* key)
{
    tpl_tcell_t *section = tpl_get_section(tpl->tpl, key);
    if (section != NULL)
    {
        tpl->tpl->tpl = section->tpl;
        tpl->tpl = tpl->tpl->tpl;
        return TPL_OK;
    }
    return TPL_SECTION_NOT_FOUND_ERROR;
}

int tpl_deselect_section(tpl_t *tpl)
{
    if (tpl->tpl != tpl)
    {
        tpl->tpl = tpl->tpl->parent;
        tpl->tpl->tpl = tpl->tpl;
        return TPL_OK;
    }
    return TPL_NO_SECTION_SELECTED_ERROR;
}


/* Set or replace added content in section with supplied value */
static int tpl_set_section_ex(tpl_t* tpl,
                              const char* key,
                              const char* val,
                              int len,
                              tpl_node_t *existing_node)
{
    tpl_tcell_t *section = tpl_get_section(tpl->tpl, key);

    if (section != NULL)
    {
        if (section->tpl->first != NULL
            || section->tpl->head == NULL
            || section->tpl->head->next != NULL
            || section->tpl->head->len != len)
        {
            /* Destroy the contents of this section */
            tpl_release(section->tpl);
            tpl_init(section->tpl);

            section->tpl->parent = tpl->tpl;
            section->tpl->head = create_node(len);
        }

        (void)memcpy(section->tpl->head->val, val, len);
        section->tpl->head->val[len] = 0;

        if (existing_node != NULL)
        {
             existing_node->next = section->tpl->added_head;
             section->tpl->added_head = existing_node;
        }
        else
        {
            if (section->tpl->added_head != NULL
                && section->tpl->added_head->len != len)
            {
                tpl_node_t *node = create_node(len);
                node->next = section->tpl->added_head;
                section->tpl->added_head = node;
            }
            else if (section->tpl->added_head == NULL)
                section->tpl->added_head = create_node(len);

            (void)memcpy(section->tpl->added_head->val, val, len);
            section->tpl->added_head->val[len] = 0;
        }

        while (section->tpl->added_head->next != NULL)
        {
            tpl_node_t *deleted = section->tpl->added_head->next;
            section->tpl->added_head->next = deleted->next;
            destroy_node(deleted);
        }

        section->tpl->added_tail = &section->tpl->added_head->next;

        return TPL_OK;
    }

    return TPL_SECTION_NOT_FOUND_ERROR;
}

int tpl_set_section(tpl_t* tpl, const char* key, const char* val, int len)
{
    return tpl_set_section_ex(tpl, key, val, len, NULL);
}

int tpl_set_section_from_file(tpl_t* tpl, const char* key, const char* filename)
{
    int len, status;
    tpl_node_t *node;
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL)
        return TPL_OPEN_ERROR;

    /* Find length of file content */
    (void)fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    (void)rewind(fp);

    node = create_node(len);

    if (fread(node->val, 1, len, fp) < (unsigned)len)
        status = TPL_READ_ERROR;
    else
        status = tpl_set_section_ex(tpl, key, node->val, len, node);

    (void)fclose(fp);

    if (status != TPL_OK)
        destroy_node(node);

    return status;
}


int tpl_append_section(tpl_t* tpl)
{
    if (tpl->tpl != tpl)
    {
        tpl_tcell_t *curr_sect = tpl->tpl->first;
        tpl_node_t *curr_node = tpl->tpl->head;
        tpl_node_t *some_node;
        int len;
        char *buffer;

        for (; curr_sect != NULL; curr_sect = curr_sect->next)
        {
            if (curr_sect->tpl->added_head != NULL)
            {

                for (len = 0, some_node = curr_node; 
                     some_node != *curr_sect->preceding;
                     some_node = some_node->next)
                {
                    len += M_NODE_LEN(some_node);
                }

                *tpl->tpl->added_tail = create_node(len);
                buffer = (*tpl->tpl->added_tail)->val;
                tpl->tpl->added_tail = &(*tpl->tpl->added_tail)->next;

                for (; 
                     curr_node != *curr_sect->preceding;
                     curr_node = curr_node->next)
                {
                    if (curr_node->val)
                    {
                        (void)memcpy(buffer, curr_node->val, curr_node->len);
                        buffer += curr_node->len;
                    }
                    else
                    {
                        (void)memcpy(buffer, 
                                     curr_node->fval->val,
                                     curr_node->fval->len);
                        buffer += curr_node->fval->len;
                    }
                }

                *buffer = 0;

                /* Cut 'n paste added content chain from section */
                *tpl->tpl->added_tail = curr_sect->tpl->added_head;
                tpl->tpl->added_tail = curr_sect->tpl->added_tail;
                curr_sect->tpl->added_tail = &curr_sect->tpl->added_head;
                curr_sect->tpl->added_head = NULL;
            }
        }

        for (len = 0, some_node = curr_node;
             some_node != NULL;
             some_node = some_node->next)
        {
            len += M_NODE_LEN(some_node);
        }


        *tpl->tpl->added_tail = create_node(len);
        buffer = (*tpl->tpl->added_tail)->val;
        tpl->tpl->added_tail = &(*tpl->tpl->added_tail)->next;

        for (; curr_node != NULL; curr_node = curr_node->next)
        {
            if (curr_node->val)
            {
                (void)memcpy(buffer, curr_node->val, curr_node->len);
                buffer += curr_node->len;
            }
            else
            {
                (void)memcpy(buffer,
                             curr_node->fval->val,
                             curr_node->fval->len);

                buffer += curr_node->fval->len;
            }
        }
        *buffer = 0;
        *tpl->tpl->added_tail = NULL;

        return TPL_OK;
    }
    return TPL_NO_SECTION_SELECTED_ERROR;
}


int tpl_length(const tpl_t* tpl)
{
    tpl_node_t *n = tpl->head;
    tpl_tcell_t *s = tpl->first;
    int len;

    for (len = 0; n != NULL; n = n->next)
        len += (n->val) ? n->len : n->fval->len;

    for (; s != NULL; s = s->next)
    {
        for (n = s->tpl->added_head; n != NULL; n = n->next)
            len += n->len;
    }

    return len;
}


int tpl_section_length(const tpl_t* tpl)
{
    return tpl_length(tpl->tpl);
}

void tpl_get_content(const tpl_t* tpl, char* buffer)
{
    tpl_node_t *n = tpl->head;
    tpl_tcell_t *s = tpl->first;
    
    for (; s != NULL; s = s->next)
    {
        tpl_node_t *an = s->tpl->added_head;

        /* Copy part until beginning of section */
        for (; n != *s->preceding; n = n->next)
        {
            if (n->val)
            {
                (void)memcpy(buffer, n->val, n->len);
                buffer += n->len;
            }
            else
            {
                (void)memcpy(buffer, n->fval->val, n->fval->len);
                buffer += n->fval->len;
            }
        }

        /* Copy content appended in section */
        for (; an != NULL; an = an->next)
        {
            (void)memcpy(buffer, an->val, an->len);
            buffer += an->len;
        }
    }

    /* Copy the rest */
    for (; n != NULL; n = n->next)
    {
        if (n->val)
        {
            (void)memcpy(buffer, n->val, n->len);
            buffer += n->len;
        }
        else
        {
            (void)memcpy(buffer, n->fval->val, n->fval->len);
            buffer += n->fval->len;
        }
    }
    *buffer = 0;
}

void tpl_get_section_content(const tpl_t* tpl, char* buffer)
{
    tpl_get_content(tpl->tpl, buffer);
}

int tpl_save_as(const tpl_t* tpl, const char* filename)
{
    FILE *fp = fopen(filename, "wb");

    if (fp != NULL)
    {
        int len = tpl_length(tpl);
	int n;
        char *content = (char *)acl_mymalloc(len + 1);

        tpl_get_content(tpl, content);

        n = (int) fwrite(content, len, 1, fp);
	if (n != 1)
        {
            (void)fclose(fp);
            acl_myfree(content);
            return TPL_WRITE_ERROR;
        }
        (void)fclose(fp);

        acl_myfree(content);
        return TPL_OK;
    }
    return TPL_OPEN_ERROR;
}

#ifdef WIN32
#define write _write
#endif

int tpl_write(const tpl_t* tpl, int fd)
{  
    int len = tpl_length(tpl);
    char *content = (char *)acl_mymalloc(len + 1);

    tpl_get_content(tpl, content);
    len = write(fd, content, len);
    acl_myfree(content);

    if (len < 0)
        return TPL_WRITE_ERROR;

    return TPL_OK;
}


int tpl_http_write(const tpl_t* tpl, int fd)
{  
    /* Assumption: the content length can be max a 10 digit number */
    int length_digits = 10;

    int header_len = sizeof(TPL_NPH_HTTP_HEADER_START)
                   + sizeof(TPL_NPH_HTTP_HEADER_END)
                   + length_digits;

    int content_len = tpl_length(tpl);
    int tmp_len = content_len;
    
    /* Long enough for both the HTTP header and content */
    char *response = (char*)acl_mymalloc(content_len + header_len + 1);

    char *str_p = response;

    (void)memcpy(str_p, 
                TPL_NPH_HTTP_HEADER_START,
                sizeof(TPL_NPH_HTTP_HEADER_START) - 1);

    str_p += sizeof(TPL_NPH_HTTP_HEADER_START) - 1;
    
    M_INT_TO_STRING(str_p, length_digits, tmp_len, 0);
    str_p += length_digits;

    (void)memcpy(str_p,
                 TPL_NPH_HTTP_HEADER_END,
                 sizeof(TPL_NPH_HTTP_HEADER_END) - 1);

    str_p += sizeof(TPL_NPH_HTTP_HEADER_END) - 1;

    tpl_get_content(tpl, str_p);
    content_len = write(fd, response, content_len
	    + (unsigned int) (str_p - response));
    acl_myfree(response);

    if (content_len < 0)
        return TPL_WRITE_ERROR;

    return TPL_OK;
}

void tpl_out(tpl_t *tpl, ACL_VSTREAM *out)
{
	char *buf;
	int   n;

	if (tpl == NULL)
		return;
	if (out == NULL)
		out = ACL_VSTREAM_OUT;

	n = tpl_length(tpl);
	if (n <= 0)
		return;
	buf = (char*) acl_mymalloc(n + 1);
	tpl_get_content(tpl, buf);
	acl_vstream_writen(out, buf, n);
	acl_myfree(buf);
}
