package com.example.http;

public class HttpClientFiber extends Fiber {
    //private String serverAddr;
    //private String serverHost;
    //private String url;
    private String serverAddr = "www.baidu.com:80";
    private String serverHost = "www.baidu.com";
    private String url = "/";

    HttpClientFiber() {
    }

    public void setServerAddr(String serverAddr) {
        this.serverAddr = serverAddr;
    }

    public void setServerHost(String serverHost) {
        this.serverHost = serverHost;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    @Override
    public void run() {
        HttpClient httpClient = new HttpClient(serverAddr);
        String body = httpClient.get(serverHost, url);
        System.out.println("got response:\r\n" + body);
    }

    ////////////////////////////////////////////////////////////////////////////
}
