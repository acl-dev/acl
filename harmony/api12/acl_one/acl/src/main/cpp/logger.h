//
// Created by zhengshuxin on 2019/8/26.
//

#pragma once

typedef void (*write_fn)(void *, const char *);

void log_info(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_fatal(const char *fmt, ...);
void log_debug(const char *fmt, ...);

void log_open();
void log_close();