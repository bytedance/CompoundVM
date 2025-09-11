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

package jdk.internal.loader;

/**
 * The platform or application class loader. Resources loaded from modules
 * defined to the boot class loader are also loaded via an instance of this
 * ClassLoader type.
 *
 * <p> This ClassLoader supports loading of classes and resources from modules.
 * Modules are defined to the ClassLoader by invoking the {@link #loadModule}
 * method. Defining a module to this ClassLoader has the effect of making the
 * types in the module visible. </p>
 *
 * <p> This ClassLoader also supports loading of classes and resources from a
 * class path of URLs that are specified to the ClassLoader at construction
 * time. The class path may expand at runtime (the Class-Path attribute in JAR
 * files or via instrumentation agents). </p>
 *
 * <p> The delegation model used by this ClassLoader differs to the regular
 * delegation model. When requested to load a class then this ClassLoader first
 * maps the class name to its package name. If there is a module defined to a
 * BuiltinClassLoader containing this package then the class loader delegates
 * directly to that class loader. If there isn't a module containing the
 * package then it delegates the search to the parent class loader and if not
 * found in the parent then it searches the class path. The main difference
 * between this and the usual delegation model is that it allows the platform
 * class loader to delegate to the application class loader, important with
 * upgraded modules defined to the platform class loader.
 */

public class BuiltinClassLoader {}
