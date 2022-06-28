#!/bin/sh

./https_request -s www.sina.com.cn:443
echo ""
./https_request -s www.baidu.com:443
echo ""
./https_request -s echo.websocket.org:443
