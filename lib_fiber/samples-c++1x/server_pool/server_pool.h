#pragma once

void server_pool_run(const char* addr, bool sync, int nfibers, int rtimeo, int wtimeo);
void server_pool2_run(const char* addr, bool sync, int nfibers);
