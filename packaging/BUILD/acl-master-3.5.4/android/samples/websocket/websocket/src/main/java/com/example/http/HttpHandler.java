package com.example.http;

import android.os.Handler;
import android.os.Message;

public final class HttpHandler extends Handler {
    final public static int DATA_OK = 0;
    private final MainActivity mainActivity;

    public HttpHandler(MainActivity mainActivity) {
        this.mainActivity = mainActivity;
    }

    @Override
    public void handleMessage(Message message) {
        switch (message.what) {
        case DATA_OK:
            mainActivity.onData((String) message.obj);
            break;
        default:
            break;
        }
    }

    /**
     * 在 Websocket 线程空间中的回调方法，用来传递消息
     * @param data {String} 响应的 HTTP 数据体
     */
    public void onData(String data) {
        Message message = new Message();
        message.what = DATA_OK;
        message.obj  = data;
        this.sendMessage(message);
    }
}
