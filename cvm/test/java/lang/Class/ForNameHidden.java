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
 * @summary use Class.forName to detect anonymous classes
 * @run main/othervm -server17 -Xlog:class+init=info,class+load=info ForNameHidden
 * @run main/othervm -server -verbose:class ForNameHidden
 */

import java.util.LinkedList;
import java.util.List;

public class ForNameHidden {

    public static void main(String argv[]) throws Exception {
        List<String> l = new LinkedList<>();
        System.getProperties().forEach((k,v)->{
            if (!l.stream().anyMatch(s-> s.equals(k))) {
                l.add((String)k);
            }
        });

        try {
            Class.forName("ForNameHidden$$Lambda$1");
            throw new RuntimeException("Should throw ClassNotFoundException");
        } catch (ClassNotFoundException e) {
            // expected
        }
    }
}
