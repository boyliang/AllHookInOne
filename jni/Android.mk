LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := onehook
LOCAL_LDLIBS	:= -ldvm.4 -llog -landroid_runtime
LOCAL_CFLAGS	:= -DDEBUG -O0
LOCAL_SRC_FILES := \
	JavaHook/JavaMethodHook.cpp \
	JavaHook/DalvikMethodHook.cpp \
	JavaHook/ArtMethodHook.cpp \
	ElfHook/elfhook.cpp \
	ElfHook/elfio.cpp \
	ElfHook/elfutils.cpp \
	main.cpp
include $(BUILD_SHARED_LIBRARY)

