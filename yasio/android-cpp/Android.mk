LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lyasio_static

LOCAL_MODULE_FILENAME := liblyasio

#LOCAL_CFLAGS = -DFIXED_POINT -DUSE_KISS_FFT -DEXPORT="" -UHAVE_CONFIG_H
LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_SYSTEM_NO_LIB -DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB
#-fvisibility=hidden

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../lua/luajit/include

LOCAL_SRC_FILES := ../xxsocket.cpp \
    ../yasio.cpp \
    ../ibinarystream.cpp \
    ../obinarystream.cpp

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
