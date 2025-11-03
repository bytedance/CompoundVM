/*
 * Copyright (c) 2015, 2024, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
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
 */
package java.lang;

/**
 * A stack walker.
 *
 * <p> The {@link StackWalker#walk walk} method opens a sequential stream
 * of {@link StackFrame StackFrame}s for the current thread and then applies
 * the given function to walk the {@code StackFrame} stream.
 * The stream reports stack frame elements in order, from the top most frame
 * that represents the execution point at which the stack was generated to
 * the bottom most frame.
 * The {@code StackFrame} stream is closed when the {@code walk} method returns.
 * If an attempt is made to reuse the closed stream,
 * {@code IllegalStateException} will be thrown.
 *
 * <p> {@linkplain Option <em>Stack walker options</em>} configure the stack frame
 * information obtained by a {@code StackWalker}.
 * By default, the class name and method information are collected but
 * not the {@link StackFrame#getDeclaringClass() Class reference}.
 * The method information can be dropped via the {@link Option#DROP_METHOD_INFO
 * DROP_METHOD_INFO} option. The {@code Class} object can be retained for
 * access via the {@link Option#RETAIN_CLASS_REFERENCE RETAIN_CLASS_REFERENCE} option.
 * Stack frames of the reflection API and implementation classes are
 * {@linkplain Option#SHOW_HIDDEN_FRAMES hidden} by default.
 *
 * <p> {@code StackWalker} is thread-safe. Multiple threads can share
 * a single {@code StackWalker} object to traverse its own stack.
 *
 * @apiNote
 * Examples
 *
 * <p>1. To find the first caller filtering out a known list of implementation class:
 * {@snippet lang="java" :
 *     StackWalker walker = StackWalker.getInstance(Set.of(Option.DROP_METHOD_INFO, Option.RETAIN_CLASS_REFERENCE));
 *     Optional<Class<?>> callerClass = walker.walk(s ->
 *             s.map(StackFrame::getDeclaringClass)
 *              .filter(Predicate.not(implClasses::contains))
 *              .findFirst());
 * }
 *
 * <p>2. To snapshot the top 10 stack frames of the current thread,
 * {@snippet lang="java" :
 *     List<StackFrame> stack = StackWalker.getInstance().walk(s -> s.limit(10).toList());
 * }
 *
 * Unless otherwise noted, passing a {@code null} argument to a
 * constructor or method in this {@code StackWalker} class
 * will cause a {@link NullPointerException NullPointerException}
 * to be thrown.
 *
 * @since 9
 */
public final class StackWalker {}
