#include <cutils/properties.h>

#include "common.h"
#include "JavaMethodHook.h"

extern int dalvik_java_method_hook(JNIEnv*, HookInfo *);
extern int art_java_method_hook(JNIEnv*, HookInfo*);

static bool isArt(){
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.sys.dalvik.vm.lib", value, "");
	LOGI("[+] persist.sys.dalvik.vm.lib = %s", value);
	return strncmp(value, "libart.so", strlen("libart.so")) == 0;
}

int java_method_hook(JNIEnv* env, HookInfo *info) {
	if (!isArt()) {
		return dalvik_java_method_hook(env, info);
	} else {
		return art_java_method_hook(env, info);
	}

}

