package com.example.http;

public class HttpFiberThread {
    private HttpHandler handler;
    private String addr = "www.baidu.com:80";
    private String host = "www.baidu.com";
    private String url = "/";
    private long waiter;

    public HttpFiberThread(HttpHandler handler) {
        this.handler = handler;
    }

    public void start() {
        // in fiber schedule loop
        waiter = FiberSchedule();
    }

    public void httpGet(String addr, String host, String url) {
        String body = HttpGet(waiter, addr, host, url);
        if (body != null) {
            System.out.println("body: " + body);
            handler.onBody(body);
        } else {
            System.out.println("body null");
            handler.onError(host, url);
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    // Jni Native method
    private native long FiberSchedule();
    private native String HttpGet(long o, String addr, String host, String url);
}
