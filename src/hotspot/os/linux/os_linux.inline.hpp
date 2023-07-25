// This project is a modified version of OpenJDK, licensed under GPL v2.
// Modifications Copyright (C) 2025 ByteDance Inc.
/*
 * Copyright (c) 1999, 2021, Oracle and/or its affiliates. All rights reserved.
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

// os_linux.hpp included by os.hpp

#include "runtime/os.hpp"
#include "os_posix.inline.hpp"

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

#if defined(HOTSPOT_TARGET_CLASSLIB) && HOTSPOT_TARGET_CLASSLIB == 8
#include <unistd.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netdb.h>

inline int os::timeout(int fd, long timeout) {
  julong prevtime,newtime;
  struct timeval t;

  gettimeofday(&t, NULL);
  prevtime = ((julong)t.tv_sec * 1000)  +  t.tv_usec / 1000;

  for(;;) {
    struct pollfd pfd;

    pfd.fd = fd;
    pfd.events = POLLIN | POLLERR;

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

inline int os::connect(int fd, struct sockaddr* him, socklen_t len) {
  RESTARTABLE_RETURN_INT(::connect(fd, him, len));
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

inline int os::unlink(const char *path) {
  return ::unlink(path);
}
#endif

// Trim-native support
inline bool os::can_trim_native_heap() {
#ifdef __GLIBC__
  return true;
#else
  return false; // musl
#endif
}

#endif // OS_LINUX_OS_LINUX_INLINE_HPP
