#include "classfile/classLoader.hpp"
#include "classfile/systemDictionary.hpp"
#include "classfile/vmClasses.hpp"
#include "compiler/compileBroker.hpp"
#include "gc/shared/collectedHeap.hpp"
#include "jmm.h"
#include "memory/allocation.inline.hpp"
#include "memory/iterator.hpp"
#include "memory/oopFactory.hpp"
#include "memory/resourceArea.hpp"
#include "memory/universe.hpp"
#include "oops/klass.hpp"
#include "oops/klass.inline.hpp"
#include "oops/objArrayKlass.hpp"
#include "oops/objArrayOop.inline.hpp"
#include "oops/oop.inline.hpp"
#include "oops/oopHandle.inline.hpp"
#include "oops/typeArrayOop.inline.hpp"
#include "runtime/flags/jvmFlag.hpp"
#include "runtime/globals.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/javaCalls.hpp"
#include "runtime/jniHandles.inline.hpp"
#include "runtime/mutexLocker.hpp"
#include "runtime/notificationThread.hpp"
#include "runtime/os.hpp"
#include "runtime/thread.inline.hpp"
#include "runtime/threads.hpp"
#include "runtime/threadSMR.hpp"
#include "runtime/vmOperations.hpp"
#include "services/classLoadingService.hpp"
#include "services/diagnosticCommand.hpp"
#include "services/diagnosticFramework.hpp"
#include "services/finalizerService.hpp"
#include "services/writeableFlags.hpp"
#include "services/heapDumper.hpp"
#include "services/lowMemoryDetector.hpp"
#include "services/gcNotifier.hpp"
#include "services/management.hpp"
#include "services/memoryManager.hpp"
#include "services/memoryPool.hpp"
#include "services/memoryService.hpp"
#include "services/runtimeService.hpp"
#include "services/threadService.hpp"
#include "utilities/debug.hpp"
#include "utilities/formatBuffer.hpp"
#include "utilities/macros.hpp"

#if INCLUDE_MANAGEMENT
#if HOTSPOT_TARGET_CLASSLIB == 8

extern jobjectArray dump_threads_common(JNIEnv *env, jlongArray thread_ids, jboolean locked_monitors,
                                 jboolean locked_synchronizers, jint maxDepth, TRAPS);

// Dump thread info for the specified threads.
// It returns an array of ThreadInfo objects. Each element is the ThreadInfo
// for the thread ID specified in the corresponding entry in
// the given array of thread IDs; or NULL if the thread does not exist
// or has terminated.
//
// Input parameter:
//    ids - array of thread IDs; NULL indicates all live threads
//    locked_monitors - if true, dump locked object monitors
//    locked_synchronizers - if true, dump locked JSR-166 synchronizers
//
// This method exists only for compatbility with compiled binaries that call it.
// The JDK library uses jmm_DumpThreadsMaxDepth.
//
JVM_ENTRY(jobjectArray, jmm_DumpThreads_JDK8(JNIEnv *env, jlongArray thread_ids, jboolean locked_monitors,
                                             jboolean locked_synchronizers))
return dump_threads_common(env, thread_ids, locked_monitors, locked_synchronizers, INT_MAX, THREAD);
JVM_END

// Returns a java.lang.String object containing the input arguments to the VM.
JVM_ENTRY(jobject, jmm_GetInputArguments(JNIEnv *env))
ResourceMark rm(THREAD);

if (Arguments::num_jvm_args() == 0 && Arguments::num_jvm_flags() == 0) {
    return NULL;
}

char **vm_flags = Arguments::jvm_flags_array();
char **vm_args = Arguments::jvm_args_array();
int num_flags = Arguments::num_jvm_flags();
int num_args = Arguments::num_jvm_args();

size_t length = 1; // null terminator
int i;
for (i = 0; i < num_flags; i++) {
    length += strlen(vm_flags[i]);
}
for (i = 0; i < num_args; i++) {
    length += strlen(vm_args[i]);
}
// add a space between each argument
length += num_flags + num_args - 1;

// Return the list of input arguments passed to the VM
// and preserve the order that the VM processes.
char *args = NEW_RESOURCE_ARRAY(char, length);
args[0] = '\0';
// concatenate all jvm_flags
if (num_flags > 0) {
    strcat(args, vm_flags[0]);
    for (i = 1; i < num_flags; i++) {
        strcat(args, " ");
        strcat(args, vm_flags[i]);
    }
}

if (num_args > 0 && num_flags > 0) {
    // append a space if args already contains one or more jvm_flags
    strcat(args, " ");
}

// concatenate all jvm_args
if (num_args > 0) {
    strcat(args, vm_args[0]);
    for (i = 1; i < num_args; i++) {
        strcat(args, " ");
        strcat(args, vm_args[i]);
    }
}

Handle hargs = java_lang_String::create_from_platform_dependent_str(args, CHECK_NULL);
return JNIHandles::make_local(THREAD, hargs());
JVM_END

// Returns an array of java.lang.String object containing the input arguments to the VM.
JVM_ENTRY(jobjectArray, jmm_GetInputArgumentArray(JNIEnv *env))
ResourceMark rm(THREAD);

if (Arguments::num_jvm_args() == 0 && Arguments::num_jvm_flags() == 0) {
    return NULL;
}

char **vm_flags = Arguments::jvm_flags_array();
char **vm_args = Arguments::jvm_args_array();
int num_flags = Arguments::num_jvm_flags();
int num_args = Arguments::num_jvm_args();

InstanceKlass *ik = vmClasses::String_klass();
objArrayOop r = oopFactory::new_objArray(ik, num_args + num_flags, CHECK_NULL);
objArrayHandle result_h(THREAD, r);

int index = 0;
for (int j = 0; j < num_flags; j++, index++) {
    Handle h = java_lang_String::create_from_platform_dependent_str(vm_flags[j], CHECK_NULL);
    result_h->obj_at_put(index, h());
}
for (int i = 0; i < num_args; i++, index++) {
    Handle h = java_lang_String::create_from_platform_dependent_str(vm_args[i], CHECK_NULL);
    result_h->obj_at_put(index, h());
}
return (jobjectArray)JNIHandles::make_local(THREAD, result_h());
JVM_END

JVM_ENTRY(void, jmm_GetDiagnosticCommandArgumentsInfo_JDK8(JNIEnv *env,
          jstring command, dcmdArgInfo* infoArray))
  ResourceMark rm(THREAD);
  oop cmd = JNIHandles::resolve_external_guard(command);
  if (cmd == nullptr) {
    THROW_MSG(vmSymbols::java_lang_NullPointerException(),
              "Command line cannot be null.");
  }
  char* cmd_name = java_lang_String::as_utf8_string(cmd);
  if (cmd_name == nullptr) {
    THROW_MSG(vmSymbols::java_lang_NullPointerException(),
              "Command line content cannot be null.");
  }
  DCmd* dcmd = nullptr;
  DCmdFactory*factory = DCmdFactory::factory(DCmd_Source_MBean, cmd_name,
                                             strlen(cmd_name));
  if (factory != nullptr) {
    dcmd = factory->create_resource_instance(nullptr);
  }
  if (dcmd == nullptr) {
    THROW_MSG(vmSymbols::java_lang_IllegalArgumentException(),
              "Unknown diagnostic command");
  }
  DCmdMark mark(dcmd);
  GrowableArray<DCmdArgumentInfo*>* array = dcmd->argument_info_array();
  const int num_args = array->length();
  for (int i = 0; i < num_args; i++) {
    infoArray[i].name = array->at(i)->name();
    infoArray[i].description = array->at(i)->description();
    infoArray[i].type = array->at(i)->type();
    infoArray[i].default_string = array->at(i)->default_string();
    infoArray[i].mandatory = array->at(i)->is_mandatory();
    infoArray[i].option = array->at(i)->is_option();
    infoArray[i].multiple = array->at(i)->is_multiple();
    infoArray[i].position = array->at(i)->position();
  }
  return;
JVM_END

#endif
#endif