/*
 * art_object.h
 *
 *  Created on: 2015年3月30日
 *      Author: boyliang
 */

#ifndef ART_OBJECT_H_
#define ART_OBJECT_H_

#include <cutils/atomic.h>
#include <stdint.h>
#include <stddef.h>

#define LIKELY(x)       __builtin_expect((x), true)
#define UNLIKELY(x)     __builtin_expect((x), false)
#define MANAGED __attribute__((aligned(4)))
#define ALWAYS_INLINE  __attribute__ ((always_inline))

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

template<typename T> void UNUSED(const T&) {}
#define SHARED_LOCKS_REQUIRED(...) __attribute__ ((shared_locks_required(__VA_ARGS__)))
#define OFFSETOF_MEMBER(t, f) \
  (reinterpret_cast<const char*>(&reinterpret_cast<t*>(16)->f) - reinterpret_cast<const char*>(16)) // NOLINT

namespace art {

typedef uint8_t byte;

// Allow the meaning of offsets to be strongly typed
class Offset {
public:
	explicit Offset(size_t val) : val_(val) {}

	int32_t Int32Value() const {
		return static_cast<int32_t>(val_);
	}

	uint32_t Uint32Value() const {
		return static_cast<uint32_t>(val_);
	}
protected:
	size_t val_;
};

// Offsets relative to an object
class MemberOffset: public Offset {
public:
	explicit MemberOffset(size_t val) : Offset(val) {}
};

#define OFFSET_OF_OBJECT_MEMBER(type, field) MemberOffset(OFFSETOF_MEMBER(type, field))

class Primitive {
 public:
  enum Type {
    kPrimNot = 0,
    kPrimBoolean,
    kPrimByte,
    kPrimChar,
    kPrimShort,
    kPrimInt,
    kPrimLong,
    kPrimFloat,
    kPrimDouble,
    kPrimVoid,
  };
};

struct ArtMethodOffsets;
struct ConstructorMethodOffsets;
class ClassLinker;
struct MethodClassOffsets;
class DexFile;

namespace mirror {
  class ArtMethod;
  class Object;
  class DexCache;
}

union JValue {
	u1 z;
	s1 b;
	u2 c;
	s2 s;
	s4 i;
	s8 j;
	float f;
	double d;
	mirror::Object* l;
};

class MethodHelper{

public:
	const mirror::ArtMethod* GetMethod() const {
		return method_;
	}

private:
	ClassLinker* class_linker_;
	mirror::DexCache* dex_cache_;
	const DexFile* dex_file_;
	const mirror::ArtMethod* method_;
	const char* shorty_;
	uint32_t shorty_len_;
};

class StringPiece;

class ShadowFrame{

public:

	uint32_t NumberOfVRegs() const {
#if defined(ART_USE_PORTABLE_COMPILER)
		return number_of_vregs_ & ~kHasReferenceArray;
#else
		return number_of_vregs_;
#endif
	}

	int32_t GetVReg(size_t i) const {
		const uint32_t* vreg = &vregs_[i];
		return *reinterpret_cast<const int32_t*>(vreg);
	}

	float GetVRegFloat(size_t i) const {
//  	DCHECK_LT(i, NumberOfVRegs());
		// NOTE: Strict-aliasing?
		const uint32_t* vreg = &vregs_[i];
		return *reinterpret_cast<const float*>(vreg);
	}

	int64_t GetVRegLong(size_t i) const {
//  	DCHECK_LT(i, NumberOfVRegs());
		const uint32_t* vreg = &vregs_[i];
		return *reinterpret_cast<const int64_t*>(vreg);
	}

	double GetVRegDouble(size_t i) const {
//  	DCHECK_LT(i, NumberOfVRegs());
		const uint32_t* vreg = &vregs_[i];
		return *reinterpret_cast<const double*>(vreg);
	}

	mirror::Object* GetVRegReference(size_t i) const {
//  	DCHECK_LT(i, NumberOfVRegs());
		if (HasReferenceArray()) {
			return References()[i];
		} else {
			const uint32_t* vreg = &vregs_[i];
			return *reinterpret_cast<mirror::Object* const *>(vreg);
		}
	}

	// Get view of vregs as range of consecutive arguments starting at i.
	uint32_t* GetVRegArgs(size_t i) {
		return &vregs_[i];
	}

	void SetVReg(size_t i, int32_t val) {
//  	DCHECK_LT(i, NumberOfVRegs());
		uint32_t* vreg = &vregs_[i];
		*reinterpret_cast<int32_t*>(vreg) = val;
	}

	void SetVRegFloat(size_t i, float val) {
//  	DCHECK_LT(i, NumberOfVRegs());
		uint32_t* vreg = &vregs_[i];
		*reinterpret_cast<float*>(vreg) = val;
	}

	void SetVRegLong(size_t i, int64_t val) {
//  	DCHECK_LT(i, NumberOfVRegs());
		uint32_t* vreg = &vregs_[i];
		*reinterpret_cast<int64_t*>(vreg) = val;
	}

	void SetVRegDouble(size_t i, double val) {
//  	DCHECK_LT(i, NumberOfVRegs());
		uint32_t* vreg = &vregs_[i];
		*reinterpret_cast<double*>(vreg) = val;
	}

	void SetVRegReference(size_t i, mirror::Object* val) {
//  	DCHECK_LT(i, NumberOfVRegs());
		uint32_t* vreg = &vregs_[i];
		*reinterpret_cast<mirror::Object**>(vreg) = val;
		if (HasReferenceArray()) {
			References()[i] = val;
		}
	}

	mirror::ArtMethod* GetMethod() const {
		//	DCHECK_NE(method_, static_cast<void*>(NULL));
		return method_;
	}

    mirror::Object* GetThisObject() const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

    mirror::Object* GetThisObject(uint16_t num_ins) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

	bool HasReferenceArray() const {
#if defined(ART_USE_PORTABLE_COMPILER)
		return (number_of_vregs_ & kHasReferenceArray) != 0;
#else
		return true;
#endif
	}

	mirror::Object* const * References() const {
//		DCHECK(HasReferenceArray());
		const uint32_t* vreg_end = &vregs_[NumberOfVRegs()];
		return reinterpret_cast<mirror::Object* const *>(vreg_end);
	}

	mirror::Object** References() {
		return const_cast<mirror::Object**>(const_cast<const ShadowFrame*>(this)->References());
	}

private:
#if defined(ART_USE_PORTABLE_COMPILER)
  enum ShadowFrameFlag {
    kHasReferenceArray = 1ul << 31
  };
  // TODO: make const in the portable case.
  uint32_t number_of_vregs_;
#else
  const uint32_t number_of_vregs_;
#endif
  // Link to previous shadow frame or NULL.
  ShadowFrame* link_;
#if defined(ART_USE_PORTABLE_COMPILER)
  // TODO: make const in the portable case.
  mirror::ArtMethod* method_;
#else
  mirror::ArtMethod* const method_;
#endif
  uint32_t dex_pc_;
  uint32_t vregs_[0];
};


struct CodeItem {
	uint16_t registers_size_;
	uint16_t ins_size_;
	uint16_t outs_size_;
	uint16_t tries_size_;
	uint32_t debug_info_off_;  // file offset to debug info stream
	uint32_t insns_size_in_code_units_; // size of the insns array, in 2 byte code units
	uint16_t insns_[1];
};

struct ClassOffsets;
class Thread;

namespace mirror {

class Class;
class Throwable;
class Array;
class String;

class ArtMethod;
class ArtField;
class Object;
class DexCache;
class IfTable;

template<class T> class PrimitiveArray;
typedef PrimitiveArray<uint8_t> BooleanArray;
typedef PrimitiveArray<int8_t> ByteArray;
typedef PrimitiveArray<uint16_t> CharArray;
typedef PrimitiveArray<double> DoubleArray;
typedef PrimitiveArray<float> FloatArray;
typedef PrimitiveArray<int32_t> IntArray;
typedef PrimitiveArray<int64_t> LongArray;
typedef PrimitiveArray<int16_t> ShortArray;
template<class T> class ObjectArray;


class MANAGED Object {

public:

	uint32_t GetField32(MemberOffset field_offset, bool is_volatile) const {
//	    VerifyObject(this);
		const byte* raw_addr = reinterpret_cast<const byte*>(this) + field_offset.Int32Value();
		const int32_t* word_addr = reinterpret_cast<const int32_t*>(raw_addr);
		if (UNLIKELY(is_volatile)) {
			return android_atomic_acquire_load(word_addr);
		} else {
			return *word_addr;
		}
	}

	template<class T>
	T GetFieldObject(MemberOffset field_offset, bool is_volatile) const {
		T result = reinterpret_cast<T>(GetField32(field_offset, is_volatile));
//	    VerifyObject(result);
		return result;
	}

	Class* GetClass() const {
	  return GetFieldObject<Class*>(OFFSET_OF_OBJECT_MEMBER(Object, klass_), false);
	}

	void SetField32(MemberOffset field_offset, uint32_t new_value, bool is_volatile, bool this_is_valid = true) {
		if (this_is_valid) {
//			VerifyObject(this);
		}
		byte* raw_addr = reinterpret_cast<byte*>(this) + field_offset.Int32Value();
		uint32_t* word_addr = reinterpret_cast<uint32_t*>(raw_addr);
		if (UNLIKELY(is_volatile)) {
			/*
			 * TODO: add an android_atomic_synchronization_store() function and
			 * use it in the 32-bit volatile set handlers.  On some platforms we
			 * can use a fast atomic instruction and avoid the barriers.
			 */
//			ANDROID_MEMBAR_STORE();
			*word_addr = new_value;
//			ANDROID_MEMBAR_FULL();
		} else {
			*word_addr = new_value;
		}
	}

protected:
  // Accessors for non-Java type fields
  template<class T>
  T GetFieldPtr(MemberOffset field_offset, bool is_volatile) const {
    return reinterpret_cast<T>(GetField32(field_offset, is_volatile));
  }

  template<typename T>
  void SetFieldPtr(MemberOffset field_offset, T new_value, bool is_volatile, bool this_is_valid = true) {
	  SetField32(field_offset, reinterpret_cast<uint32_t>(new_value), is_volatile, this_is_valid);
  }

private:
	Class* klass_;
	uint32_t monitor_;
};

class StaticStorageBase;
class ClassLoader;

class MANAGED Array: public Object {
public:

	int32_t GetLength() const;

	void* GetRawData(size_t component_size);

	const void* GetRawData(size_t component_size) const;

	bool IsValidIndex(int32_t index) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);

private:
	// The number of array elements.
	int32_t length_;
	// Marker for the data (used by generated code)
	uint32_t first_element_[0];
};

template<class T>
class MANAGED PrimitiveArray : public Array {
public:
  const T* GetData() const;

  T* GetData();

  T Get(int32_t i) const SHARED_LOCKS_REQUIRED(Locks::mutator_lock_);
};

class MANAGED String: public Object {

public:
	static MemberOffset OffsetOffset() {
		return OFFSET_OF_OBJECT_MEMBER(String, offset_);
	}

	int32_t GetOffset() const {
		int32_t result = GetField32(OffsetOffset(), false);
		return result;
	}
	const CharArray* GetCharArray() const;
	int32_t GetLength() const;
	int32_t GetUtfLength() const;

private:
	// Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
	CharArray* array_;

	int32_t count_;

	uint32_t hash_code_;

	int32_t offset_;

	static Class* java_lang_String_;
};

typedef void (EntryPointFromInterpreter)(Thread* self, MethodHelper& mh,
		const CodeItem* code_item, ShadowFrame* shadow_frame,
		JValue* result);

static const uint32_t kAccPublic = 0x0001;  // class, field, method, ic
static const uint32_t kAccPrivate = 0x0002;  // field, method, ic
static const uint32_t kAccProtected = 0x0004;  // field, method, ic
static const uint32_t kAccStatic = 0x0008;  // field, method, ic
static const uint32_t kAccFinal = 0x0010;  // class, field, method, ic
static const uint32_t kAccSynchronized = 0x0020;  // method (only allowed on natives)
static const uint32_t kAccSuper = 0x0020;  // class (not used in dex)
static const uint32_t kAccVolatile = 0x0040;  // field
static const uint32_t kAccBridge = 0x0040;  // method (1.5)
static const uint32_t kAccTransient = 0x0080;  // field
static const uint32_t kAccVarargs = 0x0080;  // method (1.5)
static const uint32_t kAccNative = 0x0100;  // method
static const uint32_t kAccInterface = 0x0200;  // class, ic
static const uint32_t kAccAbstract = 0x0400;  // class, method, ic
static const uint32_t kAccStrict = 0x0800;  // method
static const uint32_t kAccSynthetic = 0x1000;  // field, method, ic
static const uint32_t kAccAnnotation = 0x2000;  // class, ic (1.5)
static const uint32_t kAccEnum = 0x4000;  // class, field, ic (1.5)

static const uint32_t kAccConstructor = 0x00010000;  // method (dex only) <init> and <clinit>

class MANAGED ArtMethod : public Object {

public:
	Class* GetDeclaringClass() const {
	  Class* result = GetFieldObject<Class*>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, declaring_class_), false);
//	  DCHECK(result != NULL) << this;
//	  DCHECK(result->IsIdxLoaded() || result->IsErroneous()) << this;
	  return result;
	}

	uint32_t GetAccessFlags() const {
		return GetField32(OFFSET_OF_OBJECT_MEMBER(ArtMethod, access_flags_), false);
	}

	// Returns true if the method is declared public.
	bool IsPublic() const{
		return (GetAccessFlags() & kAccPublic) != 0;
	}

	// Returns true if the method is declared private.
	bool IsPrivate() const{
		return (GetAccessFlags() & kAccPrivate) != 0;
	}

	// Returns true if the method is declared static.
	bool IsStatic() const{
		return (GetAccessFlags() & kAccStatic) != 0;
	}

	bool IsNative() const{
		return (GetAccessFlags() & kAccNative) != 0;
	}

	// Returns true if the method is a constructor.
	bool IsConstructor() const {
		return (GetAccessFlags() & kAccConstructor) != 0;
	}

	const void* GetEntryPointFromCompiledCode() const {
		return GetFieldPtr<const void*>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, entry_point_from_compiled_code_), false);
	}

	void SetEntryPointFromCompiledCode(const void* entry_point_from_compiled_code) {
	    SetFieldPtr<const void*>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, entry_point_from_compiled_code_), entry_point_from_compiled_code, false);
	}

	EntryPointFromInterpreter* GetEntryPointFromInterpreter() const {
	    return GetFieldPtr<EntryPointFromInterpreter*>(OFFSET_OF_OBJECT_MEMBER(ArtMethod, entry_point_from_interpreter_), false);
	}

	void SetAccessFlags(uint32_t new_access_flags) {
		SetField32(OFFSET_OF_OBJECT_MEMBER(ArtMethod, access_flags_), new_access_flags, false);
	}

	void SetNativeMethod(const void* native_method);

	static MemberOffset NativeMethodOffset() {
	    return OFFSET_OF_OBJECT_MEMBER(ArtMethod, native_method_);
	}

	const void* GetNativeMethod() const {
	    return reinterpret_cast<const void*>(GetField32(NativeMethodOffset(), false));
	}

	void RegisterNative(Thread* self, const void* native_method);

	void Invoke(Thread* self, uint32_t* args, uint32_t args_size, JValue* result, char result_type);

protected:
	// Field order required by test "ValidateFieldOrderOfJavaCppUnionClasses".
	// The class we are a part of
	Class* declaring_class_;

	// short cuts to declaring_class_->dex_cache_ member for fast compiled code access
	ObjectArray<StaticStorageBase>* dex_cache_initialized_static_storage_;

	// short cuts to declaring_class_->dex_cache_ member for fast compiled code access
	ObjectArray<ArtMethod>* dex_cache_resolved_methods_;

	// short cuts to declaring_class_->dex_cache_ member for fast compiled code access
	ObjectArray<Class>* dex_cache_resolved_types_;

	// short cuts to declaring_class_->dex_cache_ member for fast compiled code access
	ObjectArray<String>* dex_cache_strings_;

	// Access flags; low 16 bits are defined by spec.
	uint32_t access_flags_;

	// Offset to the CodeItem.
	uint32_t code_item_offset_;

	// Architecture-dependent register spill mask
	uint32_t core_spill_mask_;

	// Compiled code associated with this method for callers from managed code.
	// May be compiled managed code or a bridge for invoking a native method.
	// TODO: Break apart this into portable and quick.
	const void* entry_point_from_compiled_code_;

	// Called by the interpreter to execute this method.
	EntryPointFromInterpreter* entry_point_from_interpreter_;

	// Architecture-dependent register spill mask
	uint32_t fp_spill_mask_;

	// Total size in bytes of the frame
	size_t frame_size_in_bytes_;

	// Garbage collection map of native PC offsets (quick) or dex PCs (portable) to reference bitmaps.
	const uint8_t* gc_map_;

	// Mapping from native pc to dex pc
	const uint32_t* mapping_table_;

	// Index into method_ids of the dex file associated with this method
	uint32_t method_dex_index_;

	// For concrete virtual methods, this is the offset of the method in Class::vtable_.
	//
	// For abstract methods in an interface class, this is the offset of the method in
	// "iftable_->Get(n)->GetMethodArray()".
	//
	// For static and direct methods this is the index in the direct methods table.
	uint32_t method_index_;

	// The target native method registered with this method
	const void* native_method_;

	// When a register is promoted into a register, the spill mask holds which registers hold dex
	// registers. The first promoted register's corresponding dex register is vmap_table_[1], the Nth
	// is vmap_table_[N]. vmap_table_[0] holds the length of the table.
	const uint16_t* vmap_table_;

	static Class* java_lang_reflect_ArtMethod_;
};

class MANAGED StaticStorageBase : public Object {
};

// C++ mirror of java.lang.Class
class MANAGED Class: public StaticStorageBase {

public:
	enum Status {
	    kStatusError = -1,
	    kStatusNotReady = 0,
	    kStatusIdx = 1,  // Loaded, DEX idx in super_class_type_idx_ and interfaces_type_idx_.
	    kStatusLoaded = 2,  // DEX idx values resolved.
	    kStatusResolved = 3,  // Part of linking.
	    kStatusVerifying = 4,  // In the process of being verified.
	    kStatusRetryVerificationAtRuntime = 5,  // Compile time verification failed, retry at runtime.
	    kStatusVerifyingAtRuntime = 6,  // Retrying verification at runtime.
	    kStatusVerified = 7,  // Logically part of linking; done pre-init.
	    kStatusInitializing = 8,  // Class init in progress.
	    kStatusInitialized = 9,  // Ready to go.
	  };

	uint32_t GetAccessFlags() const {
	  return GetField32(OFFSET_OF_OBJECT_MEMBER(Class, access_flags_), false);
	}

	String* GetName() const {
		return GetFieldObject<String*>(OFFSET_OF_OBJECT_MEMBER(Class, name_), false);
	}

	Class* GetSuperClass() const SHARED_LOCKS_REQUIRED (Locks::mutator_lock_){
		 return GetFieldObject<Class*>(OFFSET_OF_OBJECT_MEMBER(Class, super_class_), false);
	}

	ClassLoader* GetClassLoader() const {
		return GetFieldObject<ClassLoader*>(OFFSET_OF_OBJECT_MEMBER(Class, class_loader_), false);
	}

	DexCache* GetDexCache() const SHARED_LOCKS_REQUIRED (Locks::mutator_lock_){
		return GetFieldObject<DexCache*>(OFFSET_OF_OBJECT_MEMBER(Class, dex_cache_), false);
	}

private:
	// defining class loader, or NULL for the "bootstrap" system loader
	ClassLoader* class_loader_;

	// For array classes, the component class object for instanceof/checkcast
	// (for String[][][], this will be String[][]). NULL for non-array classes.
	Class* component_type_;

	// DexCache of resolved constant pool entries (will be NULL for classes generated by the
	// runtime such as arrays and primitive classes).
	DexCache* dex_cache_;

	// static, private, and <init> methods
	ObjectArray<ArtMethod>* direct_methods_;

	// instance fields
	//
	// These describe the layout of the contents of an Object.
	// Note that only the fields directly declared by this class are
	// listed in ifields; fields declared by a superclass are listed in
	// the superclass's Class.ifields.
	//
	// All instance fields that refer to objects are guaranteed to be at
	// the beginning of the field list.  num_reference_instance_fields_
	// specifies the number of reference fields.
	ObjectArray<ArtField>* ifields_;

	// The interface table (iftable_) contains pairs of a interface class and an array of the
	// interface methods. There is one pair per interface supported by this class.  That means one
	// pair for each interface we support directly, indirectly via superclass, or indirectly via a
	// superinterface.  This will be null if neither we nor our superclass implement any interfaces.
	//
	// Why we need this: given "class Foo implements Face", declare "Face faceObj = new Foo()".
	// Invoke faceObj.blah(), where "blah" is part of the Face interface.  We can't easily use a
	// single vtable.
	//
	// For every interface a concrete class implements, we create an array of the concrete vtable_
	// methods for the methods in the interface.
	IfTable* iftable_;

	// descriptor for the class such as "java.lang.Class" or "[C". Lazily initialized by ComputeName
	String* name_;

	// Static fields
	ObjectArray<ArtField>* sfields_;

	// The superclass, or NULL if this is java.lang.Object, an interface or primitive type.
	Class* super_class_;

	// If class verify fails, we must return same error on subsequent tries.
	Class* verify_error_class_;

	// Virtual methods defined in this class; invoked through vtable.
	ObjectArray<ArtMethod>* virtual_methods_;

	// Virtual method table (vtable), for use by "invoke-virtual".  The vtable from the superclass is
	// copied in, and virtual methods from our class either replace those from the super or are
	// appended. For abstract classes, methods may be created in the vtable that aren't in
	// virtual_ methods_ for miranda methods.
	ObjectArray<ArtMethod>* vtable_;

	// Access flags; low 16 bits are defined by VM spec.
	uint32_t access_flags_;

	// Total size of the Class instance; used when allocating storage on gc heap.
	// See also object_size_.
	size_t class_size_;

	// Tid used to check for recursive <clinit> invocation.
	pid_t clinit_thread_id_;

	// ClassDef index in dex file, -1 if no class definition such as an array.
	// TODO: really 16bits
	int32_t dex_class_def_idx_;

	// Type index in dex file.
	// TODO: really 16bits
	int32_t dex_type_idx_;

	// Number of instance fields that are object refs.
	size_t num_reference_instance_fields_;

	// Number of static fields that are object refs,
	size_t num_reference_static_fields_;

	// Total object size; used when allocating storage on gc heap.
	// (For interfaces and abstract classes this will be zero.)
	// See also class_size_.
	size_t object_size_;

	// Primitive type value, or Primitive::kPrimNot (0); set for generated primitive classes.
	Primitive::Type primitive_type_;

	// Bitmap of offsets of ifields.
	uint32_t reference_instance_offsets_;

	// Bitmap of offsets of sfields.
	uint32_t reference_static_offsets_;

	// State of class initialization.
	Status status_;

	// TODO: ?
	// initiating class loader list
	// NOTE: for classes with low serialNumber, these are unused, and the
	// values are kept in a table in gDvm.
	// InitiatingLoaderList initiating_loader_list_;

	// Location of first static field.
	uint32_t fields_[0];

	// java.lang.Class
	static Class* java_lang_Class_;
};
}



// namespace mirror
}// namespace art

#endif /* ART_OBJECT_H_ */
