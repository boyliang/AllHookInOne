#include <jni.h>

#include "art_object_4_4.h"
#include "art_func_4_4.h"
#include "common.h"
#include "JavaMethodHook.h"

using namespace art::mirror;
using namespace art;

static char* get_chars_from_utf16(const String *src) {
	size_t byte_count = src->GetUtfLength();
	char* bytes = new char[byte_count + 1];
	const uint16_t* chars = src->GetCharArray()->GetData() + src->GetOffset();
	ConvertUtf16ToModifiedUtf8(bytes, chars, src->GetLength());
	bytes[byte_count] = '\0';
	return bytes;
}

extern "C" void art_quick_dispatcher(ArtMethod*);
extern "C" uint64_t art_quick_call_entrypoint(ArtMethod* method, Thread *self, u4 **args, u4 **old_sp, const void *entrypoint);
extern "C" uint64_t artQuickToDispatcher(ArtMethod* method, Thread *self, u4 **args, u4 **old_sp){
	HookInfo *info = (HookInfo *)method->GetNativeMethod();
	LOGI("[+] entry ArtHandler %s->%s", info->classDesc, info->methodName);

	// If it not is static method, then args[0] was pointing to this
	if(!info->isStaticMethod){
		Object *thiz = reinterpret_cast<Object *>(args[0]);
		if(thiz != NULL){
			char *bytes = get_chars_from_utf16(thiz->GetClass()->GetName());
			LOGI("[+] thiz class is %s", bytes);
			delete bytes;
		}
	}

	const void *entrypoint = info->entrypoint;
	method->SetNativeMethod(info->nativecode); //restore nativecode for JNI method
	uint64_t res = art_quick_call_entrypoint(method, self, args, old_sp, entrypoint);

	JValue* result = (JValue* )&res;
	if(result != NULL){
		Object *obj = result->l;
		char *raw_class_name = get_chars_from_utf16(obj->GetClass()->GetName());

		if(strcmp(raw_class_name, "java.lang.String") == 0){
			char *raw_string_value = get_chars_from_utf16((String *)obj);
			LOGI("result-class %s, result-value \"%s\"", raw_class_name, raw_string_value);
			free(raw_string_value);
		}else{
			LOGI("result-class %s", raw_class_name);
		}

		free(raw_class_name);
	}

	// entrypoid may be replaced by trampoline, only once.
//	if(method->IsStatic() && !method->IsConstructor()){

	entrypoint = method->GetEntryPointFromCompiledCode();
	if(entrypoint != (const void *)art_quick_dispatcher){
		LOGW("[*] entrypoint was replaced. %s->%s", info->classDesc, info->methodName);

		method->SetEntryPointFromCompiledCode((const void *)art_quick_dispatcher);
		info->entrypoint = entrypoint;
		info->nativecode = method->GetNativeMethod();
	}

	method->SetNativeMethod((const void *)info);

//	}

	return res;
}

extern int __attribute__ ((visibility ("hidden"))) art_java_method_hook(JNIEnv* env, HookInfo *info) {
	const char* classDesc = info->classDesc;
	const char* methodName = info->methodName;
	const char* methodSig = info->methodSig;
	const bool isStaticMethod = info->isStaticMethod;

	// TODO we can find class by special classloader what do just like dvm
	jclass claxx = env->FindClass(classDesc);
	if(claxx == NULL){
		LOGE("[-] %s class not found", classDesc);
		return -1;
	}

	jmethodID methid = isStaticMethod ?
			env->GetStaticMethodID(claxx, methodName, methodSig) :
			env->GetMethodID(claxx, methodName, methodSig);

	if(methid == NULL){
		LOGE("[-] %s->%s method not found", classDesc, methodName);
		return -1;
	}

	ArtMethod *artmeth = reinterpret_cast<ArtMethod *>(methid);

	if(art_quick_dispatcher != artmeth->GetEntryPointFromCompiledCode()){
		uint64_t (*entrypoint)(ArtMethod* method, Object *thiz, u4 *arg1, u4 *arg2);
		entrypoint = (uint64_t (*)(ArtMethod*, Object *, u4 *, u4 *))artmeth->GetEntryPointFromCompiledCode();

		info->entrypoint = (const void *)entrypoint;
		info->nativecode = artmeth->GetNativeMethod();

		artmeth->SetEntryPointFromCompiledCode((const void *)art_quick_dispatcher);

		// save info to nativecode :)
		artmeth->SetNativeMethod((const void *)info);

		LOGI("[+] %s->%s was hooked\n", classDesc, methodName);
	}else{
		LOGW("[*] %s->%s method had been hooked", classDesc, methodName);
	}

	return 0;
}
