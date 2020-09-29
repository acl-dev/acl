package com.example.http;

public class HttpFiberThread extends Thread {
    private String serverAddr = "www.baidu.com:80";
    private String host = "www.baidu.com";
    private String url = "/";
    private static HttpClientFiber fb;

    public HttpFiberThread() {
    }

    @Override
    public void run() {
        // 创建几个HTTP协程
        for (int i = 0; i < 5; i++) {
            //getOne();
        }

        fb = new HttpClientFiber();
        fb.setServerAddr(serverAddr);
        fb.setServerHost(host);
        fb.setUrl(url);

        fb.start();

        // in fiber schedule loop
        FiberSchedule();
    }

    private void getOne() {
        HttpFiberGet(serverAddr, host, url);
    }

    ////////////////////////////////////////////////////////////////////////////

    // Jni Native method
    private native void FiberSchedule();
    private native void HttpFiberGet(String serverAddr, String host, String url);
}
