
/*
 * @test
 * @bug 1234567
 * @summary check -Xbootclasspath and -Xbootclasspath/p option works for java17
 * @compile BootClassPathTest.java BootClassPathTestRunner.java
 * @run main/othervm -server17 BootClassPathTestRunner
 */

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.*;
 
public class BootClassPathTestRunner {
    static final File JAVAHOME = new File(System.getProperty("java.home"));
    static final String NEWLINE = System.getProperty("line.separator");

    public static void main(String[] argv) throws Exception {
        String libPath = new File(JAVAHOME, "lib").getPath();
        // Jar order same as os::set_boot_path8
        List<String> jars = Stream.of("rt17.jar", "rt8.jar", "resources.jar", "rt.jar", "jsse.jar", "jce.jar", "charsets.jar", "jfr.jar")
            .map(s -> libPath + "/" + s)
            .collect(Collectors.toList());
        String expect = String.join(":", jars);
        expect = ".:" + expect;
        test("-Xbootclasspath/p:.", expect);
        test("-Xbootclasspath:" + expect, expect);
    }

    static void test(String bootclasspath, String expect) throws Exception {
        List<String> cmd = new ArrayList<String>();
        cmd.add(new File(new File(JAVAHOME, "bin"), "java").getPath());
        cmd.add("-server17");
        cmd.add(bootclasspath);
        cmd.add("BootClassPathTest");
        ProcessBuilder pb = new ProcessBuilder(cmd);
        Process p = pb.start();
        StringWriter sw = new StringWriter();
        BufferedReader in = new BufferedReader(new InputStreamReader(p.getInputStream()));
        sw.write(in.readLine()); // only expect one line
        p.waitFor();
        String actual = sw.toString();
        if (!actual.equals(expect))
            throw new Exception("bootclasspath error.\nExpected:\n" + expect + "\nActual:\n" + actual);
    }
}
 