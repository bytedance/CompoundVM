// This project is a modified version of OpenJDK, licensed under GPL v2.
// Modifications Copyright (C) 2025 ByteDance Inc.
/*
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
 */

/*
 * @test
 * @bug 1234567
 * @summary check String.contains c2 bug is fixed
 * @compile StringContains.java
 * @run main/othervm -server17 StringContains
 */

public class StringContains {
    public static void main(String[] args) throws Exception {
        String hitGroups = "|pre|";
        String[] expLabels = {"pre", "prod", "test"};
        for (int i = 0; i < 1000000; i++)
            testContains(hitGroups, expLabels[i % 2]);
    }

    public static void testContains(String str, String sub) throws Exception {
        boolean contains = str.contains("|" + sub + "|");
        if(sub == "pre" && !contains) {
            throw new Exception(str + " should contain " + sub);
        }
        return;
    }
}
