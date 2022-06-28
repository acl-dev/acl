package com.example.HttpFiber;

import android.os.Handler;
import android.os.Message;

public final class HttpHandler extends Handler {
    final public static int BODY_OK = 0;
    final public static int BODY_ERR = 1;
    private final MainActivity mainActivity;

    public HttpHandler(MainActivity mainActivity) {
        this.mainActivity = mainActivity;
    }

    @Override
    public void handleMessage(Message message) {
        switch (message.what) {
        case BODY_OK:
            mainActivity.onBody((String) message.obj);
            break;
        case BODY_ERR:
            mainActivity.onError((String) message.obj);
            break;
        default:
            break;
        }
    }

    /**
     * 成功获得 HTTP 数据体的回调方法
     * @param body {String} 响应的 HTTP 数据体
     */
    public void onBody(String body) {
        Message message = new Message();
        message.what = BODY_OK;
        message.obj  = body;
        this.sendMessage(message);
    }

    /**
     * 获取 HTTP 结果失败时的回调方法
     * @param domain {String} 请求的域名
     * @param url {String} 请求的 URL
     */
    public void onError(String domain, String url) {
        String error = "Domain: domain, " + "url: url";
        Message message = new Message();
        message.what = BODY_ERR;
        message.obj = error;
        this.sendMessage(message);
    }
}
