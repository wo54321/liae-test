LOCAL_PATH:= $(call my-dir)

# We need to build this for both the device (as a shared library)
# and the host (as a static library for tools to use).

common_SRC_FILES := 	\
	src/ae.c 			\
	src/anet.c 			\
	src/zmalloc.c 		\

common_CFLAGS := 		\
    -Wall 				\
    -Wno-parentheses 	\
    -Wno-switch-enum 	\
    -Wno-unused-value 	\
    -fexceptions 		\

common_C_INCLUDES += 	\
	$(LOCAL_PATH)/src 	\

common_COPY_HEADERS_TO := libae
common_COPY_HEADERS := 	\
	src/ae.h 			\
	src/anet.h 			\
	src/zmalloc.h


# For the device
# =====================================================

# Device static library
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH),arm)
    LOCAL_SDK_VERSION := 8
else
    LOCAL_SDK_VERSION := 9
endif

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_C_INCLUDES += $(common_C_INCLUDES)

LOCAL_MODULE:= libae_static
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

# Device shared library
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH),arm)
    LOCAL_SDK_VERSION := 8
else
    LOCAL_SDK_VERSION := 9
endif

LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_C_INCLUDES += $(common_C_INCLUDES)

LOCAL_MODULE:= libae
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_TAGS := optional
LOCAL_COPY_HEADERS_TO := $(common_COPY_HEADERS_TO)
LOCAL_COPY_HEADERS := $(common_COPY_HEADERS)

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))