#!/bin/sh
# udp transfer with UNIX domain socket
#./udp_client -s /opt/soft/acl-master/var/public/test.sock@udp -l /tmp/tt.sock@udp -n 100000
./udp_client -s 127.0.0.1:8888 -l 127.0.0.1:8889 -n 100000
