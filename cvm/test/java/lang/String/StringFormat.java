/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates. All rights reserved.
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
 * @test
 * @summary This exercises String#format patterns and limits.
 * @run main/othervm StringFormat
 */

/*
 * @test
 * @summary This exercises String#format patterns and limits.
 * @requires os.maxMemory >= 2G
 * @requires vm.bits == "64"
 * @run main/othervm -Xmx2g StringFormat 16777216
 */

public class StringFormat {
    private static String input1 = "1. this does not have any percentages at all";
    private static String input2 = "2. this %s has only a simple field";
    private static String input3 = "3. this has a simple field %s and then a complex %-20s";
    private static String input4 = "4. %s %1s %2s %3s %4s %5s %10s %22s";

    public static void main(String... args) {
        verifyCorrectness();
    }

    private static void verifyCorrectness() {
        assertEquals(
                "1. this does not have any percentages at all",
                String.format(input1));

        assertEquals(
                "2. this 参数1 has only a simple field",
                String.format(input2, "参数1"));

        assertEquals(
                "3. this has a simple field 参数A and then a complex 参数B                 ",
                String.format(input3, "参数A", "参数B"));

        assertEquals(
                "4. p1 p2 p3  p4   p5    p6         p7                     p8",
                String.format(
                        input4,
                        "p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8"));

        assertEquals(
                "4. p1 p2 p3 参数4   p5    p6         p7                    参数8",
                String.format(
                        input4,
                        "p1", "p2", "p3", "参数4", "p5", "p6", "p7", "参数8"));
    }

    private static void assertEquals(String expected, String actual) {
        if (!expected.equals(actual)) {
            throw new RuntimeException(
                    "Expected: [" + expected + "]\nActual:   [" + actual + "]");
        }
    }
}
