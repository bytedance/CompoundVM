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
