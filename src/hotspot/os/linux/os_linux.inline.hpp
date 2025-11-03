/*
 * Copyright (c) 1999, 2022, Oracle and/or its affiliates. All rights reserved.
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

#ifndef OS_LINUX_OS_LINUX_INLINE_HPP
#define OS_LINUX_OS_LINUX_INLINE_HPP

#include "os_linux.hpp"

#include "runtime/os.hpp"
#include "os_posix.inline.hpp"
#if HOTSPOT_TARGET_CLASSLIB == 8
#include <sys/poll.h>
#endif

inline bool os::zero_page_read_protected() {
  return true;
}

inline bool os::uses_stack_guard_pages() {
  return true;
}

inline bool os::must_commit_stack_guard_pages() {
  assert(uses_stack_guard_pages(), "sanity check");
  return true;
}

// Bang the shadow pages if they need to be touched to be mapped.
inline void os::map_stack_shadow_pages(address sp) {
}

// Trim-native support
inline bool os::can_trim_native_heap() {
#ifdef __GLIBC__
  return true;
#else
  return false; // musl
#endif
}

#if HOTSPOT_TARGET_CLASSLIB == 8
inline int os::unlink(const char *path) {
  return ::unlink(path);
}

inline int os::close(int fd) {
  return ::close(fd);
}

inline void* os::thread_local_storage_at(int index) {
  return pthread_getspecific((pthread_key_t)index);
}

// File names are case-sensitive on windows only
inline int os::file_name_strcmp(const char* s1, const char* s2) {
  return strcmp(s1, s2);
}

inline bool os::obsolete_option(const JavaVMOption *option) {
  return false;
}
inline bool os::allocate_stack_guard_pages() {
  assert(uses_stack_guard_pages(), "sanity check");
  return true;
}

// On Linux, reservations are made on a page by page basis, nothing to do.
inline void os::pd_split_reserved_memory(char *base, size_t size,
                                      size_t split, bool realloc) {
}

// Bang the shadow pages if they need to be touched to be mapped.
inline void os::bang_stack_shadow_pages() {
}

inline const int os::default_file_open_flags() { return 0;}

inline int os::fsync(int fd) {
  return ::fsync(fd);
}

// macros for restartable system calls

#define RESTARTABLE(_cmd, _result) do { \
    _result = _cmd; \
  } while(((int)_result == OS_ERR) && (errno == EINTR))

#define RESTARTABLE_RETURN_INT(_cmd) do { \
  int _result; \
  RESTARTABLE(_cmd, _result); \
  return _result; \
} while(false)

inline bool os::numa_has_static_binding()   { return true; }

inline size_t os::restartable_read(int fd, void *buf, unsigned int nBytes) {
  size_t res;
  RESTARTABLE( (size_t) ::read(fd, buf, (size_t) nBytes), res);
  return res;
}

inline int os::socket(int domain, int type, int protocol) {
  return ::socket(domain, type, protocol);
}

inline int os::timeout(int fd, long timeout) {
  julong prevtime,newtime;
  struct timeval t;

  gettimeofday(&t, NULL);
  prevtime = ((julong)t.tv_sec * 1000)  +  t.tv_usec / 1000;

  for(;;) {
    struct pollfd pfd;

    pfd.fd = fd;
    pfd.events = POLL_IN | POLL_ERR;

    int res = ::poll(&pfd, 1, timeout);

    if (res == OS_ERR && errno == EINTR) {

      // On Linux any value < 0 means "forever"

      if(timeout >= 0) {
        gettimeofday(&t, NULL);
        newtime = ((julong)t.tv_sec * 1000)  +  t.tv_usec / 1000;
        timeout -= newtime - prevtime;
        if(timeout <= 0)
          return OS_OK;
        prevtime = newtime;
      }
    } else
      return res;
  }
}

inline int os::listen(int fd, int count) {
  return ::listen(fd, count);
}

inline int os::accept(int fd, struct sockaddr* him, socklen_t* len) {
  // Linux doc says this can't return EINTR, unlike accept() on Solaris.
  // But see attachListener_linux.cpp, LinuxAttachListener::dequeue().
  return (int)::accept(fd, him, len);
}

inline int os::recvfrom(int fd, char* buf, size_t nBytes, uint flags,
                        sockaddr* from, socklen_t* fromlen) {
  RESTARTABLE_RETURN_INT((int)::recvfrom(fd, buf, nBytes, flags, from, fromlen));
}

inline int os::sendto(int fd, char* buf, size_t len, uint flags,
                      struct sockaddr* to, socklen_t tolen) {
  RESTARTABLE_RETURN_INT((int)::sendto(fd, buf, len, flags, to, tolen));
}

inline int os::socket_shutdown(int fd, int howto) {
  return ::shutdown(fd, howto);
}

inline int os::bind(int fd, struct sockaddr* him, socklen_t len) {
  return ::bind(fd, him, len);
}

inline int os::get_sock_name(int fd, struct sockaddr* him, socklen_t* len) {
  return ::getsockname(fd, him, len);
}

inline int os::get_host_name(char* name, int namelen) {
  return ::gethostname(name, namelen);
}

inline struct hostent* os::get_host_by_name(char* name) {
  return ::gethostbyname(name);
}

inline int os::get_sock_opt(int fd, int level, int optname,
                            char* optval, socklen_t* optlen) {
  return ::getsockopt(fd, level, optname, optval, optlen);
}

inline int os::set_sock_opt(int fd, int level, int optname,
                            const char* optval, socklen_t optlen) {
  return ::setsockopt(fd, level, optname, optval, optlen);
}

#endif // HOTSPOT_TARGET_CLASSLIB
#endif // OS_LINUX_OS_LINUX_INLINE_HPP
