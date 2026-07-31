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
 * @bug 8000000
 * @summary Verifies that jdk.internal.misc.Unsafe.registerNatives() has been
 *          invoked by the time the class is initialized so that private
 *          natives (objectFieldOffset0/1, allocateMemory0, etc.) installed
 *          via JVM_RegisterJDKInternalMiscUnsafeMethods are callable. This
 *          guards against a classfile build regression (stale rt17.jar bytes)
 *          that would otherwise surface as an UnsatisfiedLinkError in
 *          jdk.internal.ref.CleanerImpl -> InnocuousThread.<clinit>, which
 *          is exactly how the jffi/jnr-unixsocket library path fails inside
 *          applications like Flink shaded metrics.
 *
 *          Run with -server17 to exercise the JDK-17-based kernel where
 *          jdk.internal.* classes from rt17.jar are on the boot class path.
 *
 * @run main/othervm -server17 JDKInternalMiscUnsafeRegisterNatives
 */

import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class JDKInternalMiscUnsafeRegisterNatives {

    private static long jdkInternalObjectFieldOffset(Class<?> c, String name) throws Exception {
        Class<?> iu = Class.forName("jdk.internal.misc.Unsafe");
        Field uf = iu.getDeclaredField("theUnsafe");
        uf.setAccessible(true);
        Object theUnsafe = uf.get(null);
        Method of1 = iu.getDeclaredMethod("objectFieldOffset1", Class.class, String.class);
        of1.setAccessible(true);
        return (Long) of1.invoke(theUnsafe, c, name);
    }

    private static long jdkInternalStaticFieldOffset(Field f) throws Exception {
        Class<?> iu = Class.forName("jdk.internal.misc.Unsafe");
        Field uf = iu.getDeclaredField("theUnsafe");
        uf.setAccessible(true);
        Object theUnsafe = uf.get(null);
        Method sf0 = iu.getDeclaredMethod("staticFieldOffset0", Field.class);
        sf0.setAccessible(true);
        return (Long) sf0.invoke(theUnsafe, f);
    }

    private static long sunMiscObjectFieldOffset(Field f) throws Exception {
        Class<?> su = Class.forName("sun.misc.Unsafe");
        Field uf = su.getDeclaredField("theUnsafe");
        uf.setAccessible(true);
        Object theUnsafe = uf.get(null);
        Method of = su.getMethod("objectFieldOffset", Field.class);
        return (Long) of.invoke(theUnsafe, f);
    }

    private static Object createCleaner() throws Exception {
        Class<?> cleaner = Class.forName("java.lang.ref.Cleaner");
        return cleaner.getMethod("create").invoke(null);
    }

    public static void main(String[] args) throws Exception {
        String vmName = System.getProperty("java.vm.name");
        if (vmName == null || !vmName.contains("17.")) {
            System.out.println("Skipping: not a -server17 launch (java.vm.name=" + vmName + ")");
            return;
        }

        Field stringValue = String.class.getDeclaredField("value");

        // 1) jdk.internal.misc.Unsafe.objectFieldOffset1 -> must not ULE
        long jdkOf1 = jdkInternalObjectFieldOffset(String.class, "value");
        long sunOf = sunMiscObjectFieldOffset(stringValue);
        if (jdkOf1 != sunOf) {
            throw new AssertionError(
                    "jdk.internal.misc.Unsafe.objectFieldOffset1(String,\"value\")="
                    + jdkOf1 + " != sun.misc.Unsafe.objectFieldOffset(String.value)=" + sunOf);
        }
        System.out.println("objectFieldOffset1 OK: String.value -> " + jdkOf1);

        // 2) staticFieldOffset0 -> must not ULE, used by JFR/UnsafeConstants paths
        Field theUnsafeStatic = Class.forName("jdk.internal.misc.Unsafe").getDeclaredField("theUnsafe");
        long sf0 = jdkInternalStaticFieldOffset(theUnsafeStatic);
        if (sf0 < 0) {
            throw new AssertionError("negative staticFieldOffset0 -> " + sf0);
        }
        System.out.println("staticFieldOffset0 OK: Unsafe.theUnsafe -> " + sf0);

        // 3) End-to-end: Cleaner.create() -> CleanerImpl.start -> InnocuousThreadFactory
        //    -> InnocuousThread.<clinit> -> Unsafe.objectFieldOffset -> objectFieldOffset1.
        //    This is the exact jffi failure stack reported from Flink shaded metrics.
        Object cleaner = createCleaner();
        if (cleaner == null) {
            throw new AssertionError("Cleaner.create() returned null");
        }
        System.out.println("Cleaner.create OK: " + cleaner);
    }
}
