package com.example.http;

public final class HttpThread extends Thread {
    private HttpHandler handler;
    private String serverAddr;
    private String host;
    private String url;

    public HttpThread(HttpHandler handler, String serverAddr,
                      String host, String url) {
        this.handler = handler;
        this.serverAddr = serverAddr;
        this.host = host;
        this.url = url;
    }

    @Override
    public void run() {
        // 在独立线程中发起 HTTP 请求，并将结果通过 handler 传递至界面线程中

        HttpClient httpClient = new HttpClient(serverAddr);
        String body = httpClient.get(host, url);
        if (body != null) {
            System.out.println("body: " + body);
            handler.onBody(body);
        } else {
            System.out.println("body null");
            handler.onError(host, url);
        }
    }
}
