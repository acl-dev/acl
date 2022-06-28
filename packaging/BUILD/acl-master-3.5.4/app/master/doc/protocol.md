# acl_master 服务器框架远程管理

## 远程管理功能

---

## 通信协议

采用标准 HTTP 协议，数据体内容为 Json 格式  

### 1. 显示当前所有的服务

请求协议：
```
POST /?cmd=list HTTP/1.0
Content-Length: 25
Connection: keep-alive
Content-Type: text/json

{ cmd: 'list', data: {}}
```

响应协议：
```
HTTP/1.1 200 OK
Date: Tue, 19 Sep 2017 06:51:50 GMT
Server: acl
Content-Type: text/json; charset=utf-8
Content-Length: 1588
Connection: Keep-Alive

{"status":200,"msg":"ok","data":[{"status":200,"name":"*.*.*.*:53","type":5,"start":0,"owner":"","path":"/opt/soft/acl-master/libexec/udp-echo","conf":"/opt/soft/acl-master/conf/service/udp-echo.cf","proc_max":1,"proc_prefork":1,"proc_total":1,"proc_avail":1,"throttle_delay":6,"listen_fd_count":0,"notify_addr":"","notify_recipients":"","env":[{"LOG":"/opt/soft/acl-master/var/log/udp-echo"},{"MASTER_LOG":"/opt/soft/acl-master/var/log/acl_master"},{"SERVICE_LOG":"/opt/soft/acl-master/var/log/udp-echo"}],"procs":[{"pid":26036,"start":1505803886}]}]}
```

### 2. 显示指定服务的状态**

请求协议
```
POST /?cmd=stat HTTP/1.0
Content-Length: 81
Connection: keep-alive
Content-Type: text/json

{ cmd: 'stat', data: [{'path':'/opt/soft/acl-master/conf/service/udp-echo.cf'}]}
```

响应协议
```
HTTP/1.1 200 OK
Date: Tue, 19 Sep 2017 06:59:34 GMT
Server: acl
Content-Type: text/json; charset=utf-8
Content-Length: 552
Connection: Keep-Alive

{"status":200,"msg":"ok","data":[{"status":200,"name":"*.*.*.*:53","type":5,"start":0,"owner":"","path":"/opt/soft/acl-master/libexec/udp-echo","conf":"/opt/soft/acl-master/conf/service/udp-echo.cf","proc_max":1,"proc_prefork":1,"proc_total":1,"proc_avail":1,"throttle_delay":6,"listen_fd_count":0,"notify_addr":"","notify_recipients":"","env":[{"LOG":"/opt/soft/acl-master/var/log/udp-echo"},{"MASTER_LOG":"/opt/soft/acl-master/var/log/acl_master"},{"SERVICE_LOG":"/opt/soft/acl-master/var/log/udp-echo"}],"procs":[{"pid":26036,"start":1505803886}]}]}
```

### 3. 停止指定的服务进程

请求协议
```
POST /?cmd=stop HTTP/1.0
Content-Length: 81
Connection: keep-alive
Content-Type: text/json

{ cmd: 'stop', data: [{'path':'/opt/soft/acl-master/conf/service/udp-echo.cf'}]}
```

响应协议
```
HTTP/1.1 200 OK
Date: Tue, 19 Sep 2017 07:02:58 GMT
Server: acl
Content-Type: text/json; charset=utf-8
Content-Length: 104
Connection: Keep-Alive

{"status":200,"msg":"ok","data":[{"status":200,"path":"/opt/soft/acl-master/conf/service/udp-echo.cf"}]}
```

### 4. 启动指定的服务进程

请求协议
```
POST /?cmd=start HTTP/1.0
Content-Length: 83
Connection: keep-alive
Content-Type: text/json

{ cmd: 'start', data: [{'path':'/opt/soft/acl-master/conf/service/udp-echo.cf'}]}
```

响应协议
```
HTTP/1.1 200 OK
Date: Tue, 19 Sep 2017 07:07:27 GMT
Server: acl
Content-Type: text/json; charset=utf-8
Content-Length: 116
Connection: Keep-Alive

{"status":200,"msg":"ok","data":[{"status":200,"name":"*.*.*.*:53","path":"/opt/soft/acl-master/libexec/udp-echo"}]}
```

### 5. 使指定服务进程重读配置

请求协议
```
POST /?cmd=reload HTTP/1.0
Content-Length: 84
Connection: keep-alive
Content-Type: text/json

{ cmd: 'reload', data: [{'path':'/opt/soft/acl-master/conf/service/udp-echo.cf'}]}
```

响应协议
```
HTTP/1.1 200 OK
Date: Tue, 19 Sep 2017 07:09:13 GMT
Server: acl
Content-Type: text/json; charset=utf-8
Content-Length: 137
Connection: Keep-Alive

{"status":200,"msg":"ok","data":[{"status":200,"proc_count":1,"proc_signaled":1,"path":"/opt/soft/acl-master/conf/service/udp-echo.cf"}]}
```
