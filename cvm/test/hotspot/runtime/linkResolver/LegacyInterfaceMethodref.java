// This project is a modified version of OpenJDK, licensed under GPL v2.
// Modifications Copyright (C) 2026 ByteDance Inc.
/*
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
 */

/*
 * @test
 * @summary Verify CVM8 accepts the legacy Scala 2.11 Methodref encoding for an interface static method
 * @compile LegacyInterfaceMethodrefCaller.jasm
 * @compile LegacyInterfaceMethodref.java
 * @run main/othervm LegacyInterfaceMethodref
 * @run main/othervm -Xint LegacyInterfaceMethodref
 * @run main/othervm -Xcomp LegacyInterfaceMethodref
 * @run main/othervm -XX:-AllowLegacyInterfaceMethodref -DexpectICCE=true LegacyInterfaceMethodref
 */

interface LegacyStaticInterface {
    static int value() {
        return 42;
    }
}

public class LegacyInterfaceMethodref {
    public static void main(String[] args) {
        boolean expectICCE = Boolean.getBoolean("expectICCE");
        try {
            int result = LegacyInterfaceMethodrefCaller.call();
            if (expectICCE) {
                throw new AssertionError("Expected IncompatibleClassChangeError");
            }
            if (result != 42) {
                throw new AssertionError("Unexpected result: " + result);
            }
        } catch (IncompatibleClassChangeError error) {
            if (!expectICCE) {
                throw error;
            }
        }
    }
}
