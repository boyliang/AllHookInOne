#include <jni.h>
#include "JavaMethodHook.h"

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
