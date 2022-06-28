LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

SRC_PATH_SRC := .
LOCAL_MODULE := libacl

###########################################################

#Project's objs
INIT_SRC     := $(wildcard $(SRC_PATH_SRC)/init/*.c)
PRIV_SRC     := $(wildcard $(SRC_PATH_SRC)/private/*.c)
STDLIB_SRC   := $(wildcard $(SRC_PATH_SRC)/stdlib/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/common/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/iostuff/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/configure/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/filedir/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/string/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/memory/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/debug/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/sys/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/sys/unix/*.c)
NET_SRC      := $(wildcard $(SRC_PATH_SRC)/net/*.c) \
		$(wildcard $(SRC_PATH_SRC)/net/listen/*.c) \
		$(wildcard $(SRC_PATH_SRC)/net/connect/*.c) \
		$(wildcard $(SRC_PATH_SRC)/net/dns/*.c)
ENGINE_SRC   := $(wildcard $(SRC_PATH_SRC)/event/*.c)
IOCTL_SRC    := $(wildcard $(SRC_PATH_SRC)/ioctl/*.c)
AIO_SRC      := $(wildcard $(SRC_PATH_SRC)/aio/*.c)
MSG_SRC      := $(wildcard $(SRC_PATH_SRC)/msg/*.c)
THREAD_SRC   := $(wildcard $(SRC_PATH_SRC)/thread/*.c)
SVR_SRC      := $(wildcard $(SRC_PATH_SRC)/svr/*.c)
DB_SRC       := $(wildcard $(SRC_PATH_SRC)/db/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/mysql/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/null/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/memdb/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/zdb/*.c)
CODE_SRC     := $(wildcard $(SRC_PATH_SRC)/code/*.c) 
MASTER_SRC   := $(wildcard $(SRC_PATH_SRC)/master/*.c) \
		$(wildcard $(SRC_PATH_SRC)/master/template/*.c)
PROCTL_SRC   := $(wildcard $(SRC_PATH_SRC)/proctl/*.c)
XML_SRC      := $(wildcard $(SRC_PATH_SRC)/xml/*.c)
JSON_SRC     := $(wildcard $(SRC_PATH_SRC)/json/*.c)
UTEST_SRC    := $(wildcard $(SRC_PATH_SRC)/unit_test/*.c)

###########################################################

LOCAL_SRC_FILES := $(INIT_SRC) $(PRIV_SRC) $(STDLIB_SRC) $(NET_SRC) \
		$(ENGINE_SRC) $(IOCTL_SRC) $(AIO_SRC) $(MSG_SRC) \
		$(THREAD_SRC) $(SVR_SRC) $(DB_SRC) $(CODE_SRC) \
		$(MASTER_SRC) $(PROCTL_SRC) $(XML_SRC) $(JSON_SRC) \
		$(UTEST_SRC)

LOCAL_CFLAGS  += -Os -fPIC -g -I.. -I../include -fvisibility=hidden -fdata-sections -ffunction-sections
include $(BUILD_STATIC_LIBRARY)
