package com.example.http;

public final class HttpClient {
    private String serverAddr;

    public HttpClient(String addr) {
        this.serverAddr = addr;
    }

    public String get(String host, String url) {
        try {
            String body = HttpGet(serverAddr, host, url);
            return body;
        } catch (UnsatisfiedLinkError e) {
            System.out.println("HttpGet error=" + e.toString());
            return null;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Jni Native method
    private native String HttpGet(String serverAddr, String host, String url);
}
