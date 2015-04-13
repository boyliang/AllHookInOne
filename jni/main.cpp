#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "JavaHook/JavaMethodHook.h"
#include "ELFHook/elfutils.h"
#include "ElfHook/elfhook.h"
#include "common.h"


static inline void get_cstr_from_jstring(JNIEnv* env, jstring jstr, char **out) {
	jboolean iscopy = JNI_TRUE;
	const char *cstr = env->GetStringUTFChars(jstr, &iscopy);
	*out = strdup(cstr);
	env->ReleaseStringUTFChars(jstr, cstr);
}

extern "C" jint Java_com_example_allhookinone_HookUtils_hookMethodNative(JNIEnv *env, jobject thiz, jstring cls, jstring methodname, jstring methodsig, jboolean isstatic){
	HookInfo *info = (HookInfo *)malloc(sizeof(HookInfo));

	get_cstr_from_jstring(env, cls, &info->classDesc);
	get_cstr_from_jstring(env, methodname, &info->methodName);
	get_cstr_from_jstring(env, methodsig, &info->methodSig);

	info->isStaticMethod = isstatic == JNI_TRUE;
	return java_method_hook(env, info);
}


typedef int (*strlen_fun)(const char *);
strlen_fun old_strlen = NULL;

size_t my_strlen(const char *str){
	LOGI("strlen was called.");
	int len = old_strlen(str);
	return len * 2;
}


strlen_fun global_strlen1 = (strlen_fun)strlen;
strlen_fun global_strlen2 = (strlen_fun)strlen;

#define SHOW(x) LOGI("%s is %d", #x, x)

extern "C" jint Java_com_example_allhookinone_HookUtils_elfhook(JNIEnv *env, jobject thiz){
	const char *str = "helloworld";

	strlen_fun local_strlen1 = (strlen_fun)strlen;
	strlen_fun local_strlen2 = (strlen_fun)strlen;

	int len0 = global_strlen1(str);
	int len1 = global_strlen2(str);
	int len2 = local_strlen1(str);
	int len3 = local_strlen2(str);
	int len4 = strlen(str);
	int len5 = strlen(str);

	LOGI("hook before:");
	SHOW(len0);
	SHOW(len1);
	SHOW(len2);
	SHOW(len3);
	SHOW(len4);
	SHOW(len5);

	elfHook("libonehook.so", "strlen", (void *)my_strlen, (void **)&old_strlen);

	len0 = global_strlen1(str);
	len1 = global_strlen2(str);
	len2 = local_strlen1(str);
	len3 = local_strlen2(str);
	len4 = strlen(str);
	len5 = strlen(str);

	LOGI("hook after:");
	SHOW(len0);
	SHOW(len1);
	SHOW(len2);
	SHOW(len3);
	SHOW(len4);
	SHOW(len5);

	return 0;
}
