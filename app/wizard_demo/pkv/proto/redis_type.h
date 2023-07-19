//
// Created by shuxin ¡¡¡¡zheng on 2023/7/19.
//

#pragma once

namespace pkv {

enum {
    redis_s_null,
    redis_s_obj,
    redis_s_arlen,
    redis_s_array,
    redis_s_number,
    redis_s_strlen,
    redis_s_string,
    redis_s_status,
    redis_s_error,
};

} // namespace pkv
