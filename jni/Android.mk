LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := onehook
LOCAL_ARM_MODE	:= thumb
LOCAL_LDLIBS	:= -llog -landroid_runtime -lutils -lcutils -lart -ldvm
LOCAL_CFLAGS	:= -std=gnu++11 -fpermissive -DDEBUG -O0
LOCAL_SRC_FILES := \
	JavaHook/JavaMethodHook.cpp \
	JavaHook/ArtMethodHook.cpp \
	JavaHook/DalvikMethodHook.cpp \
	JavaHook/art_quick_proxy.S \
	ElfHook/elfhook.cpp \
	ElfHook/elfio.cpp \
	ElfHook/elfutils.cpp \
	main.cpp
include $(BUILD_SHARED_LIBRARY)

