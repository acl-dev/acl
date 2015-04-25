A redis tool for redis cluster, which can help to build a new redis cluster,
add one new redis node to the existing one or cluster, show the information
of the redis cluster about nodes and slots. Below are the using method of
the tool:
## 1) show help information:
```help
./redis_build -h
usage: redis_builder.exe -h[help]
-s redis_addr[ip:port]
-a cmd[nodes|slots|create|add_node|del_node|node_id|reshard]
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
./redis_builder -s 127.0.0.1:6379 -a reshard
```

## 2) build a new redis cluster after all of the redis nodes started:
### 2.1) build cluster and the relationship between master and slave
was specified by the configuer file.
./redis_builder -a create -f cluster.xml
the cluster.xml's content just like:
```xml
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
```
The result maybe like as:
```result
master: 192.168.136.172:16380
        slave: 192.168.136.172:16381
        slave: 192.168.136.172:16382
master: 192.168.136.172:16383
        slave: 192.168.136.172:16384
        slave: 192.168.136.172:16385
master: 192.168.136.172:16386
        slave: 192.168.136.172:16387
        slave: 192.168.136.172:16388
```

### 2.2) build cluster and the relationship between master and slave was specified
by the command args.
./redis_builder -a create -r 2 -f cluster.xml
the cluster.xml's content just like:
```xml
<?xml version="1.0"?>
<xml>
    <node addr = "192.168.136.171:16380" />
    <node addr = "192.168.136.171:16381" />
    <node addr = "192.168.136.171:16382" />
    <node addr = "192.168.136.172:16383" />
    <node addr = "192.168.136.172:16384" />
    <node addr = "192.168.136.172:16385" />
    <node addr = "192.168.136.173:16386" />
    <node addr = "192.168.136.173:16387" />
    <node addr = "192.168.136.173:16388" />
</xml>
```
The redis_builder will create three master nodes that each master will have
two slave nodes. The three master will be in different hosts, and each
master's slave nodes will be in other host than its master. The redis nodes
of the cluster maybe like as:
```result
master: 192.168.136.171:16380
        slave: 192.168.136.172:16383
        slave: 192.168.136.173:16386
master: 192.168.136.172:16384
        slave: 192.168.136.173:16388
        slave: 192.168.136.171:16382
master: 192.168.136.173:16387
        slave: 192.168.136.172:16385
        slave: 192.168.136.171:16381
```

## 3) add a new slave redis node to the master node:
```help
./redis_builder -s 127.0.0.1:6379 -a add_node -S -N 127.0.0.1:6380
'-s' specifys the existing master node,
'-S' specifys the new redis node added was as a slave node,
'-N' specify the new redis addr to be added
'-a' specify the cmd of this tool
```

## 4) compile this redis_builder tool
### 4.1) build the base three libs for redis_builder:
because redis_builder depends on lib_acl/lib_protocol/lib_acl_cpp,
so you need to compile the lib_acl/lib_protocol/lib_acl_cpp first
```compile
$cd lib_acl; make
$cd lib_protocol; make
$cd lib_acl_cpp; make
```
### 4.2) compile redis_builder
```compile
$cd app/redis_tools/redis_builder; make
```

## 5) reference
- redis samples in acl: [redis of acl](../../../lib_acl_cpp/samples/redis)
- redis include in acl: [lib_acl_cpp/include/acl_cpp/redis](../../../lib_acl_cpp/include/acl_cpp/redis)
- redis source  in acl: lib_acl_cpp/src/redis: [lib_acl_cpp/src/redis](../../../lib_acl_cpp/src/redis)

