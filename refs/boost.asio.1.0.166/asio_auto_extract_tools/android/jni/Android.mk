LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(call all-subdir-makefiles)

LOCAL_MODULE    := libpseudo
#LOCAL_CFLAGS = -DFIXED_POINT -DUSE_KISS_FFT -DEXPORT="" -UHAVE_CONFIG_H
LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions -w -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_SYSTEM_NO_LIB -DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB
#-fvisibility=hidden
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include /develop/inetlibs/boost.asio.1.0.166
#LOCAL_LDFLAGS += -llog

LOCAL_SRC_FILES := pseudo.cpp

include $(BUILD_SHARED_LIBRARY)

#$(call import-module,.)

