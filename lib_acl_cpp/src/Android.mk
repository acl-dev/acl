LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

SRC_PATH := .
LOCAL_MODULE := libacl_cpp

LOCAL_SRC_FILES := $(wildcard ./*.cpp) \
	   $(wildcard $(SRC_PATH)/stdlib/*.cpp) \
	   $(wildcard $(SRC_PATH)/stream/*.cpp) \
	   $(wildcard $(SRC_PATH)/master/*.cpp) \
	   $(wildcard $(SRC_PATH)/ipc/*.cpp) \
	   $(wildcard $(SRC_PATH)/db/*.cpp) \
	   $(wildcard $(SRC_PATH)/event/*.cpp) \
	   $(wildcard $(SRC_PATH)/queue/*.cpp) \
	   $(wildcard $(SRC_PATH)/http/*.cpp) \
	   $(wildcard $(SRC_PATH)/hsocket/*.cpp) \
	   $(wildcard $(SRC_PATH)/beanstalk/*.cpp) \
	   $(wildcard $(SRC_PATH)/memcache/*.cpp) \
	   $(wildcard $(SRC_PATH)/session/*.cpp) \
	   $(wildcard $(SRC_PATH)/smtp/*.cpp) \
	   $(wildcard $(SRC_PATH)/mime/*.cpp) \
	   $(wildcard $(SRC_PATH)/mime/internal/*.cpp) \
	   $(wildcard $(SRC_PATH)/net/*.cpp) \
	   $(wildcard $(SRC_PATH)/connpool/*.cpp) \
	   $(wildcard $(SRC_PATH)/redis/*.cpp) \
	   $(wildcard $(SRC_PATH)/disque/*.cpp) \
	   $(wildcard $(SRC_PATH)/serialize/*.cpp)

LOCAL_CPP_INCLUDES += ../include
LOCAL_CPPFLAGS += -I. -I../include \
		-I../../lib_acl/include \
		-I../../lib_protocol/include \
		-I../../include/sqlite \
		-I../../include/mysql \
		-I../../include/zlib \
		-I../../include/pgsql
LOCAL_CPPFLAGS += -Os -fPIC -g -fvisibility=hidden -fvisibility-inlines-hidden \
		-fdata-sections -ffunction-sections -fexceptions \
		-DACL_CPP_LOG_SKIP_FILE -DACL_CLIENT_ONLY
include $(BUILD_STATIC_LIBRARY)
