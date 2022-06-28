package com.example.http;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    private EditText domain;
    private TextView result;
    StringBuffer text = new StringBuffer();

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("ws");
    }

    /**
     * HTTP 读取失败的回调方法
     * @param info
     */
    public void onError(String info) {
        text.append("httpGet error\r\n");
        text.append(info + "\r\n");
        result.setText(text);
    }

    /**
     * 获得 HTTP 数据体成功的回调方法
     * @param data
     */
    public void onData(String data) {
        result.append(data);
        result.append("\r\n");
        int offset = result.getLineCount() * result.getLineHeight();
        if (offset > result.getHeight()){
            result.scrollTo(0,offset - result.getHeight());
        }
    }

    /**
     * 从 HTTP Websocket 服务器获得数据
     */
    private void websocketStart() {
        String addr = domain.getText().toString().trim();
        if (addr.isEmpty()) {
            return;
        }

        System.out.println("addr is " + addr);
        try {
            HttpHandler handler = new HttpHandler(this);
            HttpThread httpThread = new HttpThread(handler, addr);
            httpThread.start();
        } catch (Exception e) {
            e.printStackTrace();
            onError(addr);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        domain = (EditText) findViewById(R.id.domain);
        result = findViewById(R.id.result);
        result.setMovementMethod(ScrollingMovementMethod.getInstance());
        result.setScrollbarFadingEnabled(false);

        // 绑定 HTTP 请求事件
        Button get = (Button) findViewById(R.id.http_get);
        get.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                websocketStart();
            }
        });

        Button clear = (Button) findViewById(R.id.clear_results);
        clear.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
            // 清空结果显示区域
            result.setText("");
            }
        });
    }
}
