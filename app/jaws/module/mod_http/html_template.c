#include "http_service.h"

char HTTP_REPLY_DNS_ERR[] = \
"HTTP/1.1 503 DNS lookup error\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body>DNS LOOKUP error\r\n</body></html>\r\n";

char HTTP_REPLY_TIMEOUT[] = \
"HTTP/1.1 504 webserver respond timeout\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body>504 webserver respond timeout</body></html>\r\n";

char HTTP_REPLY_ERROR[] = \
"HTTP/1.1 500 webserver respond error\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 500 webserver respond error</body></html>\r\n";

char HTTP_SEND_ERROR[] = \
"HTTP/1.1 500 send to webserver error\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 500 send to webserver error</body></html>\r\n";

char HTTP_CONNECT_ERROR[] = \
"HTTP/1.1 500 connect webserver error\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 500 connect webserver error</body></html>\r\n";

char HTTP_CONNECT_TIMEOUT[] = \
"HTTP/1.1 504 connect webserver error\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 504 connect webserver timeout</body></html>\r\n";

char HTTP_REQUEST_INVALID[] = \
"HTTP/1.1 400 client request invalid\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 400 request invalid</body></html>\r\n";

char HTTP_REQUEST_LOOP[] = \
"HTTP/1.1 403 request loop test forbidden\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 403 forbidden, request loop tested</body></html>\r\n";

char HTTP_REQUEST_DENY[] = \
"HTTP/1.1 403 request forbidden\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 403 forbidden, request denied</body></html>\r\n";

char HTTP_REQUEST_NOFOUND[] = \
"HTTP/1.1 404 request url not found\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 404 forbidden, request url not found</body></html>\r\n";

char HTTP_INTERNAL_ERROR[] = \
"HTTP/1.1 500 internal error\r\n" \
"Connection: close\r\n" \
"\r\n" \
"<html><body> 500 internal error</body></html>\r\n";
