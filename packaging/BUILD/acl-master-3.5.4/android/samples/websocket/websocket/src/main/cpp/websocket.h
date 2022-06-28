#pragma once

/**
 * 启动 Websocket 非阻塞客户端
 * @param addr {const char*} Websocket 服务端地址
 * @param callback {void (*)(void*, void*, const char*)} 读到服务端信息后的回调
 * @param env {void*}
 * @param obj {void*}
 * @return {bool}
 */
bool websocket_run(const char* addr, void (*callback)(void*, void*, const char*),
        void* env, void* obj);
