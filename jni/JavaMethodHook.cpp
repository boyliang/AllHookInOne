#include "JavaMethodHook.h"

extern int dalvik_java_method_hook(JNIEnv*, HookInfo *);
extern int art_java_method_hook(JNIEnv*, HookInfo*);

static bool isArt(){
	return false;
}

int java_method_hook(JNIEnv* env, HookInfo *info) {
	if (!isArt()) {
		return dalvik_java_method_hook(env, info);
	} else {
		return art_java_method_hook(env, info);
	}
}

