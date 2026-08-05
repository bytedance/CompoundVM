/*
 * @test
 * @summary Verify that available Cleaner implementations can be created
 *          through reflection without throwing an exception
 * @run main/othervm CleanerCreateTest
 */

import java.lang.reflect.Method;

public class CleanerCreateTest {
    private static final String SUN_CLEANER = "sun.misc.Cleaner";
    private static final String JAVA_CLEANER = "java.lang.ref.Cleaner";

    public static void main(String[] args) throws Exception {
        testSunCleaner();
        testJavaCleaner();
    }

    private static void testSunCleaner() throws Exception {
        Class<?> cleanerClass = loadClass(SUN_CLEANER);
        if (cleanerClass == null) {
            return;
        }

        Method create = cleanerClass.getDeclaredMethod(
                "create", Object.class, Runnable.class);
        create.invoke(null, new Object(), new Runnable() {
            @Override
            public void run() {
                // No cleanup is required by this test.
            }
        });
    }

    private static void testJavaCleaner() throws Exception {
        Class<?> cleanerClass = loadClass(JAVA_CLEANER);
        if (cleanerClass == null) {
            return;
        }

        Method create = cleanerClass.getDeclaredMethod("create");
        create.invoke(null);
    }

    private static Class<?> loadClass(String className) {
        try {
            return Class.forName(className);
        } catch (ClassNotFoundException e) {
            return null;
        }
    }
}
