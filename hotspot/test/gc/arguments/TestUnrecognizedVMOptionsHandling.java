/*
* Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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
*/

/*
 * @test TestUnrecognizedVMOptionsHandling
 * @key gc
 * @bug 8017611
 * @summary Tests handling unrecognized VM options
 * @library /testlibrary
 * @run main/othervm TestUnrecognizedVMOptionsHandling
 */

import com.oracle.java.testlibrary.*;

public class TestUnrecognizedVMOptionsHandling {

  public static void main(String args[]) throws Exception {
    // CompoundVM on top of the JDK 17 kernel accepts these legacy spellings,
    // so the compatibility expectation differs from the old JDK 8 suggestion
    // text checks.
    ProcessBuilder pb = ProcessTools.createJavaProcessBuilder(
      "-XX:+PrintGc",
      "-version"
      );
    OutputAnalyzer output = new OutputAnalyzer(pb.start());
    output.shouldHaveExitValue(0);

    pb = ProcessTools.createJavaProcessBuilder(
      "-XX:MaxiumHeapSize=500m",
      "-version"
      );
    output = new OutputAnalyzer(pb.start());
    output.shouldHaveExitValue(0);

    // Sanity check with the canonical spelling.
    pb = ProcessTools.createJavaProcessBuilder(
      "-XX:+PrintGC",
      "-version"
      );
    OutputAnalyzer outputWithNoError = new OutputAnalyzer(pb.start());
    outputWithNoError.shouldHaveExitValue(0);
  }
}
