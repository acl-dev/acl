package com.example.http;

public final class HttpClient {
    private HttpHandler handler;
    private String serverAddr;

    public HttpClient(HttpHandler handler, String addr) {
        this.handler    = handler;
        this.serverAddr = addr;
    }

    /**
     * 启动并测试 Websocket 客户端
     */
    public void websocketStart() {
        WebsocketStart(serverAddr);
    }

    /**
     * 在 jni 中回调此方法
     * @param msg {String}
     */
    public void onMessage(String msg) {
        handler.onData(msg);
    }

    ////////////////////////////////////////////////////////////////////////////

    // Jni Native method
    private native void WebsocketStart(String addr);
}
