LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := fbbuffer
LOCAL_SRC_FILES := com_fb_screenshotapp_fbjni.c \
                   fbbuffer.c
LOCAL_LDLIBS    := -lm -llog 
include $(BUILD_SHARED_LIBRARY)

