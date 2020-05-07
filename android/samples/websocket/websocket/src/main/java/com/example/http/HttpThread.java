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
        HttpClient httpClient = new HttpClient(serverAddr);
        httpClient.websocketStart();
    }
}
