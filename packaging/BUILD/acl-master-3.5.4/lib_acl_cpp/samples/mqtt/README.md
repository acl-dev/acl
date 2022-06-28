# **The streaming mqtt parser in acl**
## **1. About**
Because most of mqtt parsers at present are bound with network IO, which limits their using scope when we just want a mqtt parser to be used in our own network libs. I've complete a streaming mqtt in FSM（Finite State Machine）mode which can be used by any network libs(TCP ore UDP).  

The streaming mqtt parser implemented in acl was written in standard C++, which can be used client or server sides, including all the mqtt commands such as CONNECT, CONNACK, PUBLISH, PUBACK, etc.

## **2. C++ API**
The MQTT module in acl includes MQTT commands and MQTT network IO that each MQTT command was implemented as a C++ class. The user can set the MQTT C++ object to build for each MQTT command, and send the data after serializing the C++ object.   

The MQTT commands classes include **`mqtt_connect, mqtt_connack, mqtt_disconnect, mqtt_publish, mqtt_puback, mqtt_pubcomp, mqtt_pubrec, mqtt_rel, mqtt_subscribe, mqtt_suback, mqtt_unsubscribe, mqtt_unsuback, mqtt_pingreq, mqtt_pingresp`** which are inheriting from the base class **`mqtt_message`**. The **`mqtt_header`** class in acl mqtt is used to parse or build the mqtt header.   

The **`mqtt_client, mqtt_aclient`** are the network communication classes of acl mqtt, which **`mqtt_client`** uses the sync IO, and **`mqtt_aclient`** uses async IO.

## **3. Samples**
Although the mqtt parser of acl is alone from network IO, the network IO module for mqtt was also provided, including sync IO and async IO. Some mqtt samples were written by acl mqtt are shown in [mqtt_samples path](./). 
- **mqtt_aclient:** A MQTT client in async IO mode;
- **mqtt_aserver:** A MQTT server in async IO mode;
- **mqtt_client:** A MQTT client in sync IO mode;
- **mqtt_server:** A MQTT server in sync IO mode;
- **mqtt_pub:** A MQTT publish client in sync IO mode.

### **3.1 Write a MQTT client in sync IO mode with acl mqtt**
At first, we should construct a MQTT command C++ class object as below:

```C++
        acl::mqtt_connect message;

        message.set_cid("client-id-test-xxx");
        message.set_username("user-zsx");
        message.set_passwd("pass-zsx");
        message.set_will_qos(acl::MQTT_QOS0);
        message.set_will_topic("test/topic");
        message.set_will_msg("msg-hello");
```

The second, construct a MQTT IO object and send the MQTT command object:
```C++
        const char* addr = "127.0.0.1|1883";
        int conn_timeout = 10 /* seconds */, rw_timeout = 0;
        acl::mqtt_client conn(addr, conn_timeout, rw_timeout);
        if (!conn.send(message)) {
                printf("send MQTT message failed!\r\n");
        } else {
                printf("send MQTT message success!\r\n");
        }
```

At last, waiting for the MQTT messages from MQTT server:
```C++
static bool handle_message(acl::mqtt_client& conn, acl::mqtt_message& message) {
        acl::mqtt_type_t type = message.get_header().get_type();
        switch (type) {
        case acl::MQTT_CONNACK:
                return handle_connack(conn, message);
        case acl::MQTT_PINGREQ:
                return handle_pingreq(conn);
        case acl::MQTT_PINGRESP:
                return handle_pingresp();
        case acl::MQTT_DISCONNECT:
                return handle_disconnect(message);
        case acl::MQTT_SUBACK:
                return handle_suback(conn, message);
        case acl::MQTT_PUBLISH:
                return handle_publish(conn, message);
        default:
                printf("unknown type=%d\r\n", (int) type);
                return false;
        }
}

static void waiting_messages(acl::mqtt_client& conn) {
        while (true) {
                acl::mqtt_message* res = conn.get_message();
                if (res == NULL) {
                        printf("read message error\r\n");
                        break;
                }
                if (!handle_message(conn, *res)) {
                        delete res;
                        break;
                }
                delete res;
        }
}
```

After getting the **CONNACK** we can send **SUBSCRIBE** command to MQTT server:
```C++
static __thread unsigned short __pkt_id = 0;

static bool handle_connack(acl::mqtt_client& conn, const acl::mqtt_message& message) {
        const acl::mqtt_connack& connack = (const acl::mqtt_connack&) message;

        printf("%s => connect code=%d\r\n", __FUNCTION__, connack.get_connack_code());

        // constrcut SUBSCRIBE command and send it to the MQTT server

        acl::mqtt_subscribe sub;

        if (++__pkt_id == 0) {
                __pkt_id = 1;  // the pkt id must more than zero.
        }
        sub.set_pkt_id(__pkt_id);

        sub.add_topic("test/topic1", acl::MQTT_QOS1);
        sub.add_topic("test/topic2", acl::MQTT_QOS1);
        sub.add_topic("test/topic3", acl::MQTT_QOS1);

        if (conn.send(sub)) {
                printf("send subscribe ok\r\n");
                return true;
        }

        printf("send subscribe error\r\n");
        return false;
}
```

We can easily complete The left functions in `handle_message()` above including `handle_pingreq, handle_pingresp, handle_disconnect, handle_suback, handle_publish`.

### **3.2 Write a MQTT server in sync IO mode with acl mqtt**
### **3.3 Write a MQTT client in async IO mode with acl mqtt**
### **3.4 Write a MQTT server in async IO mode with acl mqtt**

## **4. Compile acl mqtt module**
Because acl mqtt lib is a part of lib_acl_cpp lib, and lib_acl_cpp depend lib_acl and lib_protocol, you should compile lib_acl and lib_protocol libs first, and compile lib_acl_cpp lib. After you've compiled lib_acl_cpp lib, the module lib is also compiled OK.

### **4.1 Compile on UNIX/LINUX**
- 1 compile `lib_acl.a`: Enter into **lib_acl** path and type make, the lib_acl.a will be compiled
- 2 compile `lib_protocol.a`: Enter into **lib_protocol** path and type make, the lib_protocol.a will be compiled
- 3 compile `lib_acl_cpp.a`: Enter into **lib_acl_cpp** path and type make, the lib_acl_cpp.a will be compiled
- 4 compile mqtt samples: Enter into lib_acl_cpp\samples\mqtt and type make, all the mqtt samples will be compiled.

### **4.2 Compile on WINDOWS**
You can use `VC2003`, `VC2008`, `VC2010`, `VC2012`, `VC2015`, `VC2017`, `VC2019` to build all acl libs including acl mqtt module in lib_acl_cpp module when you open the acl projects(acl_cpp_vc2003.sln, acl_cpp_vc2008.sln, acl_cpp_vc2010.sln, acl_cpp_vc2012.sln, acl_cpp_vc2015.sln, acl_cpp_vc2017.sln, acl_cpp_vc2019.sln). You should build lib_acl first, and second build lib_protocol, and third build lib_acl_cpp, and at last build all the acl samples including mqtt samples.

## **5. Reference**
The source files are in [mqtt src files](../../src/mqtt/), and the header files are in [mqtt header files](../../include/acl_cpp/mqtt/).