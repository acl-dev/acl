//
//  redis_parse.c
//  supex
//
//  Created by 周凯 on 16/1/14.
//  Copyright © 2016年 zhoukai. All rights reserved.
//
#include "redis_parse.h"

/* gcc version. for example : v4.1.2 is 40102, v3.4.6 is 30406 */
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/*
 *逻辑跳转优化
 */
#if GCC_VERSION
/*条件大多数为真，与if配合使用，直接执行if中语句*/
#define likely(x)     __builtin_expect(!!(x), 1)
/*条件大多数为假，与if配合使用，直接执行else中语句*/
#define unlikely(x)   __builtin_expect(!!(x), 0)
#else
#define likely(x)     (!!(x))
#define unlikely(x)   (!!(x))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define UINT32_MAX  (0xffffffff)

#define return_val_if_fail(p, ret)\
if(!(p)){\
printf("%s:%d Warning:"#p"Failed\n",__func__,__LINE__);\
return (ret);}



static int message_begin_cb(redis_parser *parser, unsigned value);

static int content_len_cb(redis_parser *parser, unsigned len);

static int content_cb(redis_parser *parser, const char *data, size_t len);

static int message_complete_cb(redis_parser *parser, unsigned fields);

static bool _redis_parse_handle(redis_parser *parser, redis_parser_settings *settings);

static redis_parser_settings _parse_settings = {
    message_begin_cb,
    content_len_cb,
    content_cb,
    message_complete_cb,
};

void redis_parse_init(struct redis_parse_t *info, redis_parser_type parse_type, char *const *data, unsigned const *size)
{
    assert(info);

    char *const     *p_data = (data != NULL) ? data : info->rs.data;
    unsigned const    *p_size = (size != NULL) ? size : info->rs.size;
    memset(&info->rs, 0, sizeof(info->rs));
    info->rs.data = p_data;
    info->rs.size = p_size;
    info->rs.over = false;
    info->rp.data = &info->rs;
    redis_parser_init(&info->rp, parse_type);
}


bool redis_parse_response(struct redis_parse_t *info)
{
    assert(info);
    return _redis_parse_handle(&info->rp, &_parse_settings);
}

bool redis_parse_request(struct redis_parse_t *info)
{
    assert(info);
    return _redis_parse_handle(&info->rp, &_parse_settings);
}

static int message_begin_cb(redis_parser *parser, unsigned value)
{
    struct redis_presult *stat = (struct redis_presult *)parser->data;
    assert(stat && stat->data && stat->size && *stat->size);

    stat->over = false;
    memset(stat->field, 0, sizeof(stat->field));
    /*
     * 保存总字段数
     * 当为－1或0时不会回调content_len_cb()和content_cb()
     * 如果当前为数字／成功消息／错误消息则不会掉用content_len_cb()
     */
    stat->fields = parser->fields;
    if (parser->type == REDIS_REQUEST) {
        stat->command_id = value;
    } else {
        stat->reply_type = parser->reply_type;
    }
    stat->type = parser->type;

    return 0;
}

static int content_len_cb(redis_parser *parser, unsigned len)
{
    struct redis_presult *stat = (struct redis_presult *)parser->data;
    return_val_if_fail(len <= UINT32_MAX && len > 0, -1);
    /*根据分析器的剩余处理字段，计算下标*/
    int pos = stat->fields - parser->fields;
    stat->field[pos].len = (int)len;
    return 0;
}

static int content_cb(redis_parser *parser, const char *data, size_t len)
{
    struct redis_presult *stat = (struct redis_presult *)parser->data;
    return_val_if_fail(len <= UINT32_MAX, -1);
    /*根据分析器的剩余处理字段，计算下标*/
    int pos = stat->fields - parser->fields;

    if (unlikely(stat->field[pos].offset == 0)) {
        assert(stat->data && *stat->data);
        stat->field[pos].offset = (unsigned)(data - *stat->data);
    }

    return 0;
}

static int message_complete_cb(redis_parser *parser, unsigned fields)
{
    (void)fields;
    struct redis_presult *stat = (struct redis_presult *)parser->data;
    stat->over = true;
    return 0;
}

static bool _redis_parse_handle(redis_parser *parser, redis_parser_settings *settings)
{
    struct redis_presult *stat = (struct redis_presult *)parser->data;
    assert(stat && stat->data && stat->size && *stat->size);

    /*计算本次需要分析的数据起始位置和长度*/
    const char *data = *stat->data + stat->dosize;
    size_t size = *stat->size - stat->dosize;

    unsigned done = (unsigned)redis_parser_execute(parser, settings, data, size);
    stat->step++;
    stat->dosize += done;
    stat->error = parser->redis_errno;
    if (!stat->error && !stat->over) {
        stat->error = 1; // 请求不完整，例如： "*3\r\n"此类请求
    }
    return true;
}