/*
 * dvm.h
 *
 *  Created on: 2014-9-18
 *      Author: boyliang
 */

#ifndef DVM_HOOK_H_
#define DVM_HOOK_H_

#include <pthread.h>
#include <stdio.h>
#include <assert.h>

#include "object.h"

/*
 * Get our Thread* from TLS.
 *
 * Returns NULL if this isn't a thread that the VM is aware of.
 */
Thread* dvmThreadSelf();

/*
 * Issue a method call with arguments provided in an array.  We process
 * the contents of "args" by scanning the method signature.
 *
 * The values were likely placed into an uninitialized jvalue array using
 * the field specifiers, which means that sub-32-bit fields (e.g. short,
 * boolean) may not have 32 or 64 bits of valid data.  This is different
 * from the varargs invocation where the C compiler does a widening
 * conversion when calling a function.  As a result, we have to be a
 * little more precise when pulling stuff out.
 *
 * "args" may be NULL if the method has no arguments.
 */
void dvmCallMethodA(Thread* self, const Method* method, Object* obj, bool fromJni, JValue* pResult, const jvalue* args);

/*
 * Issue a method call with a variable number of arguments.  We process
 * the contents of "args" by scanning the method signature.
 *
 * Pass in NULL for "obj" on calls to static methods.
 *
 * We don't need to take the class as an argument because, in Dalvik,
 * we don't need to worry about static synchronized methods.
 */
void dvmCallMethodV(Thread* self, const Method* method, Object* obj, bool fromJni, JValue* pResult, va_list args);

/*
 * Convert the argument to the specified type.
 *
 * Returns the width of the argument (1 for most types, 2 for J/D, -1 on
 * error).
 */
int dvmConvertArgument(DataObject* arg, ClassObject* type, s4* destPtr);

/*
 * Decode a local, global, or weak-global reference.
 */
Object* dvmDecodeIndirectRef(Thread* self, jobject jobj);

/*
 * Return whether the given object is the class Class (that is, the
 * unique class which is an instance of itself).
 */
bool dvmIsTheClassClass(const ClassObject* clazz);


/*
 * Verify that the class is an array class.
 * TODO: there may be some performance advantage to setting a flag in
 * the accessFlags field instead of chasing into the name string.
 */
bool dvmIsArrayClass(const ClassObject* clazz);

/*
 * Determine whether "instance" is an instance of "clazz".
 *
 * Returns 0 (false) if not, 1 (true) if so.
 */
int dvmInstanceof(const ClassObject* instance, const ClassObject* clazz);


/*
 * Find the named class (by descriptor), scanning through the
 * bootclasspath if it hasn't already been loaded.
 *
 * "descriptor" looks like "Landroid/debug/Stuff;".
 *
 * Uses NULL as the defining class loader.
 */
ClassObject* dvmFindSystemClass(const char* descriptor);

/*
 * Find a virtual or static method in a class.  If we don't find it, try the
 * superclass.  This is compatible with the VM spec (v2 5.4.3.3) method
 * search order, but it stops short of scanning through interfaces (which
 * should be done after this function completes).
 *
 * In most cases we know that we're looking for either a static or an
 * instance field and there's no value in searching through both types.
 * During verification we need to recognize and reject certain unusual
 * situations, and we won't see them unless we walk the lists this way.
 *
 * Returns NULL if the method can't be found.  (Does not throw an exception.)
 */
Method* dvmFindMethodHier(const ClassObject* clazz, const char* methodName, const DexProto* proto);

/*
 * Call an interpreted method from native code.  If this is being called
 * from a JNI function, references in the argument list will be converted
 * back to pointers.
 *
 * "obj" should be NULL for "direct" methods.
 */
void dvmCallMethod(Thread* self, const Method* method, Object* obj, JValue* pResult, ...);

/*
 * Invoke a method, using the specified arguments and return type, through
 * a reflection interface.
 *
 * Deals with boxing/unboxing primitives and performs widening conversions.
 *
 * "obj" should be null for a static method.
 *
 * "params" and "returnType" come from the Method object, so we don't have
 * to re-generate them from the method signature.  "returnType" should be
 * NULL if we're invoking a constructor.
 */
Object* dvmInvokeMethod(Object* invokeObj, const Method* meth, ArrayObject* argList, ArrayObject* params, ClassObject* returnType, bool noAccessCheck);

/*
 * Get a copy of the descriptor string associated with the given prototype.
 * The returned pointer must be free()ed by the caller.
 */
char* dexProtoCopyMethodDescriptor(const DexProto* pProto);


/*
 * Update method's "nativeFunc" and "insns".  If "insns" is NULL, the
 * current method->insns value is not changed.
 */
void dvmSetNativeFunc(Method* method, DalvikBridgeFunc func, const u2* insns);

/*
 * Return the class object that matches the method's signature.  For
 * primitive types, returns the box class.
 */
ClassObject* dvmGetBoxedReturnType(const Method* meth);

/*
 * Get the parameter count of the given prototype.
 */
size_t dexProtoGetParameterCount(const DexProto* pProto);

/*
 * Create a new array, given an array class.  The class may represent an
 * array of references or primitives.
 */
extern "C" ArrayObject* dvmAllocArrayByClass(ClassObject* arrayClass, size_t length, int allocFlags);


/*
 * Create a wrapper object for a primitive data type.  If "returnType" is
 * not primitive, this just casts "value" to an object and returns it.
 *
 * We could invoke the "toValue" method on the box types to take
 * advantage of pre-created values, but running that through the
 * interpreter is probably less efficient than just allocating storage here.
 *
 * The caller must call dvmReleaseTrackedAlloc on the result.
 */
DataObject* dvmBoxPrimitive(JValue value, ClassObject* returnType);

/*
 * Native function pointer type.
 *
 * "args[0]" holds the "this" pointer for virtual methods.
 *
 * The "Bridge" form is a super-set of the "Native" form; in many places
 * they are used interchangeably.  Currently, all functions have all
 * arguments passed in, but some functions only care about the first two.
 * Passing extra arguments to a C function is (mostly) harmless.
 */
typedef void (*DalvikBridgeFunc)(const u4* args, JValue* pResult, const Method* method, struct Thread* self);
typedef void (*DalvikNativeFunc)(const u4* args, JValue* pResult);

ClassObject* dvmFindPrimitiveClass(char type);

/*
 * Stop tracking an object.
 *
 * We allow attempts to delete NULL "obj" so that callers don't have to wrap
 * calls with "if != NULL".
 */
extern "C" void dvmReleaseTrackedAlloc(Object* obj, Thread* self);

/*
 * Helpful function for accessing long integers in "u4* args".
 *
 * We can't just return *(s8*)(&args[elem]), because that breaks if our
 * architecture requires 64-bit alignment of 64-bit values.
 *
 * Big/little endian shouldn't matter here -- ordering of words within a
 * long seems consistent across our supported platforms.
 */
INLINE s8 dvmGetArgLong(const u4* args, int elem)
{
    s8 val;
    memcpy(&val, &args[elem], sizeof(val));
    return val;
}

/*
 * Return a newly-allocated string for the type descriptor
 * corresponding to the "dot version" of the given class name. That
 * is, non-array names are surrounded by "L" and ";", and all
 * occurrences of '.' have been changed to '/'.
 *
 * "Dot version" names are used in the class loading machinery.
 */
char* dvmDescriptorToName(const char* str);

/* flags for dvmMalloc */
enum {
    ALLOC_DEFAULT = 0x00,
    ALLOC_DONT_TRACK = 0x01,  /* don't add to internal tracking list */
    ALLOC_NON_MOVING = 0x02,
};

#endif /* DVM_HOOK_H_ */
