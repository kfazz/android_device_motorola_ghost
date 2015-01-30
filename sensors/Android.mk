LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -DLOG_TAG=\"MotoSensors\"
LOCAL_SRC_FILES := SensorBase.cpp sensors.c nusensors.cpp msp430_hal.cpp
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := liblog libcutils libz
LOCAL_C_INCLUDES := external/zlib
LOCAL_MODULE := sensors.msm8960
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SRC_FILES := sensorhub.c
LOCAL_SHARED_LIBRARIES := libcutils libc
LOCAL_MODULE := sensorhub.msm8960
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_REQUIRED_MODULES := sensorhub.shamu
LOCAL_REQUIRED_MODULES += sensors.shamu
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -DLOG_TAG=\"MSP430\"
LOCAL_SRC_FILES:= msp430.cpp
LOCAL_MODULE_OWNER := google
LOCAL_MODULE:= msp430
LOCAL_SHARED_LIBRARIES := libcutils libc
include $(BUILD_EXECUTABLE)
