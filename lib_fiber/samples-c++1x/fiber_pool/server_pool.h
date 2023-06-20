#pragma once

void server_pool_run(const char* addr, bool sync, int nfibers);
void server_pool2_run(const char* addr, bool sync, int nfibers);
