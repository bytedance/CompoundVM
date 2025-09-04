/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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


#ifndef SHARE_PRIMS_UNSAFE_HPP
#define SHARE_PRIMS_UNSAFE_HPP

#include "jni.h"

extern "C" {
  void JNICALL JVM_RegisterJDKInternalMiscUnsafeMethods(JNIEnv *env, jclass unsafecls);
#if HOTSPOT_TARGET_CLASSLIB == 8
  void JNICALL JVM_RegisterSunMiscUnsafeMethods(JNIEnv *env, jclass unsafecls);
#endif
}

jlong Unsafe_field_offset_to_byte_offset(jlong field_offset);

jlong Unsafe_field_offset_from_byte_offset(jlong byte_offset);

#if HOTSPOT_TARGET_CLASSLIB == 8
#include "sanitizers/ub.hpp"
#include "runtime/jniHandles.inline.hpp"

static inline void* addr_from_java(jlong addr) {
  // This assert fails in a variety of ways on 32-bit systems.
  // It is impossible to predict whether native code that converts
  // pointers to longs will sign-extend or zero-extend the addresses.
  //assert(addr == (uintptr_t)addr, "must not be odd high bits");
  return (void*)(uintptr_t)addr;
}

static inline jlong addr_to_java(void* p) {
  assert(p == (void*)(uintptr_t)p, "must not be odd high bits");
  return (uintptr_t)p;
}

class GuardUnsafeAccess {
  JavaThread* _thread;

public:
  GuardUnsafeAccess(JavaThread* thread) : _thread(thread) {
    // native/off-heap access which may raise SIGBUS if accessing
    // memory mapped file data in a region of the file which has
    // been truncated and is now invalid.
    _thread->set_doing_unsafe_access(true);
  }

  ~GuardUnsafeAccess() {
    _thread->set_doing_unsafe_access(false);
  }
};

#define MAX_OBJECT_SIZE \
  ( arrayOopDesc::base_offset_in_bytes(T_DOUBLE) \
    + ((julong)max_jint * sizeof(double)) )

static inline jlong field_offset_to_byte_offset(jlong field_offset) {
  return field_offset;
}

static inline int field_offset_from_byte_offset(int byte_offset) {
  return byte_offset;
}

static inline void assert_field_offset_sane(oop p, jlong field_offset) {
#ifdef ASSERT
  jlong byte_offset = field_offset_to_byte_offset(field_offset);

  if (p != nullptr) {
    assert(byte_offset >= 0 && byte_offset <= (jlong)MAX_OBJECT_SIZE, "sane offset");
    if (byte_offset == (jint)byte_offset) {
      void* ptr_plus_disp = cast_from_oop<address>(p) + byte_offset;
      assert(p->field_addr<void>((jint)byte_offset) == ptr_plus_disp,
             "raw [ptr+disp] must be consistent with oop::field_addr");
    }
    jlong p_size = HeapWordSize * (jlong)(p->size());
    assert(byte_offset < p_size, "Unsafe access: offset " INT64_FORMAT " > object's size " INT64_FORMAT, (int64_t)byte_offset, (int64_t)p_size);
  }
#endif
}

static inline void* index_oop_from_field_offset_long(oop p, jlong field_offset) {
  assert_field_offset_sane(p, field_offset);
  uintptr_t base_address = cast_from_oop<uintptr_t>(p);
  uintptr_t byte_offset  = (uintptr_t)field_offset_to_byte_offset(field_offset);
  return (void*)(base_address + byte_offset);
}

/**
 * Helper class for accessing memory.
 *
 * Normalizes values and wraps accesses in
 * JavaThread::doing_unsafe_access() if needed.
 */
template <typename T>
class MemoryAccess : StackObj {
  JavaThread* _thread;
  oop _obj;
  ptrdiff_t _offset;

  // Resolves and returns the address of the memory access.
  // This raw memory access may fault, so we make sure it happens within the
  // guarded scope by making the access volatile at least. Since the store
  // of Thread::set_doing_unsafe_access() is also volatile, these accesses
  // can not be reordered by the compiler. Therefore, if the access triggers
  // a fault, we will know that Thread::doing_unsafe_access() returns true.
  volatile T* addr() {
    void* addr = index_oop_from_field_offset_long(_obj, _offset);
    return static_cast<volatile T*>(addr);
  }

  template <typename U>
  U normalize_for_write(U x) {
    return x;
  }

  jboolean normalize_for_write(jboolean x) {
    return x & 1;
  }

  template <typename U>
  U normalize_for_read(U x) {
    return x;
  }

  jboolean normalize_for_read(jboolean x) {
    return x != 0;
  }

public:
  MemoryAccess(JavaThread* thread, jobject obj, jlong offset)
    : _thread(thread), _obj(JNIHandles::resolve(obj)), _offset((ptrdiff_t)offset) {
    assert_field_offset_sane(_obj, offset);
  }

  T get() {
    GuardUnsafeAccess guard(_thread);
    return normalize_for_read(*addr());
  }

  // we use this method at some places for writing to 0 e.g. to cause a crash;
  // ubsan does not know that this is the desired behavior
  ATTRIBUTE_NO_UBSAN
  void put(T x) {
    GuardUnsafeAccess guard(_thread);
    *addr() = normalize_for_write(x);
  }


  T get_volatile() {
    GuardUnsafeAccess guard(_thread);
    volatile T ret = RawAccess<MO_SEQ_CST>::load(addr());
    return normalize_for_read(ret);
  }

  void put_volatile(T x) {
    GuardUnsafeAccess guard(_thread);
    RawAccess<MO_SEQ_CST>::store(addr(), normalize_for_write(x));
  }
};
#endif // HOTSPOT_TARGET_CLASSLIB

#endif // SHARE_PRIMS_UNSAFE_HPP
