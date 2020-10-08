package com.example.http;

public class HttpFiberThread {
    private HttpHandler handler;
    private long waiter;

    public HttpFiberThread(HttpHandler handler) {
        this.handler = handler;
    }

    public void start() {
        // in fiber schedule loop
        waiter = FiberSchedule();
    }

    public void httpGet(String host, int port, String url) {
        class HttpThread extends Thread {
            private String host;
            private int port;
            private String url;
            public HttpThread(String host, int port, String url) {
                this.host = host;
                this.port = port;
                this.url  = url;
            }

            @Override
            public void run() {
                String body = HttpGet(waiter, host, port, url);
                if (body != null) {
                    System.out.println("response body length: " + body.length());
                    handler.onBody(body);
                } else {
                    System.out.println("body null");
                    handler.onError(host, url);
                }
            }
        }

        HttpThread httpThread = new HttpThread(host, port, url);
        httpThread.start();
    }

    ////////////////////////////////////////////////////////////////////////////

    // Jni Native method
    private native long FiberSchedule();
    private native String HttpGet(long o, String addr, int port, String url);
}
