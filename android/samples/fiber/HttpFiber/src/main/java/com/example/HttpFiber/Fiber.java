package com.example.HttpFiber;

public abstract class Fiber {
    private String msg;

    public Fiber() {}

    private void setMsg(String msg) {
        this.msg = msg;
    }

    private void onRun() {
        System.out.println("Fiber: onRun");
        this.run();
    }

    public abstract void run();

    ////////////////////////////////////////////////////////////////////////////

}
