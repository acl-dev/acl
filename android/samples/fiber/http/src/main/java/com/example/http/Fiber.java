package com.example.http;

public abstract class Fiber {
    Fiber() {
    }

    public void start() {
        FiberStart();
    }

    private void onCreate() {
        this.run();
    }

    public abstract void run();

    ////////////////////////////////////////////////////////////////////////////

    private native void FiberStart();
}
