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
 * @summary Verify superclass access checks are performed when a class is loaded
 *          by a different class loader in CVM
 * @run main/othervm SuperclassAccessCheckTest
 */

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class SuperclassAccessCheckTest {
    private static final String SUBCLASS_NAME = SuperclassAccessCheckTest.class.getName() + "$Subclass";

    // This class is intentionally package-private. Although Subclass has the
    // same package name, a copy of it defined by another loader is in a
    // different runtime package and must not be allowed to extend Superclass.
    static class Superclass {
    }

    public static class Subclass extends Superclass {
    }

    private static class SplitLoader extends ClassLoader {
        SplitLoader(ClassLoader parent) {
            super(parent);
        }

        @Override
        protected Class<?> loadClass(String name, boolean resolve)
                throws ClassNotFoundException {
            if (!name.equals(SUBCLASS_NAME)) {
                return super.loadClass(name, resolve);
            }

            synchronized (getClassLoadingLock(name)) {
                Class<?> loaded = findLoadedClass(name);
                if (loaded == null) {
                    byte[] classBytes = readClassBytes(name);
                    loaded = defineClass(name, classBytes, 0, classBytes.length);
                }
                if (resolve) {
                    resolveClass(loaded);
                }
                return loaded;
            }
        }

        private byte[] readClassBytes(String name) throws ClassNotFoundException {
            String resourceName = name.replace('.', '/') + ".class";
            InputStream in = getParent().getResourceAsStream(resourceName);
            if (in == null) {
                throw new ClassNotFoundException(name);
            }

            try {
                ByteArrayOutputStream out = new ByteArrayOutputStream();
                byte[] buffer = new byte[4096];
                int count;
                while ((count = in.read(buffer)) != -1) {
                    out.write(buffer, 0, count);
                }
                return out.toByteArray();
            } catch (IOException ioe) {
                throw new ClassNotFoundException(name, ioe);
            } finally {
                try {
                    in.close();
                } catch (IOException ignored) {
                }
            }
        }
    }

    public static void main(String[] args) throws Exception {
        ClassLoader appLoader = SuperclassAccessCheckTest.class.getClassLoader();
        Class.forName(Superclass.class.getName(), true, appLoader);

        ClassLoader splitLoader = new SplitLoader(appLoader);
        try {
            Class.forName(SUBCLASS_NAME, false, splitLoader);
            throw new RuntimeException("Expected IllegalAccessError was not thrown");
        } catch (IllegalAccessError expected) {
            String message = expected.getMessage();
            if (message == null || !message.contains("cannot access its superclass")) {
                throw new RuntimeException("Unexpected IllegalAccessError message: " + message,
                        expected);
            }
            System.out.println("Passed with expected exception: " + expected);
        }
    }
}
