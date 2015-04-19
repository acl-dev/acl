A redis tool for redis cluster, which can help to build a new redis cluster,
add one new redis node to the existing one or cluster, show the information
of the redis cluster about nodes and slots. Below are the using method of
the tool:
1) show help information:
./redis_build -h
usage: redis_builder.exe -h[help]
-s redis_addr[ip:port]
-a cmd[nodes|slots|create|add_node|del_node|node_id]
-N new_node[ip:port]
-S [add node as slave]
-f configure_file

for samples:
./redis_builder -s 127.0.0.1:6379 -a create -f cluster.xml
./redis_builder -s 127.0.0.1:6379 -a nodes
./redis_builder -s 127.0.0.1:6379 -a slots
./redis_builder -s 127.0.0.1:6379 -a del_node -I node_id
./redis_builder -s 127.0.0.1:6379 -a node_id
./redis_builder -s 127.0.0.1:6379 -a add_node -N 127.0.0.1:6380 -S

2) build a new redis cluster after all of the redis nodes started:
./redis_builder -a create -f cluster.xml

the cluster.xml's content just like:
<?xml version="1.0"?>
<xml>
    <node addr = "192.168.136.172:16380">
        <node addr = "192.168.136.172:16381" />
        <node addr = "192.168.136.172:16382" />
    </node>
    <node addr = "192.168.136.172:16383">
        <node addr = "192.168.136.172:16384" />
        <node addr = "192.168.136.172:16385" />
    </node>
    <node addr = "192.168.136.172:16386">
        <node addr = "192.168.136.172:16387" />
        <node addr = "192.168.136.172:16388" />
    </node>
</xml>

3) add a new slave redis node to the master node:
./redis_builder -s 127.0.0.1:6379 -a add_node -S -N 127.0.0.1:6380
'-s' specifys the existing master node,
'-S' specifys the new redis node added was as a slave node,
'-N' specify the new redis addr to be added
'-a' specify the cmd of this tool

4) compile this redis_builder tool
4.1) because redis_builder depends on lib_acl/lib_protocol/lib_acl_cpp,
so you need to compile the lib_acl/lib_protocol/lib_acl_cpp first
$cd lib_acl; make
$cd lib_protocol; make
$cd lib_acl_cpp; make
4.2) compile redis_builder
$cd app/redis_tools/redis_builder; make

5) see redis module in acl:
[lib_acl_cpp/samples/redis/README.md](redis of acl)
