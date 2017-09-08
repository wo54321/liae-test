LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=                                           \
	stress_test.c 											\

LOCAL_C_INCLUDES :=                                         \
    $(TOP)/bionic                                           \
    $(TOP)/bionic/libstdc++/include                         \
    $(LOCAL_PATH)/../src 									\
    $(LOCAL_PATH)/../../rcdaemon/mavlink/ 					\


LOCAL_SHARED_LIBRARIES :=      	\
    libc                       	\
    libstlport                 	\
    liblog                     	\
    libutils                   	\
    libcutils                  	\

LOCAL_STATIC_LIBRARIES :=		\
    librcutils                  \
    libae_static				\

LOCAL_CFLAGS:= -Wall -O3 -fPIC -Wattributes

LOCAL_MODULE:= ae_test

LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)
