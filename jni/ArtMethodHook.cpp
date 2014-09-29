#include <jni.h>
#include "JavaMethodHook.h"

extern int __attribute__ ((visibility ("hidden"))) art_java_method_hook(JNIEnv* env, HookInfo *info) {
	return -1;
}
