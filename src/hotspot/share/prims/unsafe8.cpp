// This project is a modified version of OpenJDK, licensed under GPL v2.
// Modifications Copyright (C) 2025 ByteDance Inc.
/*
 * Copyright (c) 1997, 2021, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "jni.h"
#include "jvm.h"
#include "classfile/classFileStream.hpp"
#include "classfile/classLoader.hpp"
#include "classfile/classFileParser.hpp"
#include "classfile/classLoadInfo.hpp"
#include "classfile/javaClasses.inline.hpp"
#include "classfile/systemDictionary.hpp"
#include "classfile/vmSymbols.hpp"
#include "jfr/jfrEvents.hpp"
#include "memory/allocation.inline.hpp"
#include "memory/resourceArea.hpp"
#include "oops/access.inline.hpp"
#include "oops/fieldStreams.inline.hpp"
#include "oops/instanceKlass.inline.hpp"
#include "oops/klass.inline.hpp"
#include "oops/objArrayOop.inline.hpp"
#include "oops/oop.inline.hpp"
#include "oops/typeArrayOop.inline.hpp"
#include "prims/unsafe.hpp"
#include "runtime/globals.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/jniHandles.inline.hpp"
#include "runtime/orderAccess.hpp"
#include "runtime/reflection.hpp"
#include "runtime/sharedRuntime.hpp"
#include "runtime/stubRoutines.hpp"
#include "runtime/thread.hpp"
#include "runtime/threadSMR.hpp"
#include "runtime/vmOperations.hpp"
#include "runtime/vm_version.hpp"
#include "services/threadService.hpp"
#include "utilities/align.hpp"
#include "utilities/copy.hpp"
#include "utilities/dtrace.hpp"
#include "utilities/macros.hpp"
#include "prims/unsafe.inline.hpp"


/// JVM_RegisterUnsafeMethods
/*
 *      Implementation of class sun.misc.Unsafe
 */

#ifndef USDT2
//HS_DTRACE_PROBE_DECL3(hotspot, thread__park__begin, uintptr_t, int, long long);
//HS_DTRACE_PROBE_DECL1(hotspot, thread__park__end, uintptr_t);
//HS_DTRACE_PROBE_DECL1(hotspot, thread__unpark, uintptr_t);
#endif /* !USDT2 */

#define UnsafeWrapper(arg) /*nothing, for the present*/


inline jint invocation_key_from_method_slot(jint slot) {
  return slot;
}

inline jint invocation_key_to_method_slot(jint key) {
  return key;
}

jint Unsafe_invocation_key_from_method_slot(jint slot) {
  return invocation_key_from_method_slot(slot);
}
jint Unsafe_invocation_key_to_method_slot(jint key) {
  return invocation_key_to_method_slot(key);
}

// Get/SetObject must be special-cased, since it works with handles.

extern "C" jobject Unsafe_GetReference(JNIEnv *env, jobject unsafe, jobject obj, jlong offset);
extern "C" void Unsafe_PutReference(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jobject x_h);
extern "C" jobject Unsafe_GetReferenceVolatile(JNIEnv *env, jobject unsafe, jobject obj, jlong offset);
extern "C" void Unsafe_PutReferenceVolatile(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jobject x_h);

#define Unsafe_GetObject Unsafe_GetReference
#define Unsafe_SetObject Unsafe_PutReference
#define Unsafe_GetObjectVolatile Unsafe_GetReferenceVolatile
#define Unsafe_SetObjectVolatile Unsafe_PutReferenceVolatile
#define Unsafe_GetObject140 Unsafe_GetReference
#define Unsafe_SetObject140 Unsafe_PutReference

#define DEFINE_GETSETOOP(jboolean, Boolean) \
 \
extern "C" jboolean Unsafe_Get##Boolean(JNIEnv *env, jobject unsafe, jobject obj, jlong offset); \
 \
extern "C" void Unsafe_Put##Boolean(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jboolean x); \
 \
// END DEFINE_GETSETOOP.

DEFINE_GETSETOOP(jboolean, Boolean)
DEFINE_GETSETOOP(jbyte, Byte)
DEFINE_GETSETOOP(jshort, Short);
DEFINE_GETSETOOP(jchar, Char);
DEFINE_GETSETOOP(jint, Int);
DEFINE_GETSETOOP(jlong, Long);
DEFINE_GETSETOOP(jfloat, Float);
DEFINE_GETSETOOP(jdouble, Double);

#define Unsafe_SetBoolean Unsafe_PutBoolean
#define Unsafe_SetByte    Unsafe_PutByte
#define Unsafe_SetShort   Unsafe_PutShort
#define Unsafe_SetChar    Unsafe_PutChar
#define Unsafe_SetInt     Unsafe_PutInt
#define Unsafe_SetLong    Unsafe_PutLong
#define Unsafe_SetFloat   Unsafe_PutFloat
#define Unsafe_SetDouble  Unsafe_PutDouble

#define Unsafe_GetBoolean140  Unsafe_GetBoolean
#define Unsafe_GetByte140     Unsafe_GetByte
#define Unsafe_GetShort140    Unsafe_GetShort
#define Unsafe_GetChar140     Unsafe_GetChar
#define Unsafe_GetInt140      Unsafe_GetInt
#define Unsafe_GetLong140     Unsafe_GetLong
#define Unsafe_GetFloat140    Unsafe_GetFloat
#define Unsafe_GetDouble140   Unsafe_GetDouble
#define Unsafe_SetBoolean140  Unsafe_PutBoolean
#define Unsafe_SetByte140     Unsafe_PutByte
#define Unsafe_SetShort140    Unsafe_PutShort
#define Unsafe_SetChar140     Unsafe_PutChar
#define Unsafe_SetInt140      Unsafe_PutInt
#define Unsafe_SetLong140     Unsafe_PutLong
#define Unsafe_SetFloat140    Unsafe_PutFloat
#define Unsafe_SetDouble140   Unsafe_PutDouble

#undef DEFINE_GETSETOOP

#define DEFINE_GETSETOOP_VOLATILE(jboolean, Boolean) \
 \
extern "C" jboolean Unsafe_Get##Boolean##Volatile(JNIEnv *env, jobject unsafe, jobject obj, jlong offset); \
 \
extern "C" void Unsafe_Put##Boolean##Volatile(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jboolean x); \
 \
// END DEFINE_GETSETOOP_VOLATILE.

DEFINE_GETSETOOP_VOLATILE(jboolean, Boolean)
DEFINE_GETSETOOP_VOLATILE(jbyte, Byte)
DEFINE_GETSETOOP_VOLATILE(jshort, Short);
DEFINE_GETSETOOP_VOLATILE(jchar, Char);
DEFINE_GETSETOOP_VOLATILE(jint, Int);
DEFINE_GETSETOOP_VOLATILE(jfloat, Float);
DEFINE_GETSETOOP_VOLATILE(jdouble, Double);

#ifdef SUPPORTS_NATIVE_CX8
DEFINE_GETSETOOP_VOLATILE(jlong, Long);
#endif

#undef DEFINE_GETSETOOP_VOLATILE

// The non-intrinsified versions of setOrdered just use setVolatile

UNSAFE_ENTRY(void, Unsafe_SetOrderedInt(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jint x))
  UnsafeWrapper("Unsafe_SetOrderedInt");
  MemoryAccess<jint>(thread, obj, offset).put_volatile(x);
UNSAFE_END

#define Unsafe_SetOrderedObject   Unsafe_PutReferenceVolatile
#define Unsafe_SetOrderedLong     Unsafe_PutLongVolatile

UNSAFE_ENTRY(void, Unsafe_LoadFence(JNIEnv *env, jobject unsafe))
  UnsafeWrapper("Unsafe_LoadFence");
  OrderAccess::acquire();
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_StoreFence(JNIEnv *env, jobject unsafe))
  UnsafeWrapper("Unsafe_StoreFence");
  OrderAccess::release();
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_FullFence(JNIEnv *env, jobject unsafe))
  UnsafeWrapper("Unsafe_FullFence");
  OrderAccess::fence();
UNSAFE_END

////// Data in the C heap.

// Note:  These do not throw NullPointerException for bad pointers.
// They just crash.  Only a oop base pointer can generate a NullPointerException.
//
#define DEFINE_GETSETNATIVE(java_type, Type, native_type) \
 \
UNSAFE_ENTRY(java_type, Unsafe_GetNative##Type(JNIEnv *env, jobject unsafe, jlong addr)) \
  UnsafeWrapper("Unsafe_GetNative"#Type); \
  void* p = addr_from_java(addr); \
  GuardUnsafeAccess gua(JavaThread::current()); \
  java_type x = *(volatile native_type*)p; \
  return x; \
UNSAFE_END \
 \
UNSAFE_ENTRY(void, Unsafe_SetNative##Type(JNIEnv *env, jobject unsafe, jlong addr, java_type x)) \
  UnsafeWrapper("Unsafe_SetNative"#Type); \
  GuardUnsafeAccess gua(JavaThread::current()); \
  void* p = addr_from_java(addr); \
  *(volatile native_type*)p = x; \
UNSAFE_END \
 \
// END DEFINE_GETSETNATIVE.

DEFINE_GETSETNATIVE(jbyte, Byte, signed char)
DEFINE_GETSETNATIVE(jshort, Short, signed short);
DEFINE_GETSETNATIVE(jchar, Char, unsigned short);
DEFINE_GETSETNATIVE(jint, Int, jint);
// no long -- handled specially
DEFINE_GETSETNATIVE(jfloat, Float, float);
DEFINE_GETSETNATIVE(jdouble, Double, double);

#undef DEFINE_GETSETNATIVE

UNSAFE_ENTRY(jlong, Unsafe_GetNativeLong(JNIEnv *env, jobject unsafe, jlong addr))
  UnsafeWrapper("Unsafe_GetNativeLong");
  JavaThread* t = JavaThread::current();
  // We do it this way to avoid problems with access to heap using 64
  // bit loads, as jlong in heap could be not 64-bit aligned, and on
  // some CPUs (SPARC) it leads to SIGBUS.
  GuardUnsafeAccess gua(t);
  void* p = addr_from_java(addr);
  jlong x;
  if (((intptr_t)p & 7) == 0) {
    // jlong is aligned, do a volatile access
    x = *(volatile jlong*)p;
  } else {
    jlong_accessor acc;
    acc.words[0] = ((volatile jint*)p)[0];
    acc.words[1] = ((volatile jint*)p)[1];
    x = acc.long_value;
  }
  return x;
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_SetNativeLong(JNIEnv *env, jobject unsafe, jlong addr, jlong x))
  UnsafeWrapper("Unsafe_SetNativeLong");
  JavaThread* t = JavaThread::current();
  // see comment for Unsafe_GetNativeLong
  GuardUnsafeAccess gua(t);
  void* p = addr_from_java(addr);
  if (((intptr_t)p & 7) == 0) {
    // jlong is aligned, do a volatile access
    *(volatile jlong*)p = x;
  } else {
    jlong_accessor acc;
    acc.long_value = x;
    ((volatile jint*)p)[0] = acc.words[0];
    ((volatile jint*)p)[1] = acc.words[1];
  }
UNSAFE_END


UNSAFE_ENTRY(jlong, Unsafe_GetNativeAddress(JNIEnv *env, jobject unsafe, jlong addr))
  UnsafeWrapper("Unsafe_GetNativeAddress");
  void* p = addr_from_java(addr);
  return addr_to_java(*(void**)p);
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_SetNativeAddress(JNIEnv *env, jobject unsafe, jlong addr, jlong x))
  UnsafeWrapper("Unsafe_SetNativeAddress");
  void* p = addr_from_java(addr);
  *(void**)p = addr_from_java(x);
UNSAFE_END


////// Allocation requests

extern "C" jobject Unsafe_AllocateInstance(JNIEnv *env, jobject unsafe, jclass cls);

extern "C" jlong Unsafe_AllocateMemory0(JNIEnv *env, jobject unsafe, jlong size);

extern "C" jlong Unsafe_ReallocateMemory0(JNIEnv *env, jobject unsafe, jlong addr, jlong size);

extern "C" void Unsafe_FreeMemory0(JNIEnv *env, jobject unsafe, jlong addr);

UNSAFE_ENTRY(void, Unsafe_SetMemory(JNIEnv *env, jobject unsafe, jlong addr, jlong size, jbyte value))
  UnsafeWrapper("Unsafe_SetMemory");
  size_t sz = (size_t)size;
  if (sz != (julong)size || size < 0) {
    THROW(vmSymbols::java_lang_IllegalArgumentException());
  }
  char* p = (char*) addr_from_java(addr);
  Copy::fill_to_memory_atomic(p, sz, value);
UNSAFE_END

extern "C" void Unsafe_SetMemory0(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jlong size, jbyte value);

UNSAFE_ENTRY(void, Unsafe_CopyMemory(JNIEnv *env, jobject unsafe, jlong srcAddr, jlong dstAddr, jlong size))
  UnsafeWrapper("Unsafe_CopyMemory");
  if (size == 0) {
    return;
  }
  size_t sz = (size_t)size;
  if (sz != (julong)size || size < 0) {
    THROW(vmSymbols::java_lang_IllegalArgumentException());
  }
  void* src = addr_from_java(srcAddr);
  void* dst = addr_from_java(dstAddr);
  Copy::conjoint_memory_atomic(src, dst, sz);
UNSAFE_END

extern "C" void Unsafe_CopyMemory0(JNIEnv *env, jobject unsafe, jobject srcObj, jlong srcOffset, jobject dstObj, jlong dstOffset, jlong size);

////// Random queries

// See comment at file start about UNSAFE_LEAF
//UNSAFE_LEAF(jint, Unsafe_AddressSize())
UNSAFE_ENTRY(jint, Unsafe_AddressSize(JNIEnv *env, jobject unsafe))
  UnsafeWrapper("Unsafe_AddressSize");
  return sizeof(void*);
UNSAFE_END

// See comment at file start about UNSAFE_LEAF
//UNSAFE_LEAF(jint, Unsafe_PageSize())
UNSAFE_ENTRY(jint, Unsafe_PageSize(JNIEnv *env, jobject unsafe))
  UnsafeWrapper("Unsafe_PageSize");
  return os::vm_page_size();
UNSAFE_END

static jint find_field_offset(jobject field, int must_be_static, TRAPS) {
  if (field == NULL) {
    THROW_0(vmSymbols::java_lang_NullPointerException());
  }

  oop reflected   = JNIHandles::resolve_non_null(field);
  oop mirror      = java_lang_reflect_Field::clazz(reflected);
  Klass* k      = java_lang_Class::as_Klass(mirror);
  int slot        = java_lang_reflect_Field::slot(reflected);
  int modifiers   = java_lang_reflect_Field::modifiers(reflected);

  if (must_be_static >= 0) {
    int really_is_static = ((modifiers & JVM_ACC_STATIC) != 0);
    if (must_be_static != really_is_static) {
      THROW_0(vmSymbols::java_lang_IllegalArgumentException());
    }
  }

  int offset = InstanceKlass::cast(k)->field_offset(slot);
  return field_offset_from_byte_offset(offset);
}

UNSAFE_ENTRY(jlong, Unsafe_ObjectFieldOffset(JNIEnv *env, jobject unsafe, jobject field))
  UnsafeWrapper("Unsafe_ObjectFieldOffset");
  return find_field_offset(field, 0, THREAD);
UNSAFE_END

UNSAFE_ENTRY(jlong, Unsafe_StaticFieldOffset(JNIEnv *env, jobject unsafe, jobject field))
  UnsafeWrapper("Unsafe_StaticFieldOffset");
  return find_field_offset(field, 1, THREAD);
UNSAFE_END

UNSAFE_ENTRY(jobject, Unsafe_StaticFieldBaseFromField(JNIEnv *env, jobject unsafe, jobject field))
  UnsafeWrapper("Unsafe_StaticFieldBase");
  // Note:  In this VM implementation, a field address is always a short
  // offset from the base of a a klass metaobject.  Thus, the full dynamic
  // range of the return type is never used.  However, some implementations
  // might put the static field inside an array shared by many classes,
  // or even at a fixed address, in which case the address could be quite
  // large.  In that last case, this function would return NULL, since
  // the address would operate alone, without any base pointer.

  if (field == NULL)  THROW_0(vmSymbols::java_lang_NullPointerException());

  oop reflected   = JNIHandles::resolve_non_null(field);
  oop mirror      = java_lang_reflect_Field::clazz(reflected);
  int modifiers   = java_lang_reflect_Field::modifiers(reflected);

  if ((modifiers & JVM_ACC_STATIC) == 0) {
    THROW_0(vmSymbols::java_lang_IllegalArgumentException());
  }

  return JNIHandles::make_local(THREAD, mirror);
UNSAFE_END

//@deprecated
UNSAFE_ENTRY(jint, Unsafe_FieldOffset(JNIEnv *env, jobject unsafe, jobject field))
  UnsafeWrapper("Unsafe_FieldOffset");
  // tries (but fails) to be polymorphic between static and non-static:
  jlong offset = find_field_offset(field, -1, THREAD);
  guarantee(offset == (jint)offset, "offset fits in 32 bits");
  return (jint)offset;
UNSAFE_END

//@deprecated
UNSAFE_ENTRY(jobject, Unsafe_StaticFieldBaseFromClass(JNIEnv *env, jobject unsafe, jobject clazz))
  UnsafeWrapper("Unsafe_StaticFieldBase");
  if (clazz == NULL) {
    THROW_0(vmSymbols::java_lang_NullPointerException());
  }
  return JNIHandles::make_local(THREAD, JNIHandles::resolve_non_null(clazz));
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_EnsureClassInitialized(JNIEnv *env, jobject unsafe, jobject clazz)) {
  UnsafeWrapper("Unsafe_EnsureClassInitialized");
  if (clazz == NULL) {
    THROW(vmSymbols::java_lang_NullPointerException());
  }
  oop mirror = JNIHandles::resolve_non_null(clazz);

  Klass* klass = java_lang_Class::as_Klass(mirror);
  if (klass != NULL && klass->should_be_initialized()) {
    InstanceKlass* k = InstanceKlass::cast(klass);
    k->initialize(CHECK);
  }
}
UNSAFE_END

UNSAFE_ENTRY(jboolean, Unsafe_ShouldBeInitialized(JNIEnv *env, jobject unsafe, jobject clazz)) {
  UnsafeWrapper("Unsafe_ShouldBeInitialized");
  if (clazz == NULL) {
    THROW_(vmSymbols::java_lang_NullPointerException(), false);
  }
  oop mirror = JNIHandles::resolve_non_null(clazz);
  Klass* klass = java_lang_Class::as_Klass(mirror);
  if (klass != NULL && klass->should_be_initialized()) {
    return true;
  }
  return false;
}
UNSAFE_END

static void getBaseAndScale(int& base, int& scale, jclass acls, TRAPS) {
  if (acls == NULL) {
    THROW(vmSymbols::java_lang_NullPointerException());
  }
  oop      mirror = JNIHandles::resolve_non_null(acls);
  Klass* k      = java_lang_Class::as_Klass(mirror);
  if (k == NULL || !k->is_array_klass()) {
    THROW(vmSymbols::java_lang_InvalidClassException());
  } else if (k->is_objArray_klass()) {
    base  = arrayOopDesc::base_offset_in_bytes(T_OBJECT);
    scale = heapOopSize;
  } else if (k->is_typeArray_klass()) {
    TypeArrayKlass* tak = TypeArrayKlass::cast(k);
    base  = tak->array_header_in_bytes();
    assert(base == arrayOopDesc::base_offset_in_bytes(tak->element_type()), "array_header_size semantics ok");
    scale = (1 << tak->log2_element_size());
  } else {
    ShouldNotReachHere();
  }
}

UNSAFE_ENTRY(jint, Unsafe_ArrayBaseOffset(JNIEnv *env, jobject unsafe, jclass acls))
  UnsafeWrapper("Unsafe_ArrayBaseOffset");
  int base = 0, scale = 0;
  getBaseAndScale(base, scale, acls, CHECK_0);
  return field_offset_from_byte_offset(base);
UNSAFE_END


UNSAFE_ENTRY(jint, Unsafe_ArrayIndexScale(JNIEnv *env, jobject unsafe, jclass acls))
  UnsafeWrapper("Unsafe_ArrayIndexScale");
  int base = 0, scale = 0;
  getBaseAndScale(base, scale, acls, CHECK_0);
  // This VM packs both fields and array elements down to the byte.
  // But watch out:  If this changes, so that array references for
  // a given primitive type (say, T_BOOLEAN) use different memory units
  // than fields, this method MUST return zero for such arrays.
  // For example, the VM used to store sub-word sized fields in full
  // words in the object layout, so that accessors like getByte(Object,int)
  // did not really do what one might expect for arrays.  Therefore,
  // this function used to report a zero scale factor, so that the user
  // would know not to attempt to access sub-word array elements.
  // // Code for unpacked fields:
  // if (scale < wordSize)  return 0;

  // The following allows for a pretty general fieldOffset cookie scheme,
  // but requires it to be linear in byte offset.
  return field_offset_from_byte_offset(scale) - field_offset_from_byte_offset(0);
UNSAFE_END


static inline void throw_new(JNIEnv *env, const char *ename) {
  char buf[100];
  strcpy(buf, "java/lang/");
  strcat(buf, ename);
  jclass cls = env->FindClass(buf);
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    tty->print_cr("Unsafe: cannot throw %s because FindClass has failed", buf);
    return;
  }
  char* msg = NULL;
  env->ThrowNew(cls, msg);
}

extern "C" jclass Unsafe_DefineClass0(JNIEnv *env, jobject unsafe, jstring name, jbyteArray data, int offset, int length, jobject loader, jobject pd);

// define a class but do not make it known to the class loader or system dictionary
// - host_class:  supplies context for linkage, access control, protection domain, and class loader
// - data:  bytes of a class file, a raw memory address (length gives the number of bytes)
// - cp_patches:  where non-null entries exist, they replace corresponding CP entries in data

// When you load an anonymous class U, it works as if you changed its name just before loading,
// to a name that you will never use again.  Since the name is lost, no other class can directly
// link to any member of U.  Just after U is loaded, the only way to use it is reflectively,
// through java.lang.Class methods like Class.newInstance.

// Access checks for linkage sites within U continue to follow the same rules as for named classes.
// The package of an anonymous class is given by the package qualifier on the name under which it was loaded.
// An anonymous class also has special privileges to access any member of its host class.
// This is the main reason why this loading operation is unsafe.  The purpose of this is to
// allow language implementations to simulate "open classes"; a host class in effect gets
// new code when an anonymous class is loaded alongside it.  A less convenient but more
// standard way to do this is with reflection, which can also be set to ignore access
// restrictions.

// Access into an anonymous class is possible only through reflection.  Therefore, there
// are no special access rules for calling into an anonymous class.  The relaxed access
// rule for the host class is applied in the opposite direction:  A host class reflectively
// access one of its anonymous classes.

// If you load the same bytecodes twice, you get two different classes.  You can reload
// the same bytecodes with or without varying CP patches.

// By using the CP patching array, you can have a new anonymous class U2 refer to an older one U1.
// The bytecodes for U2 should refer to U1 by a symbolic name (doesn't matter what the name is).
// The CONSTANT_Class entry for that name can be patched to refer directly to U1.

// This allows, for example, U2 to use U1 as a superclass or super-interface, or as
// an outer class (so that U2 is an anonymous inner class of anonymous U1).
// It is not possible for a named class, or an older anonymous class, to refer by
// name (via its CP) to a newer anonymous class.

// CP patching may also be used to modify (i.e., hack) the names of methods, classes,
// or type descriptors used in the loaded anonymous class.

// Finally, CP patching may be used to introduce "live" objects into the constant pool,
// instead of "dead" strings.  A compiled statement like println((Object)"hello") can
// be changed to println(greeting), where greeting is an arbitrary object created before
// the anonymous class is loaded.  This is useful in dynamic languages, in which
// various kinds of metaobjects must be introduced as constants into bytecode.
// Note the cast (Object), which tells the verifier to expect an arbitrary object,
// not just a literal string.  For such ldc instructions, the verifier uses the
// type Object instead of String, if the loaded constant is not in fact a String.

static Klass*
Unsafe_DefineAnonymousClass_impl(JNIEnv *env,
                                 jclass host_class, jbyteArray data, jobjectArray cp_patches_jh,
                                 HeapWord* *temp_alloc,
                                 TRAPS) {

  if (UsePerfData) {
    ClassLoader::unsafe_defineClassCallCounter()->inc();
  }

  if (data == NULL) {
    THROW_0(vmSymbols::java_lang_NullPointerException());
  }

  jint length = typeArrayOop(JNIHandles::resolve_non_null(data))->length();
  jint word_length = (length + sizeof(HeapWord)-1) / sizeof(HeapWord);
  HeapWord* body = NEW_C_HEAP_ARRAY(HeapWord, word_length, mtInternal);
  if (body == NULL) {
    THROW_0(vmSymbols::java_lang_OutOfMemoryError());
  }

  // caller responsible to free it:
  (*temp_alloc) = body;

  {
    jbyte* array_base = typeArrayOop(JNIHandles::resolve_non_null(data))->byte_at_addr(0);
    Copy::conjoint_words((HeapWord*) array_base, body, word_length);
  }

  u1* class_bytes = (u1*) body;
  int class_bytes_length = (int) length;
  if (class_bytes_length < 0)  class_bytes_length = 0;
  if (class_bytes == NULL
      || host_class == NULL
      || length != class_bytes_length)
    THROW_0(vmSymbols::java_lang_IllegalArgumentException());

  objArrayHandle cp_patches_h;
  if (cp_patches_jh != NULL) {
    oop p = JNIHandles::resolve_non_null(cp_patches_jh);
    if (!p->is_objArray())
      THROW_0(vmSymbols::java_lang_IllegalArgumentException());
    cp_patches_h = objArrayHandle(THREAD, (objArrayOop)p);
  }

  Klass* host_klass(java_lang_Class::as_Klass(JNIHandles::resolve_non_null(host_class)));
  const char* host_source = host_klass->external_name();
  Handle      host_loader(THREAD, host_klass->class_loader());
  Handle      host_domain(THREAD, host_klass->protection_domain());

  GrowableArray<Handle>* cp_patches = NULL;
  if (cp_patches_h.not_null()) {
    int alen = cp_patches_h->length();
    for (int i = alen-1; i >= 0; i--) {
      oop p = cp_patches_h->obj_at(i);
      if (p != NULL) {
        Handle patch(THREAD, p);
        if (cp_patches == NULL)
          cp_patches = new GrowableArray<Handle>(i+1, i+1, Handle());
        cp_patches->at_put(i, patch);
      }
    }
  }

  ClassFileStream st(class_bytes, class_bytes_length, (char*) host_source);

  // Below block has been re-writen to fit into jdk17's shoes
  // TODO: test it!
  Klass* anonk = SystemDictionary::parse_stream(NULL,
                                                host_loader,
                                                host_domain,
                                                &st,
                                                host_klass,
                                                cp_patches,
                                                CHECK_NULL);
  return anonk;
}

UNSAFE_ENTRY(jclass, Unsafe_DefineAnonymousClass(JNIEnv *env, jobject unsafe, jclass host_class, jbyteArray data, jobjectArray cp_patches_jh))
{
  Klass* anon_klass;
  jobject res_jh = NULL;

  UnsafeWrapper("Unsafe_DefineAnonymousClass");
  ResourceMark rm(THREAD);

  HeapWord* temp_alloc = NULL;

  anon_klass = Unsafe_DefineAnonymousClass_impl(env, host_class, data,
                                                cp_patches_jh,
                                                   &temp_alloc, THREAD);
  anon_klass->set_is_hidden();
  if (anon_klass != NULL)
    res_jh = JNIHandles::make_local(THREAD, anon_klass->java_mirror());

  // try/finally clause:
  if (temp_alloc != NULL) {
    FREE_C_HEAP_ARRAY(HeapWord, temp_alloc);
  }

  // The anonymous class loader data has been artificially been kept alive to
  // this point.   The mirror and any instances of this class have to keep
  // it alive afterwards.
  if (anon_klass != NULL) {
    anon_klass->class_loader_data()->dec_keep_alive();
  }

  // let caller initialize it as needed...

  return (jclass) res_jh;
}
UNSAFE_END



UNSAFE_ENTRY(void, Unsafe_MonitorEnter(JNIEnv *env, jobject unsafe, jobject jobj))
  UnsafeWrapper("Unsafe_MonitorEnter");
  {
    if (jobj == NULL) {
      THROW(vmSymbols::java_lang_NullPointerException());
    }
    Handle obj(thread, JNIHandles::resolve_non_null(jobj));
    ObjectSynchronizer::jni_enter(obj, CHECK);
  }
UNSAFE_END


UNSAFE_ENTRY(jboolean, Unsafe_TryMonitorEnter(JNIEnv *env, jobject unsafe, jobject jobj))
  UnsafeWrapper("Unsafe_TryMonitorEnter");
  {
    if (jobj == NULL) {
      THROW_(vmSymbols::java_lang_NullPointerException(), JNI_FALSE);
    }
    Handle obj(thread, JNIHandles::resolve_non_null(jobj));
    bool res = ObjectSynchronizer::jni_try_enter(obj, CHECK_0);
    return (res ? JNI_TRUE : JNI_FALSE);
  }
UNSAFE_END


UNSAFE_ENTRY(void, Unsafe_MonitorExit(JNIEnv *env, jobject unsafe, jobject jobj))
  UnsafeWrapper("Unsafe_MonitorExit");
  {
    if (jobj == NULL) {
      THROW(vmSymbols::java_lang_NullPointerException());
    }
    Handle obj(THREAD, JNIHandles::resolve_non_null(jobj));
    ObjectSynchronizer::jni_exit(obj(), CHECK);
  }
UNSAFE_END


extern "C" void Unsafe_ThrowException(JNIEnv *env, jobject unsafe, jthrowable thr);

// JSR166 ------------------------------------------------------------------

extern "C" jboolean Unsafe_CompareAndSetReference(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jobject e_h, jobject x_h);

extern "C" jboolean Unsafe_CompareAndSetInt(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jint e, jint x);

extern "C" jboolean Unsafe_CompareAndSetLong(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jlong e, jlong x);

#define Unsafe_CompareAndSwapObject Unsafe_CompareAndSetReference
#define Unsafe_CompareAndSwapInt Unsafe_CompareAndSetInt
#define Unsafe_CompareAndSwapLong Unsafe_CompareAndSetLong

// Reuse those methods declared in prims/unsafe.cpp
extern "C" void Unsafe_Park(JNIEnv *env, jobject unsafe, jboolean isAbsolute, jlong time);
extern "C" void Unsafe_Unpark(JNIEnv *env, jobject unsafe, jobject jthread);

extern "C" jint Unsafe_GetLoadAverage0(JNIEnv *env, jobject unsafe, jdoubleArray loadavg, jint nelem);

UNSAFE_ENTRY(void, Unsafe_PrefetchRead(JNIEnv* env, jclass ignored, jobject obj, jlong offset))
  UnsafeWrapper("Unsafe_PrefetchRead");
  oop p = JNIHandles::resolve(obj);
  void* addr = index_oop_from_field_offset_long(p, 0);
  Prefetch::read(addr, (intx)offset);
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_PrefetchWrite(JNIEnv* env, jclass ignored, jobject obj, jlong offset))
  UnsafeWrapper("Unsafe_PrefetchWrite");
  oop p = JNIHandles::resolve(obj);
  void* addr = index_oop_from_field_offset_long(p, 0);
  Prefetch::write(addr, (intx)offset);
UNSAFE_END


/// JVM_RegisterUnsafeMethods

#define ADR "J"

#define LANG "Ljava/lang/"

#define OBJ LANG "Object;"
#define CLS LANG "Class;"
#define CTR LANG "reflect/Constructor;"
#define FLD LANG "reflect/Field;"
#define MTH LANG "reflect/Method;"
#define THR LANG "Throwable;"

#define DC0_Args LANG "String;[BII"
#define DC_Args  DC0_Args LANG "ClassLoader;" "Ljava/security/ProtectionDomain;"
#define DAC_Args CLS "[B[" OBJ

#define CC (char*)  /*cast a literal from (const char*)*/
#define FN_PTR(f) CAST_FROM_FN_PTR(void*, &f)

// define deprecated accessors for compabitility with 1.4.0
#define DECLARE_GETSETOOP_140(Boolean, Z) \
    {CC "get" #Boolean,      CC "(" OBJ "I)" #Z,      FN_PTR(Unsafe_Get##Boolean##140)}, \
    {CC "put" #Boolean,      CC "(" OBJ "I" #Z ")V",   FN_PTR(Unsafe_Set##Boolean##140)}

// Note:  In 1.4.1, getObject and kin take both int and long offsets.
#define DECLARE_GETSETOOP_141(Boolean, Z) \
    {CC "get" #Boolean,      CC "(" OBJ "J)" #Z,      FN_PTR(Unsafe_Get##Boolean)}, \
    {CC "put" #Boolean,      CC "(" OBJ "J" #Z ")V",   FN_PTR(Unsafe_Put##Boolean)}

// Note:  In 1.5.0, there are volatile versions too
#define DECLARE_GETSETOOP(Boolean, Z) \
    {CC "get" #Boolean,      CC "(" OBJ "J)" #Z,      FN_PTR(Unsafe_Get##Boolean)}, \
    {CC "put" #Boolean,      CC "(" OBJ "J" #Z ")V",   FN_PTR(Unsafe_Set##Boolean)}, \
    {CC "get" #Boolean "Volatile",      CC "(" OBJ "J)" #Z,      FN_PTR(Unsafe_Get##Boolean##Volatile)}, \
    {CC "put" #Boolean "Volatile",      CC "(" OBJ "J" #Z ")V",   FN_PTR(Unsafe_Put##Boolean##Volatile)}


#define DECLARE_GETSETNATIVE(Byte, B) \
    {CC "get" #Byte,         CC "(" ADR ")" #B,       FN_PTR(Unsafe_GetNative##Byte)}, \
    {CC "put" #Byte,         CC "(" ADR#B ")V",      FN_PTR(Unsafe_SetNative##Byte)}



// These are the methods for 1.4.0
static JNINativeMethod methods_140[] = {
    {CC "getObject",        CC "(" OBJ "I)" OBJ "",   FN_PTR(Unsafe_GetObject140)},
    {CC "putObject",        CC "(" OBJ "I" OBJ ")V",  FN_PTR(Unsafe_SetObject140)},

    DECLARE_GETSETOOP_140(Boolean, Z),
    DECLARE_GETSETOOP_140(Byte, B),
    DECLARE_GETSETOOP_140(Short, S),
    DECLARE_GETSETOOP_140(Char, C),
    DECLARE_GETSETOOP_140(Int, I),
    DECLARE_GETSETOOP_140(Long, J),
    DECLARE_GETSETOOP_140(Float, F),
    DECLARE_GETSETOOP_140(Double, D),

    DECLARE_GETSETNATIVE(Byte, B),
    DECLARE_GETSETNATIVE(Short, S),
    DECLARE_GETSETNATIVE(Char, C),
    DECLARE_GETSETNATIVE(Int, I),
    DECLARE_GETSETNATIVE(Long, J),
    DECLARE_GETSETNATIVE(Float, F),
    DECLARE_GETSETNATIVE(Double, D),

    {CC "getAddress",         CC "(" ADR ")" ADR,             FN_PTR(Unsafe_GetNativeAddress)},
    {CC "putAddress",         CC "(" ADR "" ADR ")V",          FN_PTR(Unsafe_SetNativeAddress)},

    {CC "allocateMemory",     CC "(J)" ADR,                 FN_PTR(Unsafe_AllocateMemory0)},
    {CC "reallocateMemory",   CC "(" ADR "J)" ADR,            FN_PTR(Unsafe_ReallocateMemory0)},
    {CC "freeMemory",         CC "(" ADR ")V",               FN_PTR(Unsafe_FreeMemory0)},

    {CC "fieldOffset",        CC "(" FLD ")I",               FN_PTR(Unsafe_FieldOffset)},
    {CC "staticFieldBase",    CC "(" CLS ")" OBJ,             FN_PTR(Unsafe_StaticFieldBaseFromClass)},
    {CC "ensureClassInitialized",CC "(" CLS ")V",            FN_PTR(Unsafe_EnsureClassInitialized)},
    {CC "arrayBaseOffset",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayBaseOffset)},
    {CC "arrayIndexScale",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayIndexScale)},
    {CC "addressSize",        CC "()I",                    FN_PTR(Unsafe_AddressSize)},
    {CC "pageSize",           CC "()I",                    FN_PTR(Unsafe_PageSize)},

    {CC "defineClass",        CC "(" DC_Args ")" CLS,         FN_PTR(Unsafe_DefineClass0)},
    {CC "allocateInstance",   CC "(" CLS ")" OBJ,             FN_PTR(Unsafe_AllocateInstance)},
    {CC "monitorEnter",       CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorEnter)},
    {CC "monitorExit",        CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorExit)},
    {CC "throwException",     CC "(" THR ")V",               FN_PTR(Unsafe_ThrowException)}
};

// These are the methods prior to the JSR 166 changes in 1.5.0
static JNINativeMethod methods_141[] = {
    {CC "getObject",        CC "(" OBJ "J)" OBJ "",   FN_PTR(Unsafe_GetObject)},
    {CC "putObject",        CC "(" OBJ "J" OBJ ")V",  FN_PTR(Unsafe_SetObject)},

    DECLARE_GETSETOOP_141(Boolean, Z),
    DECLARE_GETSETOOP_141(Byte, B),
    DECLARE_GETSETOOP_141(Short, S),
    DECLARE_GETSETOOP_141(Char, C),
    DECLARE_GETSETOOP_141(Int, I),
    DECLARE_GETSETOOP_141(Long, J),
    DECLARE_GETSETOOP_141(Float, F),
    DECLARE_GETSETOOP_141(Double, D),

    DECLARE_GETSETNATIVE(Byte, B),
    DECLARE_GETSETNATIVE(Short, S),
    DECLARE_GETSETNATIVE(Char, C),
    DECLARE_GETSETNATIVE(Int, I),
    DECLARE_GETSETNATIVE(Long, J),
    DECLARE_GETSETNATIVE(Float, F),
    DECLARE_GETSETNATIVE(Double, D),

    {CC "getAddress",         CC "(" ADR ")" ADR,             FN_PTR(Unsafe_GetNativeAddress)},
    {CC "putAddress",         CC "(" ADR "" ADR ")V",          FN_PTR(Unsafe_SetNativeAddress)},

    {CC "allocateMemory",     CC "(J)" ADR,                 FN_PTR(Unsafe_AllocateMemory0)},
    {CC "reallocateMemory",   CC "(" ADR "J)" ADR,            FN_PTR(Unsafe_ReallocateMemory0)},
    {CC "freeMemory",         CC "(" ADR ")V",               FN_PTR(Unsafe_FreeMemory0)},

    {CC "objectFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_ObjectFieldOffset)},
    {CC "staticFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_StaticFieldOffset)},
    {CC "staticFieldBase",    CC "(" FLD ")" OBJ,             FN_PTR(Unsafe_StaticFieldBaseFromField)},
    {CC "ensureClassInitialized",CC "(" CLS ")V",            FN_PTR(Unsafe_EnsureClassInitialized)},
    {CC "arrayBaseOffset",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayBaseOffset)},
    {CC "arrayIndexScale",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayIndexScale)},
    {CC "addressSize",        CC "()I",                    FN_PTR(Unsafe_AddressSize)},
    {CC "pageSize",           CC "()I",                    FN_PTR(Unsafe_PageSize)},

    {CC "defineClass",        CC "(" DC_Args ")" CLS,         FN_PTR(Unsafe_DefineClass0)},
    {CC "allocateInstance",   CC "(" CLS ")" OBJ,             FN_PTR(Unsafe_AllocateInstance)},
    {CC "monitorEnter",       CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorEnter)},
    {CC "monitorExit",        CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorExit)},
    {CC "throwException",     CC "(" THR ")V",               FN_PTR(Unsafe_ThrowException)}

};

// These are the methods prior to the JSR 166 changes in 1.6.0
static JNINativeMethod methods_15[] = {
    {CC "getObject",        CC "(" OBJ "J)" OBJ "",   FN_PTR(Unsafe_GetObject)},
    {CC "putObject",        CC "(" OBJ "J" OBJ ")V",  FN_PTR(Unsafe_SetObject)},
    {CC "getObjectVolatile",CC "(" OBJ "J)" OBJ "",   FN_PTR(Unsafe_GetObjectVolatile)},
    {CC "putObjectVolatile",CC "(" OBJ "J" OBJ ")V",  FN_PTR(Unsafe_SetObjectVolatile)},


    DECLARE_GETSETOOP(Boolean, Z),
    DECLARE_GETSETOOP(Byte, B),
    DECLARE_GETSETOOP(Short, S),
    DECLARE_GETSETOOP(Char, C),
    DECLARE_GETSETOOP(Int, I),
    DECLARE_GETSETOOP(Long, J),
    DECLARE_GETSETOOP(Float, F),
    DECLARE_GETSETOOP(Double, D),

    DECLARE_GETSETNATIVE(Byte, B),
    DECLARE_GETSETNATIVE(Short, S),
    DECLARE_GETSETNATIVE(Char, C),
    DECLARE_GETSETNATIVE(Int, I),
    DECLARE_GETSETNATIVE(Long, J),
    DECLARE_GETSETNATIVE(Float, F),
    DECLARE_GETSETNATIVE(Double, D),

    {CC "getAddress",         CC "(" ADR ")" ADR,             FN_PTR(Unsafe_GetNativeAddress)},
    {CC "putAddress",         CC "(" ADR "" ADR ")V",          FN_PTR(Unsafe_SetNativeAddress)},

    {CC "allocateMemory",     CC "(J)" ADR,                 FN_PTR(Unsafe_AllocateMemory0)},
    {CC "reallocateMemory",   CC "(" ADR "J)" ADR,            FN_PTR(Unsafe_ReallocateMemory0)},
    {CC "freeMemory",         CC "(" ADR ")V",               FN_PTR(Unsafe_FreeMemory0)},

    {CC "objectFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_ObjectFieldOffset)},
    {CC "staticFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_StaticFieldOffset)},
    {CC "staticFieldBase",    CC "(" FLD ")" OBJ,             FN_PTR(Unsafe_StaticFieldBaseFromField)},
    {CC "ensureClassInitialized",CC "(" CLS ")V",            FN_PTR(Unsafe_EnsureClassInitialized)},
    {CC "arrayBaseOffset",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayBaseOffset)},
    {CC "arrayIndexScale",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayIndexScale)},
    {CC "addressSize",        CC "()I",                    FN_PTR(Unsafe_AddressSize)},
    {CC "pageSize",           CC "()I",                    FN_PTR(Unsafe_PageSize)},

    {CC "defineClass",        CC "(" DC_Args ")" CLS,         FN_PTR(Unsafe_DefineClass0)},
    {CC "allocateInstance",   CC "(" CLS ")" OBJ,             FN_PTR(Unsafe_AllocateInstance)},
    {CC "monitorEnter",       CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorEnter)},
    {CC "monitorExit",        CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorExit)},
    {CC "throwException",     CC "(" THR ")V",               FN_PTR(Unsafe_ThrowException)},
    {CC "compareAndSwapObject", CC "(" OBJ "J" OBJ "" OBJ ")Z",  FN_PTR(Unsafe_CompareAndSwapObject)},
    {CC "compareAndSwapInt",  CC "(" OBJ "J""I""I"")Z",      FN_PTR(Unsafe_CompareAndSwapInt)},
    {CC "compareAndSwapLong", CC "(" OBJ "J""J""J"")Z",      FN_PTR(Unsafe_CompareAndSwapLong)},
    {CC "park",               CC "(ZJ)V",                  FN_PTR(Unsafe_Park)},
    {CC "unpark",             CC "(" OBJ ")V",               FN_PTR(Unsafe_Unpark)}

};

// These are the methods for 1.6.0 and 1.7.0
static JNINativeMethod methods_16[] = {
    {CC "getObject",        CC "(" OBJ "J)" OBJ "",   FN_PTR(Unsafe_GetObject)},
    {CC "putObject",        CC "(" OBJ "J" OBJ ")V",  FN_PTR(Unsafe_SetObject)},
    {CC "getObjectVolatile",CC "(" OBJ "J)" OBJ "",   FN_PTR(Unsafe_GetObjectVolatile)},
    {CC "putObjectVolatile",CC "(" OBJ "J" OBJ ")V",  FN_PTR(Unsafe_SetObjectVolatile)},

    DECLARE_GETSETOOP(Boolean, Z),
    DECLARE_GETSETOOP(Byte, B),
    DECLARE_GETSETOOP(Short, S),
    DECLARE_GETSETOOP(Char, C),
    DECLARE_GETSETOOP(Int, I),
    DECLARE_GETSETOOP(Long, J),
    DECLARE_GETSETOOP(Float, F),
    DECLARE_GETSETOOP(Double, D),

    DECLARE_GETSETNATIVE(Byte, B),
    DECLARE_GETSETNATIVE(Short, S),
    DECLARE_GETSETNATIVE(Char, C),
    DECLARE_GETSETNATIVE(Int, I),
    DECLARE_GETSETNATIVE(Long, J),
    DECLARE_GETSETNATIVE(Float, F),
    DECLARE_GETSETNATIVE(Double, D),

    {CC "getAddress",         CC "(" ADR ")" ADR,             FN_PTR(Unsafe_GetNativeAddress)},
    {CC "putAddress",         CC "(" ADR "" ADR ")V",          FN_PTR(Unsafe_SetNativeAddress)},

    {CC "allocateMemory",     CC "(J)" ADR,                 FN_PTR(Unsafe_AllocateMemory0)},
    {CC "reallocateMemory",   CC "(" ADR "J)" ADR,            FN_PTR(Unsafe_ReallocateMemory0)},
    {CC "freeMemory",         CC "(" ADR ")V",               FN_PTR(Unsafe_FreeMemory0)},

    {CC "objectFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_ObjectFieldOffset)},
    {CC "staticFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_StaticFieldOffset)},
    {CC "staticFieldBase",    CC "(" FLD ")" OBJ,             FN_PTR(Unsafe_StaticFieldBaseFromField)},
    {CC "ensureClassInitialized",CC "(" CLS ")V",            FN_PTR(Unsafe_EnsureClassInitialized)},
    {CC "arrayBaseOffset",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayBaseOffset)},
    {CC "arrayIndexScale",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayIndexScale)},
    {CC "addressSize",        CC "()I",                    FN_PTR(Unsafe_AddressSize)},
    {CC "pageSize",           CC "()I",                    FN_PTR(Unsafe_PageSize)},

    {CC "defineClass",        CC "(" DC_Args ")" CLS,         FN_PTR(Unsafe_DefineClass0)},
    {CC "allocateInstance",   CC "(" CLS ")" OBJ,             FN_PTR(Unsafe_AllocateInstance)},
    {CC "monitorEnter",       CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorEnter)},
    {CC "monitorExit",        CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorExit)},
    {CC "tryMonitorEnter",    CC "(" OBJ ")Z",               FN_PTR(Unsafe_TryMonitorEnter)},
    {CC "throwException",     CC "(" THR ")V",               FN_PTR(Unsafe_ThrowException)},
    {CC "compareAndSwapObject", CC "(" OBJ "J" OBJ "" OBJ ")Z",  FN_PTR(Unsafe_CompareAndSwapObject)},
    {CC "compareAndSwapInt",  CC "(" OBJ "J""I""I"")Z",      FN_PTR(Unsafe_CompareAndSwapInt)},
    {CC "compareAndSwapLong", CC "(" OBJ "J""J""J"")Z",      FN_PTR(Unsafe_CompareAndSwapLong)},
    {CC "putOrderedObject",   CC "(" OBJ "J" OBJ ")V",         FN_PTR(Unsafe_SetOrderedObject)},
    {CC "putOrderedInt",      CC "(" OBJ "JI)V",             FN_PTR(Unsafe_SetOrderedInt)},
    {CC "putOrderedLong",     CC "(" OBJ "JJ)V",             FN_PTR(Unsafe_SetOrderedLong)},
    {CC "park",               CC "(ZJ)V",                  FN_PTR(Unsafe_Park)},
    {CC "unpark",             CC "(" OBJ ")V",               FN_PTR(Unsafe_Unpark)}
};

// These are the methods for 1.8.0
static JNINativeMethod methods_18[] = {
    {CC "getObject",        CC "(" OBJ "J)" OBJ "",   FN_PTR(Unsafe_GetObject)},
    {CC "putObject",        CC "(" OBJ "J" OBJ ")V",  FN_PTR(Unsafe_SetObject)},
    {CC "getObjectVolatile",CC "(" OBJ "J)" OBJ "",   FN_PTR(Unsafe_GetObjectVolatile)},
    {CC "putObjectVolatile",CC "(" OBJ "J" OBJ ")V",  FN_PTR(Unsafe_SetObjectVolatile)},

    DECLARE_GETSETOOP(Boolean, Z),
    DECLARE_GETSETOOP(Byte, B),
    DECLARE_GETSETOOP(Short, S),
    DECLARE_GETSETOOP(Char, C),
    DECLARE_GETSETOOP(Int, I),
    DECLARE_GETSETOOP(Long, J),
    DECLARE_GETSETOOP(Float, F),
    DECLARE_GETSETOOP(Double, D),

    DECLARE_GETSETNATIVE(Byte, B),
    DECLARE_GETSETNATIVE(Short, S),
    DECLARE_GETSETNATIVE(Char, C),
    DECLARE_GETSETNATIVE(Int, I),
    DECLARE_GETSETNATIVE(Long, J),
    DECLARE_GETSETNATIVE(Float, F),
    DECLARE_GETSETNATIVE(Double, D),

    {CC "getAddress",         CC "(" ADR ")" ADR,             FN_PTR(Unsafe_GetNativeAddress)},
    {CC "putAddress",         CC "(" ADR "" ADR ")V",          FN_PTR(Unsafe_SetNativeAddress)},

    {CC "allocateMemory",     CC "(J)" ADR,                 FN_PTR(Unsafe_AllocateMemory0)},
    {CC "reallocateMemory",   CC "(" ADR "J)" ADR,            FN_PTR(Unsafe_ReallocateMemory0)},
    {CC "freeMemory",         CC "(" ADR ")V",               FN_PTR(Unsafe_FreeMemory0)},

    {CC "objectFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_ObjectFieldOffset)},
    {CC "staticFieldOffset",  CC "(" FLD ")J",               FN_PTR(Unsafe_StaticFieldOffset)},
    {CC "staticFieldBase",    CC "(" FLD ")" OBJ,             FN_PTR(Unsafe_StaticFieldBaseFromField)},
    {CC "ensureClassInitialized",CC "(" CLS ")V",            FN_PTR(Unsafe_EnsureClassInitialized)},
    {CC "arrayBaseOffset",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayBaseOffset)},
    {CC "arrayIndexScale",    CC "(" CLS ")I",               FN_PTR(Unsafe_ArrayIndexScale)},
    {CC "addressSize",        CC "()I",                    FN_PTR(Unsafe_AddressSize)},
    {CC "pageSize",           CC "()I",                    FN_PTR(Unsafe_PageSize)},

    {CC "defineClass",        CC "(" DC_Args ")" CLS,         FN_PTR(Unsafe_DefineClass0)},
    {CC "allocateInstance",   CC "(" CLS ")" OBJ,             FN_PTR(Unsafe_AllocateInstance)},
    {CC "monitorEnter",       CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorEnter)},
    {CC "monitorExit",        CC "(" OBJ ")V",               FN_PTR(Unsafe_MonitorExit)},
    {CC "tryMonitorEnter",    CC "(" OBJ ")Z",               FN_PTR(Unsafe_TryMonitorEnter)},
    {CC "throwException",     CC "(" THR ")V",               FN_PTR(Unsafe_ThrowException)},
    {CC "compareAndSwapObject", CC "(" OBJ "J" OBJ "" OBJ ")Z",  FN_PTR(Unsafe_CompareAndSwapObject)},
    {CC "compareAndSwapInt",  CC "(" OBJ "J""I""I"")Z",      FN_PTR(Unsafe_CompareAndSwapInt)},
    {CC "compareAndSwapLong", CC "(" OBJ "J""J""J"")Z",      FN_PTR(Unsafe_CompareAndSwapLong)},
    {CC "putOrderedObject",   CC "(" OBJ "J" OBJ ")V",         FN_PTR(Unsafe_SetOrderedObject)},
    {CC "putOrderedInt",      CC "(" OBJ "JI)V",             FN_PTR(Unsafe_SetOrderedInt)},
    {CC "putOrderedLong",     CC "(" OBJ "JJ)V",             FN_PTR(Unsafe_SetOrderedLong)},
    {CC "park",               CC "(ZJ)V",                  FN_PTR(Unsafe_Park)},
    {CC "unpark",             CC "(" OBJ ")V",               FN_PTR(Unsafe_Unpark)}
};

JNINativeMethod loadavg_method[] = {
    {CC "getLoadAverage",     CC "([DI)I",                 FN_PTR(Unsafe_GetLoadAverage0)}
};

JNINativeMethod prefetch_methods[] = {
    {CC "prefetchRead",       CC "(" OBJ "J)V",              FN_PTR(Unsafe_PrefetchRead)},
    {CC "prefetchWrite",      CC "(" OBJ "J)V",              FN_PTR(Unsafe_PrefetchWrite)},
    {CC "prefetchReadStatic", CC "(" OBJ "J)V",              FN_PTR(Unsafe_PrefetchRead)},
    {CC "prefetchWriteStatic",CC "(" OBJ "J)V",              FN_PTR(Unsafe_PrefetchWrite)}
};

JNINativeMethod memcopy_methods_17[] = {
    {CC "copyMemory",         CC "(" OBJ "J" OBJ "JJ)V",       FN_PTR(Unsafe_CopyMemory0)},
    {CC "setMemory",          CC "(" OBJ "JJB)V",            FN_PTR(Unsafe_SetMemory0)}
};

JNINativeMethod memcopy_methods_15[] = {
    {CC "setMemory",          CC "(" ADR "JB)V",             FN_PTR(Unsafe_SetMemory)},
    {CC "copyMemory",         CC "(" ADR ADR "J)V",          FN_PTR(Unsafe_CopyMemory)}
};

JNINativeMethod anonk_methods[] = {
    {CC "defineAnonymousClass", CC "(" DAC_Args ")" CLS,      FN_PTR(Unsafe_DefineAnonymousClass)},
};

JNINativeMethod lform_methods[] = {
    {CC "shouldBeInitialized",CC "(" CLS ")Z",               FN_PTR(Unsafe_ShouldBeInitialized)},
};

JNINativeMethod fence_methods[] = {
    {CC "loadFence",          CC "()V",                    FN_PTR(Unsafe_LoadFence)},
    {CC "storeFence",         CC "()V",                    FN_PTR(Unsafe_StoreFence)},
    {CC "fullFence",          CC "()V",                    FN_PTR(Unsafe_FullFence)},
};

#undef CC
#undef FN_PTR

#undef ADR
#undef LANG
#undef OBJ
#undef CLS
#undef CTR
#undef FLD
#undef MTH
#undef THR
#undef DC0_Args
#undef DC_Args

#undef DECLARE_GETSETOOP
#undef DECLARE_GETSETNATIVE

#undef Unsafe_CompareAndSwapObject
#undef Unsafe_CompareAndSwapInt
#undef Unsafe_CompareAndSwapLong

#undef Unsafe_GetObject
#undef Unsafe_SetObject
#undef Unsafe_GetObjectVolatile
#undef Unsafe_SetObjectVolatile
#undef Unsafe_GetObject140
#undef Unsafe_SetObject140

#undef Unsafe_SetBoolean
#undef Unsafe_SetByte
#undef Unsafe_SetShort
#undef Unsafe_SetChar
#undef Unsafe_SetInt
#undef Unsafe_SetLong
#undef Unsafe_SetFloat
#undef Unsafe_SetDouble

#undef Unsafe_GetBoolean140
#undef Unsafe_GetByte140
#undef Unsafe_GetShort140
#undef Unsafe_GetChar140
#undef Unsafe_GetInt140
#undef Unsafe_GetLong140
#undef Unsafe_GetFloat140
#undef Unsafe_GetDouble140
#undef Unsafe_SetBoolean140
#undef Unsafe_SetByte140
#undef Unsafe_SetShort140
#undef Unsafe_SetChar140
#undef Unsafe_SetInt140
#undef Unsafe_SetLong140
#undef Unsafe_SetFloat140
#undef Unsafe_SetDouble140

#undef Unsafe_SetOrderedObject
#undef Unsafe_SetOrderedLong

/**
 * Helper method to register native methods.
 */
static bool register_natives(const char* message, JNIEnv* env, jclass clazz, const JNINativeMethod* methods, jint nMethods) {
  int status = env->RegisterNatives(clazz, methods, nMethods);
  if (status < 0 || env->ExceptionOccurred()) {
    if (PrintMiscellaneous && (Verbose || WizardMode)) {
      tty->print_cr("Unsafe:  failed registering %s", message);
    }
    env->ExceptionClear();
    return false;
  } else {
    if (PrintMiscellaneous && (Verbose || WizardMode)) {
      tty->print_cr("Unsafe:  successfully registered %s", message);
    }
    return true;
  }
}

JVM_ENTRY(void, JVM_RegisterSunMiscUnsafeMethods(JNIEnv *env, jclass unsafecls)) {
  ThreadToNativeFromVM ttnfv(thread);

  // Unsafe methods
  {
    bool success = false;
    // We need to register the 1.6 methods first because the 1.8 methods would register fine on 1.7 and 1.6
    if (!success) {
      success = register_natives("1.6 methods",   env, unsafecls, methods_16,  sizeof(methods_16)/sizeof(JNINativeMethod));
    }
    if (!success) {
      success = register_natives("1.8 methods",   env, unsafecls, methods_18,  sizeof(methods_18)/sizeof(JNINativeMethod));
    }
    if (!success) {
      success = register_natives("1.5 methods",   env, unsafecls, methods_15,  sizeof(methods_15)/sizeof(JNINativeMethod));
    }
    if (!success) {
      success = register_natives("1.4.1 methods", env, unsafecls, methods_141, sizeof(methods_141)/sizeof(JNINativeMethod));
    }
    if (!success) {
      success = register_natives("1.4.0 methods", env, unsafecls, methods_140, sizeof(methods_140)/sizeof(JNINativeMethod));
    }
    guarantee(success, "register unsafe natives");
  }

  // Unsafe.getLoadAverage
  register_natives("1.6 loadavg method", env, unsafecls, loadavg_method, sizeof(loadavg_method)/sizeof(JNINativeMethod));

  // Prefetch methods
  //register_natives("1.6 prefetch methods", env, unsafecls, prefetch_methods, sizeof(prefetch_methods)/sizeof(JNINativeMethod));

  // Memory copy methods
  {
    bool success = false;
    if (!success) {
      success = register_natives("1.7 memory copy methods", env, unsafecls, memcopy_methods_17, sizeof(memcopy_methods_17)/sizeof(JNINativeMethod));
    }
    if (!success) {
      success = register_natives("1.5 memory copy methods", env, unsafecls, memcopy_methods_15, sizeof(memcopy_methods_15)/sizeof(JNINativeMethod));
    }
  }

  // Unsafe.defineAnonymousClass
  register_natives("1.7 define anonymous class method", env, unsafecls, anonk_methods, sizeof(anonk_methods)/sizeof(JNINativeMethod));
  // Unsafe.shouldBeInitialized
  register_natives("1.7 LambdaForm support", env, unsafecls, lform_methods, sizeof(lform_methods)/sizeof(JNINativeMethod));
  // Fence methods
  register_natives("1.8 fence methods", env, unsafecls, fence_methods, sizeof(fence_methods)/sizeof(JNINativeMethod));
}
JVM_END
