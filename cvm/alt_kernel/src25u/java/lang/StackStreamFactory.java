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
 * StackStreamFactory class provides static factory methods
 * to get different kinds of stack walker/traverser.
 *
 * AbstractStackWalker provides the basic stack walking support
 * fetching stack frames from VM in batches.
 *
 * AbstractStackWalker subclass is specialized for a specific kind of stack traversal
 * to avoid overhead of Stream/Lambda
 * 1. Support traversing Stream<StackFrame>
 * 2. StackWalker::getCallerClass
 * 3. AccessControlContext getting ProtectionDomain
 */
final class StackStreamFactory {
    /**
     * Subclass of AbstractStackWalker implements a specific stack walking logic.
     * It needs to set up the frame buffer and stack walking mode.
     *
     * It initiates the VM stack walking via the callStackWalk method that serves
     * as the anchored frame and VM will call up to AbstractStackWalker::doStackWalk.
     *
     * @param <R> the type of the result returned from stack walking
     * @param <T> the type of the data gathered for each frame.
     *            For example, StackFrameInfo for StackWalker::walk or
     *            Class<?> for StackWalker::getCallerClass
     */
    abstract static class AbstractStackWalker<R, T> {}
}
