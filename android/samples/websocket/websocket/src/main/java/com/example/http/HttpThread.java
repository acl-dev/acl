package com.example.http;

public final class HttpThread extends Thread {
    private HttpHandler handler;
    private String serverAddr;

    public HttpThread(HttpHandler handler, String serverAddr) {
        this.handler    = handler;
        this.serverAddr = serverAddr;
    }

    @Override
    public void run() {
        HttpClient httpClient = new HttpClient(handler, serverAddr);
        httpClient.websocketStart();
    }
}
