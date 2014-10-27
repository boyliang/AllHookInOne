/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Declaration of the fundamental Object type and refinements thereof, plus
 * some functions for manipulating them.
 */
#ifndef _DALVIK_OO_OBJECT_4_H__
#define _DALVIK_OO_OBJECT_4_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <jni.h>

#ifndef INLINE
# define INLINE extern __inline__
#endif

/*
 * These match the definitions in the VM specification.
 */

#ifdef HAVE_STDINT_H
# include <stdint.h>    /* C99 */
typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;
typedef int8_t s1;
typedef int16_t s2;
typedef int32_t s4;
typedef int64_t s8;
#else
typedef unsigned char u1;
typedef unsigned short u2;
typedef unsigned int u4;
typedef unsigned long long u8;
typedef signed char s1;
typedef signed short s2;
typedef signed int s4;
typedef signed long long s8;
#endif

/* fwd decl */
struct DataObject;
struct InitiatingLoaderList;
struct ClassObject;
struct StringObject;
struct ArrayObject;
struct Method;
struct ExceptionEntry;
struct LineNumEntry;
struct StaticField;
struct InstField;
struct Field;
struct RegisterMap;
struct DvmDex;
struct DexFile;
struct Object;
struct Thread;

union JValue {
	u1 z;
	s1 b;
	u2 c;
	s2 s;
	s4 i;
	s8 j;
	float f;
	double d;
	void* l;
};

/*
 * Storage for primitive types and object references.
 *
 * Some parts of the code (notably object field access) assume that values
 * are "left aligned", i.e. given "JValue jv", "jv.i" and "*((s4*)&jv)"
 * yield the same result.  This seems to be guaranteed by gcc on big- and
 * little-endian systems.
 */

# define STRING_FIELDOFF_VALUE      8
# define STRING_FIELDOFF_HASHCODE   12
# define STRING_FIELDOFF_OFFSET     16
# define STRING_FIELDOFF_COUNT      20


/*
 * Enumeration of all the primitive types.
 */
enum PrimitiveType {
	PRIM_NOT = 0, /* value is a reference type, not a primitive type */
	PRIM_VOID = 1, PRIM_BOOLEAN = 2, PRIM_BYTE = 3, PRIM_SHORT = 4, PRIM_CHAR = 5, PRIM_INT = 6, PRIM_LONG = 7, PRIM_FLOAT = 8, PRIM_DOUBLE = 9,
};

enum {
	ACC_PUBLIC = 0x00000001,       // class, field, method, ic
	ACC_PRIVATE = 0x00000002,       // field, method, ic
	ACC_PROTECTED = 0x00000004,       // field, method, ic
	ACC_STATIC = 0x00000008,       // field, method, ic
	ACC_FINAL = 0x00000010,       // class, field, method, ic
	ACC_SYNCHRONIZED = 0x00000020,       // method (only allowed on natives)
	ACC_SUPER = 0x00000020,       // class (not used in Dalvik)
	ACC_VOLATILE = 0x00000040,       // field
	ACC_BRIDGE = 0x00000040,       // method (1.5)
	ACC_TRANSIENT = 0x00000080,       // field
	ACC_VARARGS = 0x00000080,       // method (1.5)
	ACC_NATIVE = 0x00000100,       // method
	ACC_INTERFACE = 0x00000200,       // class, ic
	ACC_ABSTRACT = 0x00000400,       // class, method, ic
	ACC_STRICT = 0x00000800,       // method
	ACC_SYNTHETIC = 0x00001000,       // field, method, ic
	ACC_ANNOTATION = 0x00002000,       // class, ic (1.5)
	ACC_ENUM = 0x00004000,       // class, field, ic (1.5)
	ACC_CONSTRUCTOR = 0x00010000,       // method (Dalvik only)
	ACC_DECLARED_SYNCHRONIZED = 0x00020000,       // method (Dalvik only)
	ACC_CLASS_MASK = (ACC_PUBLIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM),
	ACC_INNER_CLASS_MASK = (ACC_CLASS_MASK | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC),
	ACC_FIELD_MASK = (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL | ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM),
	ACC_METHOD_MASK = (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL | ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE | ACC_ABSTRACT | ACC_STRICT | ACC_SYNTHETIC | ACC_CONSTRUCTOR | ACC_DECLARED_SYNCHRONIZED),
};

/*
 * Native function return type; used by dvmPlatformInvoke().
 *
 * This is part of Method.jniArgInfo, and must fit in 3 bits.
 * Note: Assembly code in arch/<arch>/Call<arch>.S relies on
 * the enum values defined here.
 */
enum DalvikJniReturnType {
	DALVIK_JNI_RETURN_VOID = 0, /* must be zero */
	DALVIK_JNI_RETURN_FLOAT = 1, DALVIK_JNI_RETURN_DOUBLE = 2, DALVIK_JNI_RETURN_S8 = 3, DALVIK_JNI_RETURN_S4 = 4, DALVIK_JNI_RETURN_S2 = 5, DALVIK_JNI_RETURN_U2 = 6, DALVIK_JNI_RETURN_S1 = 7
};

#define DALVIK_JNI_NO_ARG_INFO  0x80000000
#define DALVIK_JNI_RETURN_MASK  0x70000000
#define DALVIK_JNI_RETURN_SHIFT 28
#define DALVIK_JNI_COUNT_MASK   0x0f000000
#define DALVIK_JNI_COUNT_SHIFT  24

// #define PAD_SAVE_AREA       /* help debug stack trampling */

// #define EASY_GDB

/*
 * The VM-specific internal goop.
 *
 * The idea is to mimic a typical native stack frame, with copies of the
 * saved PC and FP.  At some point we'd like to have interpreted and
 * native code share the same stack, though this makes portability harder.
 */
struct StackSaveArea {
#ifdef PAD_SAVE_AREA
	u4 pad0, pad1, pad2;
#endif

#ifdef EASY_GDB
	/* make it easier to trek through stack frames in GDB */
	StackSaveArea* prevSave;
#endif

	/* saved frame pointer for previous frame, or NULL if this is at bottom */
	u4* prevFrame;

	/* saved program counter (from method in caller's frame) */
	const u2* savedPc;

	/* pointer to method we're *currently* executing; handy for exceptions */
	const Method* method;

#ifdef TDROID
	u4 argsCount;
#endif

	union {
		/* for JNI native methods: bottom of local reference segment */
		u4 localRefCookie;

		/* for interpreted methods: saved current PC, for exception stack
		 * traces and debugger traces */
		const u2* currentPc;
	} xtra;

	/* Native return pointer for JIT, or 0 if interpreted */
	const u2* returnAddr;
#ifdef PAD_SAVE_AREA
	u4 pad3, pad4, pad5;
#endif
};

/* move between the stack save area and the frame pointer */
#define SAVEAREA_FROM_FP(_fp)   ((StackSaveArea*)(_fp) -1)
#define FP_FROM_SAVEAREA(_save) ((u4*) ((StackSaveArea*)(_save) +1))

/* when calling a function, get a pointer to outs[0] */
#define OUTS_FROM_FP(_fp, _argCount) \
    ((u4*) ((u1*)SAVEAREA_FROM_FP(_fp) - sizeof(u4) * (_argCount)))

/*
 * Method prototype structure, which refers to a protoIdx in a
 * particular DexFile.
 */
struct DexProto {
	const void* dexFile; /* file the idx refers to */ // const DexFile
	u4 protoIdx; /* index into proto_ids table of dexFile */
};

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

/* vm-internal access flags and related definitions */
enum AccessFlags {
	ACC_MIRANDA = 0x8000,       // method (internal to VM)
	JAVA_FLAGS_MASK = 0xffff,       // bits set from Java sources (low 16)
};

/* Use the top 16 bits of the access flags field for
 * other class flags.  Code should use the *CLASS_FLAG*()
 * macros to set/get these flags.
 */
enum ClassFlags {
	CLASS_ISFINALIZABLE = (1 << 31), // class/ancestor overrides finalize()
	CLASS_ISARRAY = (1 << 30), // class is a "[*"
	CLASS_ISOBJECTARRAY = (1 << 29), // class is a "[L*" or "[[*"
	CLASS_ISCLASS = (1 << 28), // class is *the* class Class

	CLASS_ISREFERENCE = (1 << 27), // class is a soft/weak/phantom ref
								   // only ISREFERENCE is set --> soft
	CLASS_ISWEAKREFERENCE = (1 << 26), // class is a weak reference
	CLASS_ISFINALIZERREFERENCE = (1 << 25), // class is a finalizer reference
	CLASS_ISPHANTOMREFERENCE = (1 << 24), // class is a phantom reference

	CLASS_MULTIPLE_DEFS = (1 << 23), // DEX verifier: defs in multiple DEXs

	/* unlike the others, these can be present in the optimized DEX file */
	CLASS_ISOPTIMIZED = (1 << 17), // class may contain opt instrs
	CLASS_ISPREVERIFIED = (1 << 16), // class has been pre-verified
};

/* bits we can reasonably expect to see set in a DEX access flags field */
#define EXPECTED_FILE_FLAGS \
    (ACC_CLASS_MASK | CLASS_ISPREVERIFIED | CLASS_ISOPTIMIZED)

/*
 * Get/set class flags.
 */
#define SET_CLASS_FLAG(clazz, flag) \
    do { (clazz)->accessFlags |= (flag); } while (0)

#define CLEAR_CLASS_FLAG(clazz, flag) \
    do { (clazz)->accessFlags &= ~(flag); } while (0)

#define IS_CLASS_FLAG_SET(clazz, flag) \
    (((clazz)->accessFlags & (flag)) != 0)

#define GET_CLASS_FLAG_GROUP(clazz, flags) \
    ((u4)((clazz)->accessFlags & (flags)))

#define RETURN_VOID()           do { (void)(pResult); return; } while(0)
#define RETURN_BOOLEAN(_val)    do { pResult->i = (_val); return; } while(0)
#define RETURN_INT(_val)        do { pResult->i = (_val); return; } while(0)
#define RETURN_LONG(_val)       do { pResult->j = (_val); return; } while(0)
#define RETURN_FLOAT(_val)      do { pResult->f = (_val); return; } while(0)
#define RETURN_DOUBLE(_val)     do { pResult->d = (_val); return; } while(0)
#define RETURN_PTR(_val)        do { pResult->l = (Object*)(_val); return; } while(0)
/*
 * Use the top 16 bits of the access flags field for other method flags.
 * Code should use the *METHOD_FLAG*() macros to set/get these flags.
 */
enum MethodFlags {
	METHOD_ISWRITABLE = (1 << 31),  // the method's code is writable
};

/*
 * Get/set method flags.
 */
#define SET_METHOD_FLAG(method, flag) \
    do { (method)->accessFlags |= (flag); } while (0)

#define CLEAR_METHOD_FLAG(method, flag) \
    do { (method)->accessFlags &= ~(flag); } while (0)

#define IS_METHOD_FLAG_SET(method, flag) \
    (((method)->accessFlags & (flag)) != 0)

#define GET_METHOD_FLAG_GROUP(method, flags) \
    ((u4)((method)->accessFlags & (flags)))

/* current state of the class, increasing as we progress */
enum ClassStatus {
	CLASS_ERROR = -1,

	CLASS_NOTREADY = 0, CLASS_IDX = 1, /* loaded, DEX idx in super or ifaces */
	CLASS_LOADED = 2, /* DEX idx values resolved */
	CLASS_RESOLVED = 3, /* part of linking */
	CLASS_VERIFYING = 4, /* in the process of being verified */
	CLASS_VERIFIED = 5, /* logically part of linking; done pre-init */
	CLASS_INITIALIZING = 6, /* class init in progress */
	CLASS_INITIALIZED = 7, /* ready to go */
};

/*
 * Used for iftable in ClassObject.
 */
struct InterfaceEntry {
	/* pointer to interface class */
	ClassObject* clazz;

	/*
	 * Index into array of vtable offsets.  This points into the ifviPool,
	 * which holds the vtables for all interfaces declared by this class.
	 */
	int* methodIndexArray;
};

/*
 * There are three types of objects:
 *  Class objects - an instance of java.lang.Class
 *  Array objects - an object created with a "new array" instruction
 *  Data objects - an object that is neither of the above
 *
 * We also define String objects.  At present they're equivalent to
 * DataObject, but that may change.  (Either way, they make some of the
 * code more obvious.)
 *
 * All objects have an Object header followed by type-specific data.
 */
struct Object {
	/* ptr to class object */
	ClassObject* clazz;

	/*
	 * A word containing either a "thin" lock or a "fat" monitor.  See
	 * the comments in Sync.c for a description of its layout.
	 */
	u4 lock;
};

/*
 * Properly initialize an Object.
 * void DVM_OBJECT_INIT(Object *obj, ClassObject *clazz_)
 */
#define DVM_OBJECT_INIT(obj, clazz_) \
    dvmSetFieldObject(obj, OFFSETOF_MEMBER(Object, clazz), clazz_)

/*
 * Data objects have an Object header followed by their instance data.
 */
struct DataObject: Object {
	/* variable #of u4 slots; u8 uses 2 slots */
	u4 instanceData[1];
};

/*
 * Strings are used frequently enough that we may want to give them their
 * own unique type.
 *
 * Using a dedicated type object to access the instance data provides a
 * performance advantage but makes the java/lang/String.java implementation
 * fragile.
 *
 * Currently this is just equal to DataObject, and we pull the fields out
 * like we do for any other object.
 */
struct StringObject: Object {
	/* variable #of u4 slots; u8 uses 2 slots */
	u4 instanceData[1];

	/** Returns this string's length in characters. */
	int length() const;

	/**
	 * Returns this string's length in bytes when encoded as modified UTF-8.
	 * Does not include a terminating NUL byte.
	 */
	int utfLength() const;

	/** Returns this string's char[] as an ArrayObject. */
	ArrayObject* array() const;

	/** Returns this string's char[] as a u2*. */
	const u2* chars() const;
};

/*
 * Array objects have these additional fields.
 *
 * We don't currently store the size of each element.  Usually it's implied
 * by the instruction.  If necessary, the width can be derived from
 * the first char of obj->clazz->descriptor.
 */
struct ArrayObject: Object {
	/* number of elements; immutable after init */
	u4 length;

	/*
	 * Array contents; actual size is (length * sizeof(type)).  This is
	 * declared as u8 so that the compiler inserts any necessary padding
	 * (e.g. for EABI); the actual allocation may be smaller than 8 bytes.
	 */
	u8 contents[1];
};

/*
 * For classes created early and thus probably in the zygote, the
 * InitiatingLoaderList is kept in gDvm. Later classes use the structure in
 * Object Class. This helps keep zygote pages shared.
 */
struct InitiatingLoaderList {
	/* a list of initiating loader Objects; grown and initialized on demand */
	Object** initiatingLoaders;
	/* count of loaders in the above list */
	int initiatingLoaderCount;
};

/*
 * Generic field header.  We pass this around when we want a generic Field
 * pointer (e.g. for reflection stuff).  Testing the accessFlags for
 * ACC_STATIC allows a proper up-cast.
 */
struct Field {
	ClassObject* clazz; /* class in which the field is declared */
	const char* name;
	const char* signature; /* e.g. "I", "[C", "Landroid/os/Debug;" */
	u4 accessFlags;
};

/*
 * Static field.
 */
struct StaticField: Field {
	JValue value; /* initially set from DEX for primitives */
};

/*
 * Instance field.
 */
struct InstField: Field {
	/*
	 * This field indicates the byte offset from the beginning of the
	 * (Object *) to the actual instance data; e.g., byteOffset==0 is
	 * the same as the object pointer (bug!), and byteOffset==4 is 4
	 * bytes farther.
	 */
	int byteOffset;
};

/*
 * This defines the amount of space we leave for field slots in the
 * java.lang.Class definition.  If we alter the class to have more than
 * this many fields, the VM will abort at startup.
 */
#define CLASS_FIELD_SLOTS   4

/*
 * Class objects have many additional fields.  This is used for both
 * classes and interfaces, including synthesized classes (arrays and
 * primitive types).
 *
 * Class objects are unusual in that they have some fields allocated with
 * the system malloc (or LinearAlloc), rather than on the GC heap.  This is
 * handy during initialization, but does require special handling when
 * discarding java.lang.Class objects.
 *
 * The separation of methods (direct vs. virtual) and fields (class vs.
 * instance) used in Dalvik works out pretty well.  The only time it's
 * annoying is when enumerating or searching for things with reflection.
 */
struct ClassObject: Object {
	/* leave space for instance data; we could access fields directly if we
	 freeze the definition of java/lang/Class */

#ifdef TDROID
	u4 instanceData[CLASS_FIELD_SLOTS * 2];
#else
	u4 instanceData[CLASS_FIELD_SLOTS];
#endif

	/* UTF-8 descriptor for the class; from constant pool, or on heap
	 if generated ("[C") */
	const char* descriptor;
	char* descriptorAlloc;

	/* access flags; low 16 bits are defined by VM spec */
	u4 accessFlags;

	/* VM-unique class serial number, nonzero, set very early */
	u4 serialNumber;

	/* DexFile from which we came; needed to resolve constant pool entries */
	/* (will be NULL for VM-generated, e.g. arrays and primitive classes) */
	DvmDex* pDvmDex;

	/* state of class initialization */
	ClassStatus status;

	/* if class verify fails, we must return same error on subsequent tries */
	ClassObject* verifyErrorClass;

	/* threadId, used to check for recursive <clinit> invocation */
	u4 initThreadId;

	/*
	 * Total object size; used when allocating storage on gc heap.  (For
	 * interfaces and abstract classes this will be zero.)
	 */
	size_t objectSize;

	/* arrays only: class object for base element, for instanceof/checkcast
	 (for String[][][], this will be String) */
	ClassObject* elementClass;

	/* arrays only: number of dimensions, e.g. int[][] is 2 */
	int arrayDim;

	/* primitive type index, or PRIM_NOT (-1); set for generated prim classes */
	PrimitiveType primitiveType;

	/* superclass, or NULL if this is java.lang.Object */
	ClassObject* super;

	/* defining class loader, or NULL for the "bootstrap" system loader */
	Object* classLoader;

	/* initiating class loader list */
	/* NOTE: for classes with low serialNumber, these are unused, and the
	 values are kept in a table in gDvm. */
	InitiatingLoaderList initiatingLoaderList;

	/* array of interfaces this class implements directly */
	int interfaceCount;
	ClassObject** interfaces;

	/* static, private, and <init> methods */
	int directMethodCount;
	Method* directMethods;

	/* virtual methods defined in this class; invoked through vtable */
	int virtualMethodCount;
	Method* virtualMethods;

	/*
	 * Virtual method table (vtable), for use by "invoke-virtual".  The
	 * vtable from the superclass is copied in, and virtual methods from
	 * our class either replace those from the super or are appended.
	 */
	int vtableCount;
	Method** vtable;

	/*
	 * Interface table (iftable), one entry per interface supported by
	 * this class.  That means one entry for each interface we support
	 * directly, indirectly via superclass, or indirectly via
	 * superinterface.  This will be null if neither we nor our superclass
	 * implement any interfaces.
	 *
	 * Why we need this: given "class Foo implements Face", declare
	 * "Face faceObj = new Foo()".  Invoke faceObj.blah(), where "blah" is
	 * part of the Face interface.  We can't easily use a single vtable.
	 *
	 * For every interface a concrete class implements, we create a list of
	 * virtualMethod indices for the methods in the interface.
	 */
	int iftableCount;
	InterfaceEntry* iftable;

	/*
	 * The interface vtable indices for iftable get stored here.  By placing
	 * them all in a single pool for each class that implements interfaces,
	 * we decrease the number of allocations.
	 */
	int ifviPoolCount;
	int* ifviPool;

	/* instance fields
	 *
	 * These describe the layout of the contents of a DataObject-compatible
	 * Object.  Note that only the fields directly defined by this class
	 * are listed in ifields;  fields defined by a superclass are listed
	 * in the superclass's ClassObject.ifields.
	 *
	 * All instance fields that refer to objects are guaranteed to be
	 * at the beginning of the field list.  ifieldRefCount specifies
	 * the number of reference fields.
	 */
	int ifieldCount;
	int ifieldRefCount; // number of fields that are object refs
	InstField* ifields;

	/* bitmap of offsets of ifields */
	u4 refOffsets;

	/* source file name, if known */
	const char* sourceFile;

	/* static fields */
	int sfieldCount;
	StaticField sfields[]; /* MUST be last item */
};

/*
 * A method.  We create one of these for every method in every class
 * we load, so try to keep the size to a minimum.
 *
 * Much of this comes from and could be accessed in the data held in shared
 * memory.  We hold it all together here for speed.  Everything but the
 * pointers could be held in a shared table generated by the optimizer;
 * if we're willing to convert them to offsets and take the performance
 * hit (e.g. "meth->insns" becomes "baseAddr + meth->insnsOffset") we
 * could move everything but "nativeFunc".
 */
struct Method {
	/* the class we are a part of */
	ClassObject* clazz;

	/* access flags; low 16 bits are defined by spec (could be u2?) */
	u4 accessFlags;

	/*
	 * For concrete virtual methods, this is the offset of the method
	 * in "vtable".
	 *
	 * For abstract methods in an interface class, this is the offset
	 * of the method in "iftable[n]->methodIndexArray".
	 */
	u2 methodIndex;

	/*
	 * Method bounds; not needed for an abstract method.
	 *
	 * For a native method, we compute the size of the argument list, and
	 * set "insSize" and "registerSize" equal to it.
	 */
	u2 registersSize; /* ins + locals */
	u2 outsSize;
	u2 insSize;

	/* method name, e.g. "<init>" or "eatLunch" */
	const char* name;

	/*
	 * Method prototype descriptor string (return and argument types).
	 *
	 * TODO: This currently must specify the DexFile as well as the proto_ids
	 * index, because generated Proxy classes don't have a DexFile.  We can
	 * remove the DexFile* and reduce the size of this struct if we generate
	 * a DEX for proxies.
	 */

	DexProto prototype;
	/* short-form method descriptor string */
	const char* shorty;

	/*
	 * The remaining items are not used for abstract or native methods.
	 * (JNI is currently hijacking "insns" as a function pointer, set
	 * after the first call.  For internal-native this stays null.)
	 */

	/* the actual code */
	const u2* insns; /* instructions, in memory-mapped .dex */

	/* JNI: cached argument and return-type hints */
	int jniArgInfo;

	/*
	 * JNI: native method ptr; could be actual function or a JNI bridge.  We
	 * don't currently discriminate between DalvikBridgeFunc and
	 * DalvikNativeFunc; the former takes an argument superset (i.e. two
	 * extra args) which will be ignored.  If necessary we can use
	 * insns==NULL to detect JNI bridge vs. internal native.
	 */
	DalvikBridgeFunc nativeFunc;

	/*
	 * JNI: true if this static non-synchronized native method (that has no
	 * reference arguments) needs a JNIEnv* and jclass/jobject. Libcore
	 * uses this.
	 */
	bool fastJni;

	/*
	 * JNI: true if this method has no reference arguments. This lets the JNI
	 * bridge avoid scanning the shorty for direct pointers that need to be
	 * converted to local references.
	 *
	 * TODO: replace this with a list of indexes of the reference arguments.
	 */
	bool noRef;

	/*
	 * JNI: true if we should log entry and exit. This is the only way
	 * developers can log the local references that are passed into their code.
	 * Used for debugging JNI problems in third-party code.
	 */
	bool shouldTrace;

	/*
	 * Register map data, if available.  This will point into the DEX file
	 * if the data was computed during pre-verification, or into the
	 * linear alloc area if not.
	 */
	const RegisterMap* registerMap;

	/* set if method was called during method profiling */
	bool inProfile;
};

INLINE bool dvmIsPublicMethod(const Method* method) {
	return (method->accessFlags & ACC_PUBLIC) != 0;
}

INLINE bool dvmIsProtectedMethod(const Method* method) {
	return (method->accessFlags & ACC_PROTECTED) != 0;
}

INLINE bool dvmIsPrivateMethod(const Method* method) {
	return (method->accessFlags & ACC_PRIVATE) != 0;
}
INLINE bool dvmIsStaticMethod(const Method* method) {
	return (method->accessFlags & ACC_STATIC) != 0;
}
INLINE bool dvmIsSynchronizedMethod(const Method* method) {
	return (method->accessFlags & ACC_SYNCHRONIZED) != 0;
}
INLINE bool dvmIsDeclaredSynchronizedMethod(const Method* method) {
	return (method->accessFlags & ACC_DECLARED_SYNCHRONIZED) != 0;
}
INLINE bool dvmIsFinalMethod(const Method* method) {
	return (method->accessFlags & ACC_FINAL) != 0;
}
INLINE bool dvmIsNativeMethod(const Method* method) {
	return (method->accessFlags & ACC_NATIVE) != 0;
}
INLINE bool dvmIsAbstractMethod(const Method* method) {
	return (method->accessFlags & ACC_ABSTRACT) != 0;
}
INLINE bool dvmIsSyntheticMethod(const Method* method) {
	return (method->accessFlags & ACC_SYNTHETIC) != 0;
}
INLINE bool dvmIsMirandaMethod(const Method* method) {
	return (method->accessFlags & ACC_MIRANDA) != 0;
}
INLINE bool dvmIsConstructorMethod(const Method* method) {
	return *method->name == '<';
}

#define BYTE_OFFSET(_ptr, _offset)  ((void*) (((u1*)(_ptr)) + (_offset)))

INLINE JValue* dvmFieldPtr(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) );
}

INLINE bool dvmGetFieldBoolean(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->z;
}
INLINE s1 dvmGetFieldByte(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->b;
}
INLINE s2 dvmGetFieldShort(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->s;
}
INLINE u2 dvmGetFieldChar(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->c;
}
INLINE s4 dvmGetFieldInt(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->i;
}
INLINE s8 dvmGetFieldLong(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->j;
}
INLINE float dvmGetFieldFloat(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->f;
}
INLINE double dvmGetFieldDouble(const Object* obj, int offset) {
	return ((JValue*) BYTE_OFFSET(obj, offset) )->d;
}
INLINE Object* dvmGetFieldObject(const Object* obj, int offset) {
	return (Object*) (((JValue*) BYTE_OFFSET(obj, offset) )->l);
}

INLINE float dvmU4ToFloat(u4 val) {
    union { u4 in; float out; } conv;
    conv.in = val;
    return conv.out;
}


struct InterpSaveState {
    const u2*       pc;         // Dalvik PC
    u4*             curFrame;   // Dalvik frame pointer
    const Method    *method;    // Method being executed
    DvmDex*         methodClassDex;
    JValue          retval;
    void*           bailPtr;
#if defined(WITH_TRACKREF_CHECKS)
    int             debugTrackedRefStart;
#else
    int             unused;        // Keep struct size constant
#endif
    struct InterpSaveState* prev;  // To follow nested activations
} __attribute__ ((__packed__));

/*
 * Interpreter control struction.  Packed into a long long to enable
 * atomic updates.
 */
union InterpBreak {
    volatile int64_t   all;
    struct {
        uint16_t   subMode;
        uint8_t    breakFlags;
        int8_t     unused;   /* for future expansion */
#ifndef DVM_NO_ASM_INTERP
        void* curHandlerTable;
#else
        int32_t    unused1;
#endif
    } ctl;
};

/*
 * Information we store for each slot in the reference table.
 */
struct IndirectRefSlot {
    Object* obj;        /* object pointer itself, NULL if the slot is unused */
    u4      serial;     /* slot serial number */
};


union IRTSegmentState {
    u4          all;
    struct {
        u4      topIndex:16;            /* index of first unused entry */
        u4      numHoles:16;            /* #of holes in entire table */
    } parts;
};

class iref_iterator {

private:
    IndirectRefSlot* table_;
    size_t i_;
    size_t capacity_;
};

enum IndirectRefKind {
    kIndirectKindInvalid    = 0,
    kIndirectKindLocal      = 1,
    kIndirectKindGlobal     = 2,
    kIndirectKindWeakGlobal = 3
};

struct IndirectRefTable {
public:
    typedef iref_iterator iterator;

    /* semi-public - read/write by interpreter in native call handler */
    IRTSegmentState segmentState;

    /*
     * private:
     *
     * TODO: we can't make these private as long as the interpreter
     * uses offsetof, since private member data makes us non-POD.
     */
    /* bottom of the stack */
    IndirectRefSlot* table_;
    /* bit mask, ORed into all irefs */
    IndirectRefKind kind_;
    /* #of entries we have space for */
    size_t          alloc_entries_;
    /* max #of entries allowed */
    size_t          max_entries_;
};

enum ThreadStatus {
    THREAD_UNDEFINED    = -1,       /* makes enum compatible with int32_t */

    /* these match up with JDWP values */
    THREAD_ZOMBIE       = 0,        /* TERMINATED */
    THREAD_RUNNING      = 1,        /* RUNNABLE or running now */
    THREAD_TIMED_WAIT   = 2,        /* TIMED_WAITING in Object.wait() */
    THREAD_MONITOR      = 3,        /* BLOCKED on a monitor */
    THREAD_WAIT         = 4,        /* WAITING in Object.wait() */
    /* non-JDWP states */
    THREAD_INITIALIZING = 5,        /* allocated, not yet running */
    THREAD_STARTING     = 6,        /* started, not yet on thread list */
    THREAD_NATIVE       = 7,        /* off in a JNI native method */
    THREAD_VMWAIT       = 8,        /* waiting on a VM resource */
    THREAD_SUSPENDED    = 9,        /* suspended, usually by GC or debugger */
};

typedef bool (*SafePointCallback)(struct Thread* thread, void* arg);

/*
 * Error constants.
 */
enum JdwpError {
    ERR_NONE                                        = 0,
    ERR_INVALID_THREAD                              = 10,
    ERR_INVALID_THREAD_GROUP                        = 11,
    ERR_INVALID_PRIORITY                            = 12,
    ERR_THREAD_NOT_SUSPENDED                        = 13,
    ERR_THREAD_SUSPENDED                            = 14,
    ERR_INVALID_OBJECT                              = 20,
    ERR_INVALID_CLASS                               = 21,
    ERR_CLASS_NOT_PREPARED                          = 22,
    ERR_INVALID_METHODID                            = 23,
    ERR_INVALID_LOCATION                            = 24,
    ERR_INVALID_FIELDID                             = 25,
    ERR_INVALID_FRAMEID                             = 30,
    ERR_NO_MORE_FRAMES                              = 31,
    ERR_OPAQUE_FRAME                                = 32,
    ERR_NOT_CURRENT_FRAME                           = 33,
    ERR_TYPE_MISMATCH                               = 34,
    ERR_INVALID_SLOT                                = 35,
    ERR_DUPLICATE                                   = 40,
    ERR_NOT_FOUND                                   = 41,
    ERR_INVALID_MONITOR                             = 50,
    ERR_NOT_MONITOR_OWNER                           = 51,
    ERR_INTERRUPT                                   = 52,
    ERR_INVALID_CLASS_FORMAT                        = 60,
    ERR_CIRCULAR_CLASS_DEFINITION                   = 61,
    ERR_FAILS_VERIFICATION                          = 62,
    ERR_ADD_METHOD_NOT_IMPLEMENTED                  = 63,
    ERR_SCHEMA_CHANGE_NOT_IMPLEMENTED               = 64,
    ERR_INVALID_TYPESTATE                           = 65,
    ERR_HIERARCHY_CHANGE_NOT_IMPLEMENTED            = 66,
    ERR_DELETE_METHOD_NOT_IMPLEMENTED               = 67,
    ERR_UNSUPPORTED_VERSION                         = 68,
    ERR_NAMES_DONT_MATCH                            = 69,
    ERR_CLASS_MODIFIERS_CHANGE_NOT_IMPLEMENTED      = 70,
    ERR_METHOD_MODIFIERS_CHANGE_NOT_IMPLEMENTED     = 71,
    ERR_NOT_IMPLEMENTED                             = 99,
    ERR_NULL_POINTER                                = 100,
    ERR_ABSENT_INFORMATION                          = 101,
    ERR_INVALID_EVENT_TYPE                          = 102,
    ERR_ILLEGAL_ARGUMENT                            = 103,
    ERR_OUT_OF_MEMORY                               = 110,
    ERR_ACCESS_DENIED                               = 111,
    ERR_VM_DEAD                                     = 112,
    ERR_INTERNAL                                    = 113,
    ERR_UNATTACHED_THREAD                           = 115,
    ERR_INVALID_TAG                                 = 500,
    ERR_ALREADY_INVOKING                            = 502,
    ERR_INVALID_INDEX                               = 503,
    ERR_INVALID_LENGTH                              = 504,
    ERR_INVALID_STRING                              = 506,
    ERR_INVALID_CLASS_LOADER                        = 507,
    ERR_INVALID_ARRAY                               = 508,
    ERR_TRANSPORT_LOAD                              = 509,
    ERR_TRANSPORT_INIT                              = 510,
    ERR_NATIVE_METHOD                               = 511,
    ERR_INVALID_COUNT                               = 512,
};
typedef u8 ObjectId;

struct DebugInvokeReq {
    /* boolean; only set when we're in the tail end of an event handler */
    bool ready;

    /* boolean; set if the JDWP thread wants this thread to do work */
    bool invokeNeeded;

    /* request */
    Object* obj;        /* not used for ClassType.InvokeMethod */
    Object* thread;
    ClassObject* clazz;
    Method* method;
    u4 numArgs;
    u8* argArray;   /* will be NULL if numArgs==0 */
    u4 options;

    /* result */
    JdwpError err;
    u1 resultTag;
    JValue resultValue;
    ObjectId exceptObj;

    /* condition variable to wait on while the method executes */
    pthread_mutex_t lock;
    pthread_cond_t cv;
};

struct AllocProfState {
    bool    enabled;            // is allocation tracking enabled?

    int     allocCount;         // #of objects allocated
    int     allocSize;          // cumulative size of objects

    int     failedAllocCount;   // #of times an allocation failed
    int     failedAllocSize;    // cumulative size of failed allocations

    int     freeCount;          // #of objects freed
    int     freeSize;           // cumulative size of freed objects

    int     gcCount;            // #of times an allocation triggered a GC

    int     classInitCount;     // #of initialized classes
    u8      classInitTime;      // cumulative time spent in class init (nsec)
};

struct Monitor;

struct ReferenceTable {
    Object**        nextEntry;          /* top of the list */
    Object**        table;              /* bottom of the list */

    int             allocEntries;       /* #of entries we have space for */
    int             maxEntries;         /* max #of entries allowed */
};

struct Thread {
    /*
     * Interpreter state which must be preserved across nested
     * interpreter invocations (via JNI callbacks).  Must be the first
     * element in Thread.
     */
    InterpSaveState interpSave;

    /* small unique integer; useful for "thin" locks and debug messages */
    u4          threadId;

    /*
     * Begin interpreter state which does not need to be preserved, but should
     * be located towards the beginning of the Thread structure for
     * efficiency.
     */

    /*
     * interpBreak contains info about the interpreter mode, as well as
     * a count of the number of times the thread has been suspended.  When
     * the count drops to zero, the thread resumes.
     */
    InterpBreak interpBreak;

    /*
     * "dbgSuspendCount" is the portion of the suspend count that the
     * debugger is responsible for.  This has to be tracked separately so
     * that we can recover correctly if the debugger abruptly disconnects
     * (suspendCount -= dbgSuspendCount).  The debugger should not be able
     * to resume GC-suspended threads, because we ignore the debugger while
     * a GC is in progress.
     *
     * Both of these are guarded by gDvm.threadSuspendCountLock.
     *
     * Note the non-debug component will rarely be other than 1 or 0 -- (not
     * sure it's even possible with the way mutexes are currently used.)
     */

    int suspendCount;
    int dbgSuspendCount;

    u1*         cardTable;

    /* current limit of stack; flexes for StackOverflowError */
    const u1*   interpStackEnd;

    /* FP of bottom-most (currently executing) stack frame on interp stack */
    void*       XcurFrame;
    /* current exception, or NULL if nothing pending */
    Object*     exception;

    bool        debugIsMethodEntry;
    /* interpreter stack size; our stacks are fixed-length */
    int         interpStackSize;
    bool        stackOverflowed;

    /* thread handle, as reported by pthread_self() */
    pthread_t   handle;

    /* Assembly interpreter handler tables */
#ifndef DVM_NO_ASM_INTERP
    void*       mainHandlerTable;   // Table of actual instruction handler
    void*       altHandlerTable;    // Table of breakout handlers
#else
    void*       unused0;            // Consume space to keep offsets
    void*       unused1;            //   the same between builds with
#endif

    /*
     * singleStepCount is a countdown timer used with the breakFlag
     * kInterpSingleStep.  If kInterpSingleStep is set in breakFlags,
     * singleStepCount will decremented each instruction execution.
     * Once it reaches zero, the kInterpSingleStep flag in breakFlags
     * will be cleared.  This can be used to temporarily prevent
     * execution from re-entering JIT'd code or force inter-instruction
     * checks by delaying the reset of curHandlerTable to mainHandlerTable.
     */
    int         singleStepCount;

#ifdef WITH_JIT
    struct JitToInterpEntries jitToInterpEntries;
    /*
     * Whether the current top VM frame is in the interpreter or JIT cache:
     *   NULL    : in the interpreter
     *   non-NULL: entry address of the JIT'ed code (the actual value doesn't
     *             matter)
     */
    void*             inJitCodeCache;
    unsigned char*    pJitProfTable;
    int               jitThreshold;
    const void*       jitResumeNPC;     // Translation return point
    const u4*         jitResumeNSP;     // Native SP at return point
    const u2*         jitResumeDPC;     // Dalvik inst following single-step
    JitState    jitState;
    int         icRechainCount;
    const void* pProfileCountdown;
    const ClassObject* callsiteClass;
    const Method*     methodToCall;
#endif

    /* JNI local reference tracking */
    IndirectRefTable jniLocalRefTable;

#if defined(WITH_JIT)
#if defined(WITH_SELF_VERIFICATION)
    /* Buffer for register state during self verification */
    struct ShadowSpace* shadowSpace;
#endif
    int         currTraceRun;
    int         totalTraceLen;  // Number of Dalvik insts in trace
    const u2*   currTraceHead;  // Start of the trace we're building
    const u2*   currRunHead;    // Start of run we're building
    int         currRunLen;     // Length of run in 16-bit words
    const u2*   lastPC;         // Stage the PC for the threaded interpreter
    const Method*  traceMethod; // Starting method of current trace
    intptr_t    threshFilter[JIT_TRACE_THRESH_FILTER_SIZE];
    JitTraceRun trace[MAX_JIT_RUN_LEN];
#endif

    /*
     * Thread's current status.  Can only be changed by the thread itself
     * (i.e. don't mess with this from other threads).
     */
    volatile ThreadStatus status;

    /* thread ID, only useful under Linux */
    pid_t       systemTid;

    /* start (high addr) of interp stack (subtract size to get malloc addr) */
    u1*         interpStackStart;

    /* the java/lang/Thread that we are associated with */
    Object*     threadObj;

    /* the JNIEnv pointer associated with this thread */
    JNIEnv*     jniEnv;

    /* internal reference tracking */
    ReferenceTable  internalLocalRefTable;


    /* JNI native monitor reference tracking (initialized on first use) */
    ReferenceTable  jniMonitorRefTable;

    /* hack to make JNI_OnLoad work right */
    Object*     classLoaderOverride;

    /* mutex to guard the interrupted and the waitMonitor members */
    pthread_mutex_t    waitMutex;

    /* pointer to the monitor lock we're currently waiting on */
    /* guarded by waitMutex */
    /* TODO: consider changing this to Object* for better JDWP interaction */
    Monitor*    waitMonitor;

    /* thread "interrupted" status; stays raised until queried or thrown */
    /* guarded by waitMutex */
    bool        interrupted;

    /* links to the next thread in the wait set this thread is part of */
    struct Thread*     waitNext;

    /* object to sleep on while we are waiting for a monitor */
    pthread_cond_t     waitCond;

    /*
     * Set to true when the thread is in the process of throwing an
     * OutOfMemoryError.
     */
    bool        throwingOOME;

    /* links to rest of thread list; grab global lock before traversing */
    struct Thread* prev;
    struct Thread* next;

    /* used by threadExitCheck when a thread exits without detaching */
    int         threadExitCheckCount;

    /* JDWP invoke-during-breakpoint support */
    DebugInvokeReq  invokeReq;

    /* base time for per-thread CPU timing (used by method profiling) */
    bool        cpuClockBaseSet;
    u8          cpuClockBase;

    /* memory allocation profiling state */
    AllocProfState allocProf;

#ifdef WITH_JNI_STACK_CHECK
    u4          stackCrc;
#endif

#if WITH_EXTRA_GC_CHECKS > 1
    /* PC, saved on every instruction; redundant with StackSaveArea */
    const u2*   currentPc2;
#endif

    /* Safepoint callback state */
    pthread_mutex_t   callbackMutex;
    SafePointCallback callback;
    void*             callbackArg;

#if defined(ARCH_IA32) && defined(WITH_JIT)
    u4 spillRegion[MAX_SPILL_JIT_IA];
#endif
};

#endif // _DALVIK_OO_OBJECT_4_H__
