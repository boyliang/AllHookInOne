LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := onehook
LOCAL_LDLIBS	:= -ldvm.4 -llog -landroid_runtime
LOCAL_CFLAGS	:= -DDEBUG
LOCAL_SRC_FILES := \
	JavaMethodHook.cpp \
	DalvikMethodHook.cpp \
	ArtMethodHook.cpp \
	main.cpp

include $(BUILD_SHARED_LIBRARY)
