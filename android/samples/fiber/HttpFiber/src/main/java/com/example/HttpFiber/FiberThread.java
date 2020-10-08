package com.example.HttpFiber;

public class FiberThread {
    private long waiter;
    public FiberThread() {}

    public void start() {
        waiter = FiberSchedule();
    }

    public void execute(HttpFiber fiber) {
        FiberStart(waiter, fiber);
    }

    ///////////////////////////////////////////////////////////////////////////

    private native long FiberSchedule();
    private native void FiberStart(long fiberSchedule, Object o);
}
