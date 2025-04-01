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

#ifndef _JAVASOFT_JVM8_H_
#define _JAVASOFT_JVM8_H_

#include "jni.h"

#ifndef JVM_INTERFACE_VERSION
#define JVM_INTERFACE_VERSION 4
#endif // JVM_INTERFACE_VERSION

#if !defined(HOTSPOT_TARGET_CLASSLIB) || HOTSPOT_TARGET_CLASSLIB != 8
#error("Only works with -DHOTSPOT_TARGET_CLASSLIB=8")
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * The following defines a private JVM interface that the JDK can query
 * for the JVM version and capabilities.  sun.misc.Version defines
 * the methods for getting the VM version and its capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JVM_GetVersionInfo and Abstract_VM_Version class
 *    SDK:  Version class
 *
 * Similary, a private JDK interface JDK_GetVersionInfo0 is defined for
 * JVM to query for the JDK version and capabilities.
 *
 * When a new bit is added, the following should be updated to provide
 * access to the new capability:
 *    HS:   JDK_Version class
 *    SDK:  JDK_GetVersionInfo0
 *
 * ==========================================================================
 */
typedef struct {
    /* HotSpot Express VM version string:
     * <major>.<minor>-bxx[-<identifier>][-<debug_flavor>]
     */
    unsigned int jvm_version; /* Consists of major.minor.0.build */
    unsigned int update_version : 8;         /* 0 in HotSpot Express VM */
    unsigned int special_update_version : 8; /* 0 in HotSpot Express VM */
    unsigned int reserved1 : 16;
    unsigned int reserved2;

    /* The following bits represents JVM supports that JDK has dependency on.
     * JDK can use these bits to determine which JVM version
     * and support it has to maintain runtime compatibility.
     *
     * When a new bit is added in a minor or update release, make sure
     * the new bit is also added in the main/baseline.
     */
    unsigned int is_attachable : 1;
    unsigned int : 31;
    unsigned int : 32;
    unsigned int : 32;
} jvm_version_info;

JNIEXPORT void JNICALL
JVM_GetVersionInfo(JNIEnv* env, jvm_version_info* info, size_t info_size);


JNIEXPORT jobjectArray JNICALL
JVM_GetMethodParameters(JNIEnv *env, jobject method);

/*
 * java.io.File
 */
JNIEXPORT void JNICALL
JVM_OnExit(void (*func)(void));

/*
 * java.nio.Bits
 */
JNIEXPORT void JNICALL
JVM_CopySwapMemory(JNIEnv *env, jobject srcObj, jlong srcOffset,
                   jobject dstObj, jlong dstOffset, jlong size,
                   jlong elemSize);

JNIEXPORT void JNICALL
JVM_TraceInstructions(jboolean on);

JNIEXPORT void JNICALL
JVM_TraceMethodCalls(jboolean on);

JNIEXPORT void * JNICALL
JVM_LoadLibrary8(const char *name);

/*
 * java.lang.Float and java.lang.Double
 */
JNIEXPORT jboolean JNICALL
JVM_IsNaN(jdouble d);

JNIEXPORT jint JNICALL
JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable);

JNIEXPORT jobject JNICALL
JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index);

JNIEXPORT void JNICALL
JVM_InitializeCompiler (JNIEnv *env, jclass compCls);

JNIEXPORT jboolean JNICALL
JVM_IsSilentCompiler(JNIEnv *env, jclass compCls);

JNIEXPORT jboolean JNICALL
JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls);

JNIEXPORT jboolean JNICALL
JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname);

JNIEXPORT jobject JNICALL
JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg);

JNIEXPORT void JNICALL
JVM_EnableCompiler(JNIEnv *env, jclass compCls);

JNIEXPORT void JNICALL
JVM_DisableCompiler(JNIEnv *env, jclass compCls);

/*
 * java.lang.SecurityManager
 */
JNIEXPORT jclass JNICALL
JVM_CurrentLoadedClass(JNIEnv *env);

JNIEXPORT jobject JNICALL
JVM_CurrentClassLoader(JNIEnv *env);

JNIEXPORT jint JNICALL
JVM_ClassDepth(JNIEnv *env, jstring name);

JNIEXPORT jint JNICALL
JVM_ClassLoaderDepth(JNIEnv *env);

/*
 * java.io.ObjectInputStream
 */
JNIEXPORT jobject JNICALL
JVM_AllocateNewObject(JNIEnv *env, jobject obj, jclass currClass,
                      jclass initClass);

JNIEXPORT jobject JNICALL
JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass,
                     jint length);

JNIEXPORT jobject JNICALL
JVM_LatestUserDefinedLoader(JNIEnv *env);

/*
 * This function has been deprecated and should not be considered
 * part of the specified JVM interface.
 */
JNIEXPORT jclass JNICALL
JVM_LoadClass0(JNIEnv *env, jobject obj, jclass currClass,
               jstring currClassName);

/*
 * java.lang.Class and java.lang.ClassLoader
 */

#define JVM_CALLER_DEPTH -1

JNIEXPORT jclass JNICALL
JVM_GetCallerClass8(JNIEnv *env, int n);

/*
 * Link the class
 */
JNIEXPORT void JNICALL
JVM_ResolveClass(JNIEnv *env, jclass cls);

/*
 * Find a class from a given class loader. Throw ClassNotFoundException
 * or NoClassDefFoundError depending on the value of the last
 * argument.
 */
JNIEXPORT jclass JNICALL
JVM_FindClassFromClassLoader(JNIEnv *env, const char *name, jboolean init,
                             jobject loader, jboolean throwError);

JNIEXPORT jclass JNICALL
JVM_DefineClassWithSourceCond(JNIEnv *env, const char *name,
                              jobject loader, const jbyte *buf,
                              jsize len, jobject pd, const char *source,
                              jboolean verify);

JNIEXPORT jstring JNICALL
JVM_GetClassName(JNIEnv *env, jclass cls);

JNIEXPORT jobject JNICALL
JVM_GetClassLoader(JNIEnv *env, jclass cls);

JNIEXPORT jclass JNICALL
JVM_GetComponentType(JNIEnv *env, jclass cls);

/* Annotations support (JDK 1.6) */

// field is a handle to a java.lang.reflect.Field object
JNIEXPORT jbyteArray JNICALL
VM_GetFieldAnnotations(JNIEnv *env, jobject field);

// method is a handle to a java.lang.reflect.Method object
JNIEXPORT jbyteArray JNICALL
JVM_GetMethodAnnotations(JNIEnv *env, jobject method);

// method is a handle to a java.lang.reflect.Method object
JNIEXPORT jbyteArray JNICALL
JVM_GetMethodDefaultAnnotationValue(JNIEnv *env, jobject method);

// method is a handle to a java.lang.reflect.Method object
JNIEXPORT jbyteArray JNICALL
JVM_GetMethodParameterAnnotations(JNIEnv *env, jobject method);

JNIEXPORT jboolean JNICALL
JVM_CX8Field(JNIEnv *env, jobject obj, jfieldID fldID, jlong oldVal, jlong newVal);

/* Note that the JVM IO functions are expected to return JVM_IO_ERR
 * when there is any kind of error. The caller can then use the
 * platform specific support (e.g., errno) to get the detailed
 * error info.  The JVM_GetLastErrorString procedure may also be used
 * to obtain a descriptive error string.
 */
#define JVM_IO_ERR  (-1)

/* For interruptible IO. Returning JVM_IO_INTR indicates that an IO
 * operation has been disrupted by Thread.interrupt. There are a
 * number of technical difficulties related to interruptible IO that
 * need to be solved. For example, most existing programs do not handle
 * InterruptedIOExceptions specially, they simply treat those as any
 * IOExceptions, which typically indicate fatal errors.
 *
 * There are also two modes of operation for interruptible IO. In the
 * resumption mode, an interrupted IO operation is guaranteed not to
 * have any side-effects, and can be restarted. In the termination mode,
 * an interrupted IO operation corrupts the underlying IO stream, so
 * that the only reasonable operation on an interrupted stream is to
 * close that stream. The resumption mode seems to be impossible to
 * implement on Win32 and Solaris. Implementing the termination mode is
 * easier, but it's not clear that's the right semantics.
 *
 * Interruptible IO is not supported on Win32.It can be enabled/disabled
 * using a compile-time flag on Solaris. Third-party JVM ports do not
 * need to implement interruptible IO.
 */
#define JVM_IO_INTR (-2)

/* Write a string into the given buffer, in the platform's local encoding,
 * that describes the most recent system-level error to occur in this thread.
 * Return the length of the string or zero if no error occurred.
 */
JNIEXPORT jint JNICALL
JVM_GetLastErrorString(char *buf, int len);

/*
 * JVM I/O error codes
 */
#define JVM_EEXIST       -100

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, and a non-negative integer that is the file descriptor on
 * success.
 */
JNIEXPORT jint JNICALL
JVM_Open(const char *fname, jint flags, jint mode);

/*
 * Close a file descriptor. This function returns -1 on error, and 0
 * on success.
 *
 * fd        the file descriptor to close.
 */
JNIEXPORT jint JNICALL
JVM_Close(jint fd);

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns -1 on error, and 0 on success.
 */
JNIEXPORT jint JNICALL
JVM_Read(jint fd, char *buf, jint nbytes);

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 * This function returns -1 on error, and 0 on success.
 */
JNIEXPORT jint JNICALL
JVM_Write(jint fd, char *buf, jint nbytes);

/*
 * Returns the number of bytes available for reading from a given file
 * descriptor
 */
JNIEXPORT jint JNICALL
JVM_Available(jint fd, jlong *pbytes);

/*
 * Move the file descriptor pointer from whence by offset.
 *
 * fd        the file descriptor to move.
 * offset    the number of bytes to move it by.
 * whence    the start from where to move it.
 *
 * This function returns the resulting pointer location.
 */
JNIEXPORT jlong JNICALL
JVM_Lseek(jint fd, jlong offset, jint whence);

/*
 * Set the length of the file associated with the given descriptor to the given
 * length.  If the new length is longer than the current length then the file
 * is extended; the contents of the extended portion are not defined.  The
 * value of the file pointer is undefined after this procedure returns.
 */
JNIEXPORT jint JNICALL
JVM_SetLength(jint fd, jlong length);
/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK.
 */
JNIEXPORT jint JNICALL
JVM_Sync(jint fd);

/*
 * Networking library support
 */

JNIEXPORT jint JNICALL
JVM_InitializeSocketLibrary(void);

struct sockaddr;

JNIEXPORT jint JNICALL
JVM_Socket(jint domain, jint type, jint protocol);

JNIEXPORT jint JNICALL
JVM_SocketClose(jint fd);

JNIEXPORT jint JNICALL
JVM_SocketShutdown(jint fd, jint howto);

JNIEXPORT jint JNICALL
JVM_Recv(jint fd, char *buf, jint nBytes, jint flags);

JNIEXPORT jint JNICALL
JVM_Send(jint fd, char *buf, jint nBytes, jint flags);

JNIEXPORT jint JNICALL
JVM_Timeout(int fd, long timeout);

JNIEXPORT jint JNICALL
JVM_Listen(jint fd, jint count);

JNIEXPORT jint JNICALL
JVM_Connect(jint fd, struct sockaddr *him, jint len);

JNIEXPORT jint JNICALL
JVM_Bind(jint fd, struct sockaddr *him, jint len);

JNIEXPORT jint JNICALL
JVM_Accept(jint fd, struct sockaddr *him, jint *len);

JNIEXPORT jint JNICALL
JVM_RecvFrom(jint fd, char *buf, int nBytes,
                  int flags, struct sockaddr *from, int *fromlen);

JNIEXPORT jint JNICALL
JVM_SendTo(jint fd, char *buf, int len,
                int flags, struct sockaddr *to, int tolen);

JNIEXPORT jint JNICALL
JVM_SocketAvailable(jint fd, jint *result);


JNIEXPORT jint JNICALL
JVM_GetSockName(jint fd, struct sockaddr *him, int *len);

JNIEXPORT jint JNICALL
JVM_GetSockOpt(jint fd, int level, int optname, char *optval, int *optlen);

JNIEXPORT jint JNICALL
JVM_SetSockOpt(jint fd, int level, int optname, const char *optval, int optlen);

JNIEXPORT int JNICALL
JVM_GetHostName(char* name, int namelen);

/*
 * java.lang.reflect.Method
 */
JNIEXPORT jobject JNICALL
JVM_InvokeMethod(JNIEnv *env, jobject method, jobject obj, jobjectArray args0);

/*
 * java.lang.reflect.Constructor
 */
JNIEXPORT jobject JNICALL
JVM_NewInstanceFromConstructor(JNIEnv *env, jobject c, jobjectArray args0);

/*
 * Java thread state support
 */
enum {
    JAVA_THREAD_STATE_NEW           = 0,
    JAVA_THREAD_STATE_RUNNABLE      = 1,
    JAVA_THREAD_STATE_BLOCKED       = 2,
    JAVA_THREAD_STATE_WAITING       = 3,
    JAVA_THREAD_STATE_TIMED_WAITING = 4,
    JAVA_THREAD_STATE_TERMINATED    = 5,
    JAVA_THREAD_STATE_COUNT         = 6
};

/*
 * Returns an array of the threadStatus values representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or doesn't support the given
 * Java thread state.
 */
JNIEXPORT jintArray JNICALL
JVM_GetThreadStateValues(JNIEnv* env, jint javaThreadState);

/*
 * Returns an array of the substate names representing the
 * given Java thread state.  Returns NULL if the VM version is
 * incompatible with the JDK or the VM doesn't support
 * the given Java thread state.
 * values must be the jintArray returned from JVM_GetThreadStateValues
 * and javaThreadState.
 */
JNIEXPORT jobjectArray JNICALL
JVM_GetThreadStateNames(JNIEnv* env, jint javaThreadState, jintArray values);

/*
 * Returns true if the JVM's lookup cache indicates that this class is
 * known to NOT exist for the given loader.
 */
JNIEXPORT jboolean JNICALL
JVM_KnownToNotExist(JNIEnv *env, jobject loader, const char *classname);

/*
 * Returns an array of all URLs that are stored in the JVM's lookup cache
 * for the given loader. NULL if the lookup cache is unavailable.
 */
JNIEXPORT jobjectArray JNICALL
JVM_GetResourceLookupCacheURLs(JNIEnv *env, jobject loader);

/*
 * Returns an array of all URLs that *may* contain the resource_name for the
 * given loader. This function returns an integer array, each element
 * of which can be used to index into the array returned by
 * JVM_GetResourceLookupCacheURLs of the same loader to determine the
 * URLs.
 */
JNIEXPORT jintArray JNICALL
JVM_GetResourceLookupCache(JNIEnv *env, jobject loader, const char *resource_name);

// removed since jdk11
JNIEXPORT jint JNICALL
JVM_CountStackFrames(JNIEnv* env, jobject jthread);

JNIEXPORT jboolean JNICALL
JVM_IsInterrupted(JNIEnv* env, jobject jthread, jboolean clear_interrupted);

/*
 * java.io.ObjectInputStream
 */
JNIEXPORT jobject JNICALL
JVM_AllocateNewObject(JNIEnv *env, jobject obj, jclass currClass,
                      jclass initClass);

JNIEXPORT jobject JNICALL
JVM_AllocateNewArray(JNIEnv *env, jobject obj, jclass currClass,
                     jint length);


/*
 * java.lang.SecurityManager
 */
JNIEXPORT jclass JNICALL
JVM_CurrentLoadedClass(JNIEnv *env);

JNIEXPORT jint JNICALL
JVM_ClassDepth(JNIEnv *env, jstring name);

JNIEXPORT jint JNICALL
JVM_ClassLoaderDepth(JNIEnv *env);

JNIEXPORT jobject JNICALL
JVM_CurrentClassLoader(JNIEnv *env);

/*
 * java.lang.Float and java.lang.Double
 */
JNIEXPORT jboolean JNICALL
JVM_IsNaN(jdouble d);


/*
 * java.lang.System
 */
JNIEXPORT jobject JNICALL
JVM_InitProperties(JNIEnv *env, jobject p);

JNIEXPORT jbyteArray JNICALL
JVM_GetFieldAnnotations(JNIEnv *env, jobject field);

JNIEXPORT jint JNICALL
JVM_GetInterfaceVersion(void);

JNIEXPORT jobject JNICALL
JVM_DoPrivileged(JNIEnv *env, jclass cls,
                 jobject action, jobject context, jboolean wrapException);

#ifdef __cplusplus
}
#endif

#endif // _JAVASOFT_JVM8_H_
