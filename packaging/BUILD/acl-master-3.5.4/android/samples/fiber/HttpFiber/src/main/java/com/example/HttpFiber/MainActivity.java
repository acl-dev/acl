package com.example.HttpFiber;

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
    private FiberThread fiberThread = new FiberThread();

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    /**
     * HTTP 读取失败的回调方法
     * @param info
     */
    public void onError(String info) {
        StringBuffer text = new StringBuffer();
        text.append("httpGet error\r\n");
        text.append(info + "\r\n");
        result.setText(text);
    }

    /**
     * 获得 HTTP 数据体成功的回调方法
     * @param body
     */
    public void onBody(String body) {
        result.setText(body);
    }

    /**
     * 从 HTTP 服务器获得数据
     */
    private void httpGet() {
        String host = domain.getText().toString().trim();
        if (host.isEmpty()) {
            return;
        }

        HttpFiber fiber = new HttpFiber();
        System.out.println("create one fiber");
        fiberThread.execute(fiber);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        domain = (EditText) findViewById(R.id.domain);
        result = findViewById(R.id.result);
        result.setMovementMethod(ScrollingMovementMethod.getInstance());

        // 绑定 HTTP 请求事件
        Button get = (Button) findViewById(R.id.http_get);
        get.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                httpGet();
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

        HttpHandler handler = new HttpHandler(this);
        // 创建协程调度线程
        fiberThread.start();
    }
}
