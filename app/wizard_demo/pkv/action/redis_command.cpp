//
// Created by shuxin ¡¡¡¡zheng on 2023/8/1.
//

#include "redis_command.h"

namespace pkv {

redis_command::redis_command(redis_handler& handler, const redis_object& obj)
: handler_(handler), obj_(obj)
{}

} // namespace pkv
