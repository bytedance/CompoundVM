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

#if defined(HOTSPOT_TARGET_CLASSLIB) && HOTSPOT_TARGET_CLASSLIB == 8

#include "precompiled.hpp"
#include "jvm.h"
#include "cds/classListParser.hpp"
#include "cds/classListWriter.hpp"
#include "cds/dynamicArchive.hpp"
#include "cds/heapShared.hpp"
#include "cds/lambdaFormInvokers.hpp"
#include "classfile/classFileStream.hpp"
#include "classfile/classLoader.hpp"
#include "classfile/classLoaderData.hpp"
#include "classfile/classLoaderData.inline.hpp"
#include "classfile/classLoadInfo.hpp"
#include "classfile/javaAssertions.hpp"
#include "classfile/javaClasses.inline.hpp"
#include "classfile/moduleEntry.hpp"
#include "classfile/modules.hpp"
#include "classfile/packageEntry.hpp"
#include "classfile/stringTable.hpp"
#include "classfile/symbolTable.hpp"
#include "classfile/systemDictionary.hpp"
#include "classfile/vmClasses.hpp"
#include "classfile/vmSymbols.hpp"
#include "gc/shared/collectedHeap.inline.hpp"
#include "interpreter/bytecode.hpp"
#include "interpreter/bytecodeUtils.hpp"
#include "jfr/jfrEvents.hpp"
#include "logging/log.hpp"
#include "memory/oopFactory.hpp"
#include "memory/referenceType.hpp"
#include "memory/resourceArea.hpp"
#include "memory/universe.hpp"
#include "oops/access.inline.hpp"
#include "oops/constantPool.hpp"
#include "oops/fieldStreams.inline.hpp"
#include "oops/instanceKlass.hpp"
#include "oops/klass.inline.hpp"
#include "oops/method.hpp"
#include "oops/recordComponent.hpp"
#include "oops/objArrayKlass.hpp"
#include "oops/objArrayOop.inline.hpp"
#include "oops/oop.inline.hpp"
#include "prims/jvm_misc.hpp"
#include "prims/jvmtiExport.hpp"
#include "prims/jvmtiThreadState.hpp"
#include "prims/stackwalk.hpp"
#include "runtime/arguments.hpp"
#include "runtime/atomic.hpp"
#include "runtime/globals_extension.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/init.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/deoptimization.hpp"
#include "runtime/handshake.hpp"
#include "runtime/java.hpp"
#include "runtime/javaCalls.hpp"
#include "runtime/jfieldIDWorkaround.hpp"
#include "runtime/jniHandles.inline.hpp"
#include "runtime/os.inline.hpp"
#include "runtime/osThread.hpp"
#include "runtime/perfData.hpp"
#include "runtime/reflection.hpp"
#include "runtime/synchronizer.hpp"
#include "runtime/thread.inline.hpp"
#include "runtime/threadSMR.hpp"
#include "runtime/vframe.inline.hpp"
#include "runtime/vmOperations.hpp"
#include "runtime/vm_version.hpp"
#include "services/attachListener.hpp"
#include "services/management.hpp"
#include "services/threadService.hpp"
#include "utilities/copy.hpp"
#include "utilities/defaultStream.hpp"
#include "utilities/dtrace.hpp"
#include "utilities/events.hpp"
#include "utilities/macros.hpp"
#include "utilities/utf8.hpp"
#if INCLUDE_CDS
#include "classfile/systemDictionaryShared.hpp"
#endif
#if INCLUDE_JFR
#include "jfr/jfr.hpp"
#endif

#include <errno.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////
///  JDK8 compatibility
////////////////////////////////////////////////////////////////////////////////////////////////////////

JVM_LEAF(void, JVM_OnExit(void (*func)(void)))
  //register_on_exit_function(func);
  log_warning(thread)("JVM_OnExit not implemented");
JVM_END

JVM_LEAF(void, JVM_TraceInstructions(jboolean on))
  log_warning(thread)("JVM_TraceInstructions not supported");
JVM_END


JVM_LEAF(void, JVM_TraceMethodCalls(jboolean on))
  log_warning(thread)("JVM_TraceMethodCalls not supported");
JVM_END

JVM_ENTRY(jint, JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable))
  oop exception = JNIHandles::resolve(throwable);
  return java_lang_Throwable::depth(exception);
JVM_END

JVM_ENTRY(jobject, JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index))
  JvmtiVMObjectAllocEventCollector oam; // This ctor (throughout this module) may trigger a safepoint/GC
  Handle exception(THREAD, JNIHandles::resolve(throwable));
  oop element = java_lang_Throwable::get_stack_trace_element(exception, index, CHECK_NULL);
  return JNIHandles::make_local(THREAD, element);
JVM_END

// java.lang.Compiler ////////////////////////////////////////////////////

// The initial cuts of the HotSpot VM will not support JITs, and all existing
// JITs would need extensive changes to work with HotSpot.  The JIT-related JVM
// functions are all silently ignored unless JVM warnings are printed.

JVM_LEAF(void, JVM_InitializeCompiler (JNIEnv *env, jclass compCls))
  log_warning(thread)("JVM_InitializeCompiler not supported");
JVM_END


JVM_LEAF(jboolean, JVM_IsSilentCompiler(JNIEnv *env, jclass compCls))
  log_warning(thread)("JVM_IsSilentCompiler not supported");
  return JNI_FALSE;
JVM_END


JVM_LEAF(jboolean, JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls))
  log_warning(thread)("JVM_CompileClass not supported");
  return JNI_FALSE;
JVM_END


JVM_LEAF(jboolean, JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname))
  log_warning(thread)("JVM_CompileClasses not supported");
  return JNI_FALSE;
JVM_END


JVM_LEAF(jobject, JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg))
  log_warning(thread)("JVM_CompilerCommand not supported");
  return NULL;
JVM_END


JVM_LEAF(void, JVM_EnableCompiler(JNIEnv *env, jclass compCls))
  log_warning(thread)("JVM_EnableCompiler not supported");
JVM_END


JVM_LEAF(void, JVM_DisableCompiler(JNIEnv *env, jclass compCls))
  log_warning(thread)("JVM_DisableCompiler not supported");
JVM_END



// Error message support //////////////////////////////////////////////////////

JVM_LEAF(jint, JVM_GetLastErrorString(char *buf, int len))
  return (jint)os::lasterror(buf, len);
JVM_END

// java.nio.Bits ///////////////////////////////////////////////////////////////

#define MAX_OBJECT_SIZE \
  ( arrayOopDesc::header_size(T_DOUBLE) * HeapWordSize \
    + ((julong)max_jint * sizeof(double)) )

static inline jlong field_offset_to_byte_offset(jlong field_offset) {
  return field_offset;
}

//static inline void assert_field_offset_sane(oop p, jlong field_offset) {
//#ifdef ASSERT
//  jlong byte_offset = field_offset_to_byte_offset(field_offset);
//
//  if (p != NULL) {
//    assert(byte_offset >= 0 && byte_offset <= (jlong)MAX_OBJECT_SIZE, "sane offset");
//    if (byte_offset == (jint)byte_offset) {
//      void* ptr_plus_disp = (address)p + byte_offset;
//      assert((void*)p->obj_field_addr<oop>((jint)byte_offset) == ptr_plus_disp,
//             "raw [ptr+disp] must be consistent with oop::field_base");
//    }
//    jlong p_size = HeapWordSize * (jlong)(p->size());
//    assert(byte_offset < p_size, err_msg("Unsafe access: offset " INT64_FORMAT
//                                         " > object's size " INT64_FORMAT,
//                                         (int64_t)byte_offset, (int64_t)p_size));
//  }
//#endif
//}

static inline void* index_oop_from_field_offset_long(oop p, jlong field_offset) {
  //assert_field_offset_sane(p, field_offset);
  jlong byte_offset = field_offset_to_byte_offset(field_offset);

  if (sizeof(char*) == sizeof(jint)) {   // (this constant folds!)
    return cast_from_oop<address>(p) + (jint) byte_offset;
  } else {
    return cast_from_oop<address>(p) +        byte_offset;
  }
}

// This function is a leaf since if the source and destination are both in native memory
// the copy may potentially be very large, and we don't want to disable GC if we can avoid it.
// If either source or destination (or both) are on the heap, the function will enter VM using
// JVM_ENTRY_FROM_LEAF
JVM_LEAF(void, JVM_CopySwapMemory(JNIEnv *env, jobject srcObj, jlong srcOffset,
                                  jobject dstObj, jlong dstOffset, jlong size,
                                  jlong elemSize)) {

  size_t sz = (size_t)size;
  size_t esz = (size_t)elemSize;

  if (srcObj == NULL && dstObj == NULL) {
    // Both src & dst are in native memory
    address src = (address)srcOffset;
    address dst = (address)dstOffset;

    Copy::conjoint_swap(src, dst, sz, esz);
  } else {
    // At least one of src/dst are on heap, transition to VM to access raw pointers

JVM_ENTRY_FROM_LEAF(env, void, JVM_CopySwapMemory) {
      oop srcp = JNIHandles::resolve(srcObj);
      oop dstp = JNIHandles::resolve(dstObj);

      address src = (address)index_oop_from_field_offset_long(srcp, srcOffset);
      address dst = (address)index_oop_from_field_offset_long(dstp, dstOffset);

      Copy::conjoint_swap(src, dst, sz, esz);
    } JVM_END
  }
} JVM_END


JVM_ENTRY(void, JVM_ResolveClass(JNIEnv* env, jclass cls))
  log_warning(class)("JVM_ResolveClass not implemented");
JVM_END


JVM_ENTRY(jboolean, JVM_KnownToNotExist(JNIEnv *env, jobject loader, const char *classname))
//#if INCLUDE_CDS
//  return ClassLoaderExt::known_to_not_exist(env, loader, classname, THREAD);
//#endif
  return false;
JVM_END


JVM_ENTRY(jobjectArray, JVM_GetResourceLookupCacheURLs(JNIEnv *env, jobject loader))
//#if INCLUDE_CDS
//  return ClassLoaderExt::get_lookup_cache_urls(env, loader, THREAD);
//#else
  return NULL;
//#endif
JVM_END


JVM_ENTRY(jintArray, JVM_GetResourceLookupCache(JNIEnv *env, jobject loader, const char *resource_name))
//#if INCLUDE_CDS
//  return ClassLoaderExt::get_lookup_cache(env, loader, resource_name, THREAD);
//#endif
  return jintArray(NULL);
JVM_END

// Not used; JVM_FindClassFromCaller replaces this.
JVM_ENTRY(jclass, JVM_FindClassFromClassLoader(JNIEnv* env, const char* name,
                                               jboolean init, jobject loader,
                                               jboolean throwError))
  // Java libraries should ensure that name is never null...
  if (name == NULL || (int)strlen(name) > Symbol::max_length()) {
    // It's impossible to create this class;  the name cannot fit
    // into the constant pool.
    if (throwError) {
      THROW_MSG_0(vmSymbols::java_lang_NoClassDefFoundError(), name);
    } else {
      THROW_MSG_0(vmSymbols::java_lang_ClassNotFoundException(), name);
    }
  }
  TempNewSymbol h_name = SymbolTable::new_symbol(name);
  Handle h_loader(THREAD, JNIHandles::resolve(loader));
  jclass result = find_class_from_class_loader(env, h_name, init, h_loader,
                                               Handle(), throwError, THREAD);

  if (result != NULL) {
    trace_class_resolution(java_lang_Class::as_Klass(JNIHandles::resolve_non_null(result)));
  }
  return result;
JVM_END

// Reflection support //////////////////////////////////////////////////////////////////////////////

JVM_ENTRY(jstring, JVM_GetClassName(JNIEnv *env, jclass cls))
  assert (cls != NULL, "illegal class");
  JvmtiVMObjectAllocEventCollector oam;
  ResourceMark rm(THREAD);
  const char* name;
  if (java_lang_Class::is_primitive(JNIHandles::resolve(cls))) {
    name = type2name(java_lang_Class::primitive_type(JNIHandles::resolve(cls)));
  } else {
    // Consider caching interned string in Klass
    Klass* k = java_lang_Class::as_Klass(JNIHandles::resolve(cls));
    assert(k->is_klass(), "just checking");
    name = k->external_name();
  }
  oop result = StringTable::intern((char*) name, CHECK_NULL);
  return (jstring) JNIHandles::make_local(THREAD, result);
JVM_END

JVM_ENTRY(jobject, JVM_GetClassLoader(JNIEnv *env, jclass cls))
  if (java_lang_Class::is_primitive(JNIHandles::resolve_non_null(cls))) {
    return NULL;
  }
  Klass* k = java_lang_Class::as_Klass(JNIHandles::resolve_non_null(cls));
  oop loader = k->class_loader();
  return JNIHandles::make_local(THREAD, loader);
JVM_END

JVM_ENTRY(jclass, JVM_GetComponentType(JNIEnv *env, jclass cls))
  oop mirror = JNIHandles::resolve_non_null(cls);
  oop result = Reflection::array_component_type(mirror, CHECK_NULL);
  return (jclass) JNIHandles::make_local(THREAD, result);
JVM_END


//******************* remoted in version >jdk11 *********************
JVM_ENTRY(jint, JVM_CountStackFrames(JNIEnv* env, jobject jthread))
  ThreadsListHandle tlh(thread);
  JavaThread* receiver = NULL;
  bool is_alive = tlh.cv_internal_thread_to_JavaThread(jthread, &receiver, NULL);
  int count = 0;
  if (is_alive) {
    // jthread refers to a live JavaThread.
    if (receiver->is_suspended()) {
      // Count all java activation, i.e., number of vframes.
      for (vframeStream vfst(receiver); !vfst.at_end(); vfst.next()) {
        // Native frames are not counted.
        if (!vfst.method()->is_native()) count++;
      }
    } else {
      THROW_MSG_0(vmSymbols::java_lang_IllegalThreadStateException(),
                  "this thread is not suspended");
    }
  }
  // Implied else: if JavaThread is not alive simply return a count of 0.
  return count;
JVM_END

JVM_ENTRY(jboolean, JVM_IsInterrupted(JNIEnv* env, jobject jthread, jboolean clear_interrupted))
  ThreadsListHandle tlh(thread);
  JavaThread* receiver = NULL;
  bool is_alive = tlh.cv_internal_thread_to_JavaThread(jthread, &receiver, NULL);
  if (is_alive) {
    // jthread refers to a live JavaThread.
    return (jboolean) receiver->is_interrupted(clear_interrupted != 0);
  } else {
    return JNI_FALSE;
  }
JVM_END

// IO functions ////////////////////////////////////////////////////////////////////////////////////////

// This must match that definition in JDK8u
#define JVM_O_DELETE 0x10000

JVM_LEAF(jint, JVM_Open(const char *fname, jint flags, jint mode))
  //%note jvm_r6
  int o_delete = (flags & JVM_O_DELETE);
  flags = flags & ~JVM_O_DELETE;

  int result = os::open(fname, flags, mode);

  if (result >= 0) {
    if (o_delete != 0) {
      os::unlink(fname);
    }
    return result;
  } else {
    switch(errno) {
      case EEXIST:
        return JVM_EEXIST;
      default:
        return -1;
    }
  }
JVM_END


JVM_LEAF(jint, JVM_Close(jint fd))
  //%note jvm_r6
  return os::close(fd);
JVM_END


JVM_LEAF(jint, JVM_Read(jint fd, char *buf, jint nbytes))
  size_t res;
  do {
    res = ::read(fd, buf, nbytes);;
  } while(((int)res == OS_ERR) && (errno == EINTR));
  return (jint)res;
JVM_END


JVM_LEAF(jint, JVM_Write(jint fd, char *buf, jint nbytes))
  //%note jvm_r6
  return (jint)os::write(fd, buf, nbytes);
JVM_END

JVM_LEAF(jint, JVM_Available(jint fd, jlong *pbytes))
  //%note jvm_r6
  return os::available(fd, pbytes);
JVM_END


JVM_LEAF(jlong, JVM_Lseek(jint fd, jlong offset, jint whence))
  //%note jvm_r6
  return os::lseek(fd, offset, whence);
JVM_END


JVM_LEAF(jint, JVM_SetLength(jint fd, jlong length))
  return os::ftruncate(fd, length);
JVM_END


JVM_LEAF(jint, JVM_Sync(jint fd))
  //%note jvm_r6
  return os::fsync(fd);
JVM_END

// fixes /home/luchsh/proj/cvm/build/jdk8/bin/java: symbol lookup error: /home/luchsh/proj/cvm/build/jdk8/jre/lib/amd64/libnet.so: undefined symbol: JVM_InitializeSocketLibrary, version SUNWprivate_1.1
JVM_LEAF(jint, JVM_InitializeSocketLibrary())
  return 0;
JVM_END

JVM_LEAF(jint, JVM_Socket(jint domain, jint type, jint protocol))
  return os::socket(domain, type, protocol);
JVM_END


JVM_LEAF(jint, JVM_SocketClose(jint fd))
  //%note jvm_r6
  return os::socket_close(fd);
JVM_END


JVM_LEAF(jint, JVM_SocketShutdown(jint fd, jint howto))
  //%note jvm_r6
  //return os::socket_shutdown(fd, howto);
  return 0;
JVM_END


JVM_LEAF(jint, JVM_Recv(jint fd, char *buf, jint nBytes, jint flags))
  //%note jvm_r6
  return os::recv(fd, buf, (size_t)nBytes, (uint)flags);
JVM_END


JVM_LEAF(jint, JVM_Send(jint fd, char *buf, jint nBytes, jint flags))
  //%note jvm_r6
  return os::send(fd, buf, (size_t)nBytes, (uint)flags);
JVM_END


JVM_LEAF(jint, JVM_Timeout(int fd, long timeout))
  //%note jvm_r6
  return os::timeout(fd, timeout);
JVM_END


JVM_LEAF(jint, JVM_Listen(jint fd, jint count))
  //%note jvm_r6
  return os::listen(fd, count);
JVM_END


JVM_LEAF(jint, JVM_Connect(jint fd, struct sockaddr *him, jint len))
  //%note jvm_r6
  return os::connect(fd, him, (socklen_t)len);
JVM_END


JVM_LEAF(jint, JVM_Bind(jint fd, struct sockaddr *him, jint len))
  //%note jvm_r6
  return os::bind(fd, him, (socklen_t)len);
JVM_END

JVM_LEAF(jint, JVM_Accept(jint fd, struct sockaddr *him, jint *len))
  //%note jvm_r6
  socklen_t socklen = (socklen_t)(*len);
  jint result = os::accept(fd, him, &socklen);
  *len = (jint)socklen;
  return result;
JVM_END


JVM_LEAF(jint, JVM_RecvFrom(jint fd, char *buf, int nBytes, int flags, struct sockaddr *from, int *fromlen))
  //%note jvm_r6
  socklen_t socklen = (socklen_t)(*fromlen);
  jint result = os::recvfrom(fd, buf, (size_t)nBytes, (uint)flags, from, &socklen);
  *fromlen = (int)socklen;
  return result;
JVM_END


JVM_LEAF(jint, JVM_GetSockName(jint fd, struct sockaddr *him, int *len))
  //%note jvm_r6
  socklen_t socklen = (socklen_t)(*len);
  jint result = os::get_sock_name(fd, him, &socklen);
  *len = (int)socklen;
  return result;
JVM_END


JVM_LEAF(jint, JVM_SendTo(jint fd, char *buf, int len, int flags, struct sockaddr *to, int tolen))
  //%note jvm_r6
  return os::sendto(fd, buf, (size_t)len, (uint)flags, to, (socklen_t)tolen);
JVM_END


JVM_LEAF(jint, JVM_SocketAvailable(jint fd, jint *pbytes))
  //%note jvm_r6
  return os::socket_available(fd, pbytes);
JVM_END


JVM_LEAF(jint, JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen))
  //%note jvm_r6
  socklen_t socklen = (socklen_t)(*optlen);
  jint result = os::get_sock_opt(fd, level, optname, optval, &socklen);
  *optlen = (int)socklen;
  return result;
JVM_END


JVM_LEAF(jint, JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen))
  //%note jvm_r6
  return os::set_sock_opt(fd, level, optname, optval, (socklen_t)optlen);
JVM_END


JVM_LEAF(int, JVM_GetHostName(char* name, int namelen))
  return os::get_host_name(name, namelen);
JVM_END

/////////////////// Object allocation ///////////////////
bool force_verify_field_access(Klass* current_class, Klass* field_class, AccessFlags access, bool classloader_only) {
  if (current_class == NULL) {
    return true;
  }
  if ((current_class == field_class) || access.is_public()) {
    return true;
  }

  if (access.is_protected()) {
    // See if current_class is a subclass of field_class
    if (current_class->is_subclass_of(field_class)) {
      return true;
    }
  }

  return (!access.is_private() && InstanceKlass::cast(current_class)->is_same_class_package(field_class));
}

JVM_ENTRY(jobject, JVM_AllocateNewObject(JNIEnv *env, jobject receiver, jclass currClass, jclass initClass))
  JvmtiVMObjectAllocEventCollector oam;
  // Receiver is not used
  oop curr_mirror = JNIHandles::resolve_non_null(currClass);
  oop init_mirror = JNIHandles::resolve_non_null(initClass);

  // Cannot instantiate primitive types
  if (java_lang_Class::is_primitive(curr_mirror) || java_lang_Class::is_primitive(init_mirror)) {
    ResourceMark rm(THREAD);
    THROW_0(vmSymbols::java_lang_InvalidClassException());
  }

  // Arrays not allowed here, must use JVM_AllocateNewArray
  if (java_lang_Class::as_Klass(curr_mirror)->is_array_klass() ||
      java_lang_Class::as_Klass(init_mirror)->is_array_klass()) {
    ResourceMark rm(THREAD);
    THROW_0(vmSymbols::java_lang_InvalidClassException());
  }

  InstanceKlass* curr_klass = InstanceKlass::cast(java_lang_Class::as_Klass(curr_mirror));
  InstanceKlass* init_klass = InstanceKlass::cast(java_lang_Class::as_Klass(init_mirror));

  assert(curr_klass->is_subclass_of(init_klass), "just checking");

  // Interfaces, abstract classes, and java.lang.Class classes cannot be instantiated directly.
  curr_klass->check_valid_for_instantiation(false, CHECK_NULL);

  // Make sure klass is initialized, since we are about to instantiate one of them.
  curr_klass->initialize(CHECK_NULL);

 methodHandle m (THREAD,
                 init_klass->find_method(vmSymbols::object_initializer_name(),
                                         vmSymbols::void_method_signature()));
  if (m.is_null()) {
    ResourceMark rm(THREAD);
    THROW_MSG_0(vmSymbols::java_lang_NoSuchMethodError(),
                Method::name_and_sig_as_C_string(init_klass,
                                          vmSymbols::object_initializer_name(),
                                          vmSymbols::void_method_signature()));
  }

  if (curr_klass == init_klass && !m->is_public()) {
    // Calling the constructor for class 'curr_klass'.
    // Only allow calls to a public no-arg constructor.
    // This path corresponds to creating an Externalizable object.
    THROW_0(vmSymbols::java_lang_IllegalAccessException());
  }

  if (!force_verify_field_access(curr_klass, init_klass, m->access_flags(), false)) {
    // subclass 'curr_klass' does not have access to no-arg constructor of 'initcb'
    THROW_0(vmSymbols::java_lang_IllegalAccessException());
  }

  Handle obj = JavaCalls::construct_new_instance(curr_klass, vmSymbols::void_method_signature(), THREAD);
  return JNIHandles::make_local(THREAD, obj());
JVM_END


JVM_ENTRY(jobject, JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass, jint length))
  JvmtiVMObjectAllocEventCollector oam;
  oop mirror = JNIHandles::resolve_non_null(currClass);

  if (java_lang_Class::is_primitive(mirror)) {
    THROW_0(vmSymbols::java_lang_InvalidClassException());
  }
  Klass* k = java_lang_Class::as_Klass(mirror);
  oop result;

  if (k->is_typeArray_klass()) {
    // typeArray
    result = TypeArrayKlass::cast(k)->allocate(length, CHECK_NULL);
  } else if (k->is_objArray_klass()) {
    // objArray
    ObjArrayKlass* oak = ObjArrayKlass::cast(k);
    oak->initialize(CHECK_NULL); // make sure class is initialized (matches Classic VM behavior)
    result = oak->allocate(length, CHECK_NULL);
  } else {
    THROW_0(vmSymbols::java_lang_InvalidClassException());
  }
  return JNIHandles::make_local(THREAD, result);
JVM_END

JVM_ENTRY(jint, JVM_ClassDepth(JNIEnv *env, jstring name))
  ResourceMark rm(THREAD);
  Handle h_name (THREAD, JNIHandles::resolve_non_null(name));
  Handle class_name_str = java_lang_String::internalize_classname(h_name, CHECK_0);

  const char* str = java_lang_String::as_utf8_string(class_name_str());
  TempNewSymbol class_name_sym = SymbolTable::probe(str, (int)strlen(str));
  if (class_name_sym == NULL) {
    return -1;
  }

  int depth = 0;

  for(vframeStream vfst(thread); !vfst.at_end(); vfst.next()) {
    if (!vfst.method()->is_native()) {
      InstanceKlass* holder = vfst.method()->method_holder();
      assert(holder->is_klass(), "just checking");
      if (holder->name() == class_name_sym) {
        return depth;
      }
      depth++;
    }
  }
  return -1;
JVM_END

static bool is_trusted_frame(JavaThread* jthread, vframeStream* vfst) {
  assert(jthread->is_Java_thread(), "must be a Java thread");
  /*
  if (jthread->privileged_stack_top() == NULL) return false;
  if (jthread->privileged_stack_top()->frame_id() == vfst->frame_id()) {
    oop loader = jthread->privileged_stack_top()->class_loader();
    if (loader == NULL) return true;
    bool trusted = java_lang_ClassLoader::is_trusted_loader(loader);
    if (trusted) return true;
  }
  */
  return false;
}

JVM_ENTRY(jint, JVM_ClassLoaderDepth(JNIEnv *env))
  ResourceMark rm(THREAD);
  int depth = 0;
  for (vframeStream vfst(thread); !vfst.at_end(); vfst.next()) {
    // if a method in a class in a trusted loader is in a doPrivileged, return -1
    bool trusted = is_trusted_frame(thread, &vfst);
    if (trusted) return -1;

    Method* m = vfst.method();
    if (!m->is_native()) {
      InstanceKlass* holder = m->method_holder();
      assert(holder->is_klass(), "just checking");
      oop loader = holder->class_loader();
      if (loader != NULL && !java_lang_ClassLoader::is_trusted_loader(loader)) {
        return depth;
      }
      depth++;
    }
  }
  return -1;
JVM_END

JVM_ENTRY(jobject, JVM_CurrentClassLoader(JNIEnv *env))
  ResourceMark rm(THREAD);

  for (vframeStream vfst(thread); !vfst.at_end(); vfst.next()) {

    // if a method in a class in a trusted loader is in a doPrivileged, return NULL
    bool trusted = is_trusted_frame(thread, &vfst);
    if (trusted) return NULL;

    Method* m = vfst.method();
    if (!m->is_native()) {
      InstanceKlass* holder = m->method_holder();
      assert(holder->is_klass(), "just checking");
      oop loader = holder->class_loader();
      if (loader != NULL && !java_lang_ClassLoader::is_trusted_loader(loader)) {
        return JNIHandles::make_local(THREAD, loader);
      }
    }
  }
  return NULL;
JVM_END

JVM_ENTRY(jclass, JVM_CurrentLoadedClass(JNIEnv *env))
  ResourceMark rm(THREAD);
  for (vframeStream vfst(thread); !vfst.at_end(); vfst.next()) {
    // if a method in a class in a trusted loader is in a doPrivileged, return NULL
    bool trusted = is_trusted_frame(thread, &vfst);
    if (trusted) return NULL;

    Method* m = vfst.method();
    if (!m->is_native()) {
      InstanceKlass* holder = m->method_holder();
      oop loader = holder->class_loader();
      if (loader != NULL && !java_lang_ClassLoader::is_trusted_loader(loader)) {
        return (jclass) JNIHandles::make_local(THREAD, holder->java_mirror());
      }
    }
  }
  return NULL;
JVM_END

// Floating point support ////////////////////////////////////////////////////////////////////

JVM_LEAF(jboolean, JVM_IsNaN(jdouble a))
  return g_isnan(a);
JVM_END

static bool jvm_get_field_common(jobject field, fieldDescriptor& fd, TRAPS) {
  // some of this code was adapted from from jni_FromReflectedField

  oop reflected = JNIHandles::resolve_non_null(field);
  oop mirror    = java_lang_reflect_Field::clazz(reflected);
  Klass* k    = java_lang_Class::as_Klass(mirror);
  int slot      = java_lang_reflect_Field::slot(reflected);
  int modifiers = java_lang_reflect_Field::modifiers(reflected);

  intptr_t offset = InstanceKlass::cast(k)->field_offset(slot);

  if (modifiers & JVM_ACC_STATIC) {
    // for static fields we only look in the current class
    if (!InstanceKlass::cast(k)->find_local_field_from_offset(offset, true, &fd)) {
      assert(false, "cannot find static field");
      return false;
    }
  } else {
    // for instance fields we start with the current class and work
    // our way up through the superclass chain
    if (!InstanceKlass::cast(k)->find_field_from_offset(offset, false, &fd)) {
      assert(false, "cannot find instance field");
      return false;
    }
  }
  return true;
}

JVM_ENTRY(jbyteArray, JVM_GetFieldAnnotations(JNIEnv *env, jobject field))
  // field is a handle to a java.lang.reflect.Field object
  assert(field != NULL, "illegal field");

  fieldDescriptor fd;
  bool gotFd = jvm_get_field_common(field, fd, CHECK_NULL);
  if (!gotFd) {
    return NULL;
  }

  return (jbyteArray) JNIHandles::make_local(THREAD, Annotations::make_java_array(fd.annotations(), THREAD));
JVM_END

static Method* jvm_get_method_common(jobject method) {
  // some of this code was adapted from from jni_FromReflectedMethod

  oop reflected = JNIHandles::resolve_non_null(method);
  oop mirror    = NULL;
  int slot      = 0;

  if (reflected->klass() == vmClasses::reflect_Constructor_klass()) {
    mirror = java_lang_reflect_Constructor::clazz(reflected);
    slot   = java_lang_reflect_Constructor::slot(reflected);
  } else {
    assert(reflected->klass() == vmClasses::reflect_Method_klass(),
           "wrong type");
    mirror = java_lang_reflect_Method::clazz(reflected);
    slot   = java_lang_reflect_Method::slot(reflected);
  }
  Klass* k = java_lang_Class::as_Klass(mirror);

  Method* m = InstanceKlass::cast(k)->method_with_idnum(slot);
  assert(m != NULL, "cannot find method");
  return m;  // caller has to deal with NULL in product mode
}


JVM_ENTRY(jbyteArray, JVM_GetMethodAnnotations(JNIEnv *env, jobject method))
  // method is a handle to a java.lang.reflect.Method object
  Method* m = jvm_get_method_common(method);
  if (m == NULL) {
    return NULL;
  }

  return (jbyteArray) JNIHandles::make_local(THREAD,
    Annotations::make_java_array(m->annotations(), THREAD));
JVM_END

JVM_ENTRY(jbyteArray, JVM_GetMethodDefaultAnnotationValue(JNIEnv *env, jobject method))
  // method is a handle to a java.lang.reflect.Method object
  Method* m = jvm_get_method_common(method);
  if (m == NULL) {
    return NULL;
  }

  return (jbyteArray) JNIHandles::make_local(THREAD,
    Annotations::make_java_array(m->annotation_default(), THREAD));
JVM_END


JVM_ENTRY(jbyteArray, JVM_GetMethodParameterAnnotations(JNIEnv *env, jobject method))
  // method is a handle to a java.lang.reflect.Method object
  Method* m = jvm_get_method_common(method);
  if (m == NULL) {
    return NULL;
  }

  return (jbyteArray) JNIHandles::make_local(THREAD,
    Annotations::make_java_array(m->parameter_annotations(), THREAD));
JVM_END

JVM_ENTRY(jintArray, JVM_GetThreadStateValues(JNIEnv* env,
                                              jint javaThreadState))
  return NULL;
JVM_END


JVM_ENTRY(jobjectArray, JVM_GetThreadStateNames(JNIEnv* env,
                                                jint javaThreadState,
                                                jintArray values))
  return NULL;
JVM_END

JVM_ENTRY(jclass, JVM_LoadClass0(JNIEnv *env, jobject receiver,
                                 jclass currClass, jstring currClassName))
  // Receiver is not used
  ResourceMark rm(THREAD);

  // Class name argument is not guaranteed to be in internal format
  Handle classname (THREAD, JNIHandles::resolve_non_null(currClassName));
  Handle string = java_lang_String::internalize_classname(classname, CHECK_NULL);

  const char* str = java_lang_String::as_utf8_string(string());

  if (str == NULL || (int)strlen(str) > Symbol::max_length()) {
    // It's impossible to create this class;  the name cannot fit
    // into the constant pool.
    THROW_MSG_0(vmSymbols::java_lang_NoClassDefFoundError(), str);
  }

  TempNewSymbol name = SymbolTable::new_symbol(str);
  Handle curr_klass (THREAD, JNIHandles::resolve(currClass));
  // Find the most recent class on the stack with a non-null classloader
  oop loader = NULL;
  oop protection_domain = NULL;
  if (curr_klass.is_null()) {
    for (vframeStream vfst(thread);
         !vfst.at_end() && loader == NULL;
         vfst.next()) {
      if (!vfst.method()->is_native()) {
        InstanceKlass* holder = vfst.method()->method_holder();
        loader             = holder->class_loader();
        protection_domain  = holder->protection_domain();
      }
    }
  } else {
    Klass* curr_klass_oop = java_lang_Class::as_Klass(curr_klass());
    loader            = InstanceKlass::cast(curr_klass_oop)->class_loader();
    protection_domain = InstanceKlass::cast(curr_klass_oop)->protection_domain();
  }
  Handle h_loader(THREAD, loader);
  Handle h_prot  (THREAD, protection_domain);
  jclass result =  find_class_from_class_loader(env, name, true, h_loader, h_prot,
                                                false, thread);
  if (result != NULL) {
    trace_class_resolution(java_lang_Class::as_Klass(JNIHandles::resolve_non_null(result)));
  }
  return result;
JVM_END

// just run without checking!
JVM_ENTRY(jobject, JVM_DoPrivileged(JNIEnv *env, jclass cls, jobject action, jobject context, jboolean wrapException))
  if (action == NULL) {
    THROW_MSG_0(vmSymbols::java_lang_NullPointerException(), "Null action");
  }

  // Compute the frame initiating the do privileged operation and setup the privileged stack
  //vframeStream vfst(thread);
  //vfst.security_get_caller_frame(1);

  //if (vfst.at_end()) {
  //  THROW_MSG_0(vmSymbols::java_lang_InternalError(), "no caller?");
  //}

  //Method* method        = vfst.method();
  //Klass* klass = method->method_holder();

  // Check that action object understands "Object run()"
  /*
  Handle h_context;
  if (context != NULL) {
    h_context = Handle(THREAD, JNIHandles::resolve(context));
    bool authorized = is_authorized(h_context, klass, CHECK_NULL);
    if (!authorized) {
      // Create an unprivileged access control object and call it's run function
      // instead.
      oop noprivs = create_dummy_access_control_context(CHECK_NULL);
      h_context = Handle(THREAD, noprivs);
    }
  }
  */

  // Check that action object understands "Object run()"
  Handle object (THREAD, JNIHandles::resolve(action));

  // get run() method
  Method* m_oop = object->klass()->uncached_lookup_method(
                                           vmSymbols::run_method_name(),
                                           vmSymbols::void_object_signature(),
                                           Klass::OverpassLookupMode::find);
  methodHandle m (THREAD, m_oop);
  if (m.is_null() || !m->is_method() || !m()->is_public() || m()->is_static()) {
    THROW_MSG_0(vmSymbols::java_lang_InternalError(), "No run method");
  }

  // Stack allocated list of privileged stack elements
  //PrivilegedElement pi;
  //if (!vfst.at_end()) {
  //  pi.initialize(&vfst, h_context(), thread->privileged_stack_top(), CHECK_NULL);
  //  thread->set_privileged_stack_top(&pi);
  //}


  // invoke the Object run() in the action object. We cannot use call_interface here, since the static type
  // is not really known - it is either java.security.PrivilegedAction or java.security.PrivilegedExceptionAction
  Handle pending_exception;
  JavaValue result(T_OBJECT);
  JavaCallArguments args(object);
  JavaCalls::call(&result, m, &args, THREAD);

  // done with action, remove ourselves from the list
  //if (!vfst.at_end()) {
  //  assert(thread->privileged_stack_top() != NULL && thread->privileged_stack_top() == &pi, "wrong top element");
  //  thread->set_privileged_stack_top(thread->privileged_stack_top()->next());
  //}

  if (HAS_PENDING_EXCEPTION) {
    pending_exception = Handle(THREAD, PENDING_EXCEPTION);
    CLEAR_PENDING_EXCEPTION;
    // JVMTI has already reported the pending exception
    // JVMTI internal flag reset is needed in order to report PrivilegedActionException
    if (THREAD->is_Java_thread()) {
      JvmtiExport::clear_detected_exception((JavaThread*) THREAD);
    }
    if ( pending_exception->is_a(vmClasses::Exception_klass()) &&
        !pending_exception->is_a(vmClasses::RuntimeException_klass())) {
      // Throw a java.security.PrivilegedActionException(Exception e) exception
      JavaCallArguments args(pending_exception);
      THROW_ARG_0(vmSymbols::java_security_PrivilegedActionException(),
                  vmSymbols::exception_void_signature(),
                  &args);
    }
  }

  if (pending_exception.not_null()) THROW_OOP_0(pending_exception());
  return JNIHandles::make_local(THREAD, result.get_oop());
JVM_END

JVM_LEAF(jint, JVM_GetInterfaceVersion())
  return JVM_INTERFACE_VERSION;
JVM_END

JVM_ENTRY(void, JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size))
  memset(info, 0, info_size);

  info->jvm_version = Abstract_VM_Version::jvm_version_for_jdk8();
  info->update_version = 0;          /* 0 in HotSpot Express VM */
  info->special_update_version = 0;  /* 0 in HotSpot Express VM */

  // when we add a new capability in the jvm_version_info struct, we should also
  // consider to expose this new capability in the sun.rt.jvmCapabilities jvmstat
  // counter defined in runtimeService.cpp.
  info->is_attachable = AttachListener::is_attach_supported();
JVM_END

static void set_property(Handle props, const char* key, const char* value, TRAPS) {
  JavaValue r(T_OBJECT);
  // public synchronized Object put(Object key, Object value);
  HandleMark hm(THREAD);
  Handle key_str    = java_lang_String::create_from_platform_dependent_str(key, CHECK);
  Handle value_str  = java_lang_String::create_from_platform_dependent_str((value != NULL ? value : ""), CHECK);
  JavaCalls::call_virtual(&r,
                          props,
                          vmClasses::Properties_klass(),
                          vmSymbols::put_name(),
                          vmSymbols::object_object_object_signature(),
                          key_str,
                          value_str,
                          THREAD);
}

#ifndef PUTPROP
#define PUTPROP(props, name, value) set_property((props), (name), (value), CHECK_(properties));
#endif

JVM_ENTRY(jobject, JVM_InitProperties(JNIEnv *env, jobject properties))
  ResourceMark rm;

  Handle props(THREAD, JNIHandles::resolve_non_null(properties));

  // System property list includes both user set via -D option and
  // jvm system specific properties.
  for (SystemProperty* p = Arguments::system_properties(); p != NULL; p = p->next()) {
    PUTPROP(props, p->key(), p->value());
  }

  // Convert the -XX:MaxDirectMemorySize= command line flag
  // to the sun.nio.MaxDirectMemorySize property.
  // Do this after setting user properties to prevent people
  // from setting the value with a -D option, as requested.
  {
    if (FLAG_IS_DEFAULT(MaxDirectMemorySize)) {
      PUTPROP(props, "sun.nio.MaxDirectMemorySize", "-1");
    } else {
      char as_chars[256];
      jio_snprintf(as_chars, sizeof(as_chars), UINTX_FORMAT, MaxDirectMemorySize);
      PUTPROP(props, "sun.nio.MaxDirectMemorySize", as_chars);
    }
  }

  // JVM monitoring and management support
  // Add the sun.management.compiler property for the compiler's name
  {
#undef CSIZE
#if defined(_LP64) || defined(_WIN64)
  #define CSIZE "64-Bit "
#else
  #define CSIZE
#endif // 64bit

#ifdef TIERED
    const char* compiler_name = "HotSpot " CSIZE "Tiered Compilers";
#else
#if defined(COMPILER1)
    const char* compiler_name = "HotSpot " CSIZE "Client Compiler";
#elif defined(COMPILER2)
    const char* compiler_name = "HotSpot " CSIZE "Server Compiler";
#else
    const char* compiler_name = "";
#endif // compilers
#endif // TIERED

    if (*compiler_name != '\0' &&
        (Arguments::mode() != Arguments::_int)) {
      PUTPROP(props, "sun.management.compiler", compiler_name);
    }
  }

  const char* enableSharedLookupCache = "false";
//#if INCLUDE_CDS
//  if (ClassLoaderExt::is_lookup_cache_enabled()) {
//    enableSharedLookupCache = "true";
//  }
//#endif
  PUTPROP(props, "sun.cds.enableSharedLookupCache", enableSharedLookupCache);

  return properties;
JVM_END

//#ifdef UNDEF_PUTPROP
//#undef UNDEF_PUTPROP
//#undef PUTPROP
//#endif

#endif // if defined(HOTSPOT_TARGET_CLASSLIB) && HOTSPOT_TARGET_CLASSLIB != 8
