LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

SRC_PATH_SRC := .
LOCAL_MODULE := libprotocol

###########################################################

#Project's objs

BASE_SRC    := $(wildcard $(SRC_PATH_SRC)/*.c)
HTTP_SRC    := $(wildcard $(SRC_PATH_SRC)/http/*.c)
ICMP_SRC    := $(wildcard $(SRC_PATH_SRC)/icmp/*.c)
SMTP_SRC    := $(wildcard $(SRC_PATH_SRC)/smtp/*.c)

###########################################################

LOCAL_SRC_FILES := $(HTTP_SRC) $(ICMP_SRC) $(SMTP_SRC)
LOCAL_CFLAGS += -Os -fPIC -g -I.. -I../include -I../../lib_acl/include -fvisibility=hidden \
		-fdata-sections -ffunction-sections
include $(BUILD_STATIC_LIBRARY)
