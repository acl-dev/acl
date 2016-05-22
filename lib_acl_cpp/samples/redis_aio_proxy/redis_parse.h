//
//  redis_parse.h
//  supex
//
//  Created by 周凯 on 16/1/14.
//  Copyright © 2016年 zhoukai. All rights reserved.
//

#ifndef redis_parse_h
#define redis_parse_h

#include <redis_parser.h>

__BEGIN_DECLS

#ifndef REDIS_MAX_FIELDS
#define REDIS_MAX_FIELDS    128
#endif

#if REDIS_MAX_FIELDS < 1
#error "ERROR PARAMETER `REDIS_MAX_FIELDS`"
#endif

struct redis_presult
{
    unsigned        error;  /**< !=0 表示出错*/
    unsigned        type;   /**< 分析类型：请求／响应*/
    int             fields; /**< 结果字段数量 -1表示结果为空对象 或 >=1 单结果或结果列表*/
    union
    {
        unsigned        reply_type; /**< 响应类型，对应响应分析有效*/
        unsigned        command_id; /**< 命令索引*/
    };
    struct
    {
        unsigned        offset;         /**< 字段偏移，长度>0时有效*/
        int             len;            /**< 字段长度，当为－1时，该字段为空对象，长度可以为0*/
    }               field[REDIS_MAX_FIELDS];    /**< 字段集合，有效数量以fields字段确定*/
    char *const     *data;  /**< 当前被分析数据首地址的指针*/
    unsigned const  *size;  /**< 当前被分析数据的长度地址*/
    bool            over;   /**< 当前分析是否结束*/
    unsigned        dosize; /**< 当前已分析长度*/
    unsigned        step;   /**< 当前分析进行了多少次*/
};


struct redis_parse_t {
    redis_parser         rp; /**< 分析器*/
    struct redis_presult rs; /**< 分析结果*/
};


/**
 * @param info解析句柄
 * @param buff待解析的字符串地址指针
 * @param size待解析的字符串长度指针
 */
void redis_parse_init(struct redis_parse_t *info, redis_parser_type parse_type, char *const *data, unsigned const *size);

/**
 * 解析响应redis字符串
 * @return false 还未结束，需要后续数据；ture 已经结束，根据内部状态判断成功与否
 */
bool redis_parse_response(struct redis_parse_t *info);

/**
 * 解析请求redis字符串
 * @return false 还未结束，需要后续数据；ture 已经结束，根据内部状态判断成功与否
 */
bool redis_parse_request(struct redis_parse_t *info);


__END_DECLS
#endif  /* redis_parse_h */