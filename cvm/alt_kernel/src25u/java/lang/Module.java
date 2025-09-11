/*
 * Copyright (c) 2014, 2025, Oracle and/or its affiliates. All rights reserved.
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
 * Represents a run-time module, either {@link #isNamed() named} or unnamed.
 *
 * <p> Named modules have a {@link #getName() name} and are constructed by the
 * Java Virtual Machine when a graph of modules is defined to the Java virtual
 * machine to create a {@linkplain ModuleLayer module layer}. </p>
 *
 * <p> An unnamed module does not have a name. There is an unnamed module for
 * each {@link ClassLoader ClassLoader}, obtained by invoking its {@link
 * ClassLoader#getUnnamedModule() getUnnamedModule} method. All types that are
 * not in a named module are members of their defining class loader's unnamed
 * module. </p>
 *
 * <p> The package names that are parameters or returned by methods defined in
 * this class are the fully-qualified names of the packages as defined in
 * section {@jls 6.5.3} of <cite>The Java Language Specification</cite>, for
 * example, {@code "java.lang"}. </p>
 *
 * <p> Unless otherwise specified, passing a {@code null} argument to a method
 * in this class causes a {@link NullPointerException NullPointerException} to
 * be thrown. </p>
 *
 * @since 9
 * @see Class#getModule()
 * @jls 7.7 Module Declarations
 */

public final class Module {}
