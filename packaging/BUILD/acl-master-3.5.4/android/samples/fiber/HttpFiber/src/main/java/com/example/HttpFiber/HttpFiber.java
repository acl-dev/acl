package com.example.HttpFiber;

public class HttpFiber {
    public HttpFiber() {}

    private void onRun() {
        System.out.println("Fiber: onRun");
        this.run();
    }

    public void run() {
        System.out.println("HttpFiber started!");
    }
}
