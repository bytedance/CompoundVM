// This project is a modified version of OpenJDK, licensed under GPL v2.
// Modifications Copyright (C) 2025 ByteDance Inc.
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
#if defined(HOTSPOT_TARGET_CLASSLIB) && HOTSPOT_TARGET_CLASSLIB == 8
  void JNICALL JVM_RegisterSunMiscUnsafeMethods(JNIEnv *env, jclass unsafecls);
#endif
}

jlong Unsafe_field_offset_to_byte_offset(jlong field_offset);

jlong Unsafe_field_offset_from_byte_offset(jlong byte_offset);

#endif // SHARE_PRIMS_UNSAFE_HPP
