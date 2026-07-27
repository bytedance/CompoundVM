/*
 * @test TestCVMG1AsDefaultGC
 * @summary Test TestCVMG1AsDefaultGC default GC selection
 * @library /testlibrary
 * @run driver TestCVMG1AsDefaultGC
 */

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.oracle.java.testlibrary.OutputAnalyzer;
import com.oracle.java.testlibrary.ProcessTools;

public class TestCVMG1AsDefaultGC {
    private static final String USE_G1_GC = "UseG1GC";
    private static final String USE_PARALLEL_GC = "UseParallelGC";

    public static void main(String[] args) throws Exception {
        String VMSpecVersion = System.getProperty("java.vm.specification.version");
        boolean isCVM = VMSpecVersion.equals("17");
        // Only run the test if it is CVM, otherwise skip the test.
        if (isCVM) {
            testDefaultGC();
            testDisabledG1AsDefaultGC();
            testEnabledG1AsDefaultGC();
            testExplicitG1GCWithCVM();
            testExplicitPSGCWithCVM();
            testExplicitG1GC();
        }
    }

    private static void testDefaultGC() throws Exception {
        assertSelectedGC(USE_PARALLEL_GC);
    }

    private static void testDisabledG1AsDefaultGC() throws Exception {
        assertSelectedGC(USE_PARALLEL_GC, "-XX:-CVMG1AsDefaultGC");
    }

    private static void testExplicitG1GC() throws Exception {
        assertSelectedGC(USE_G1_GC,
                "-XX:-CVMG1AsDefaultGC",
                "-XX:+UseG1GC");
    }

    private static void testEnabledG1AsDefaultGC() throws Exception {
        assertSelectedGC(USE_G1_GC, "-XX:+CVMG1AsDefaultGC");
    }

    private static void testExplicitG1GCWithCVM() throws Exception {
        assertSelectedGC(USE_G1_GC,
                "-XX:+CVMG1AsDefaultGC",
                "-XX:+UseG1GC");
    }

    private static void testExplicitPSGCWithCVM() throws Exception {
        assertSelectedGC(USE_PARALLEL_GC,
                "-XX:+CVMG1AsDefaultGC",
                "-XX:+UseParallelGC");
    }

    private static void assertSelectedGC(String expectedGC, String... vmOptions)
            throws Exception {
        List<String> options = new ArrayList<>(Arrays.asList(vmOptions));
        options.add("-XX:+PrintFlagsFinal");
        options.add("-version");
        options.add("-cvm");

        ProcessBuilder processBuilder = ProcessTools.createJavaProcessBuilder(
                options.toArray(new String[options.size()]));
        OutputAnalyzer output = new OutputAnalyzer(processBuilder.start());
        output.shouldHaveExitValue(0);
        output.shouldMatch(flagValuePattern(expectedGC, true));

        String unexpectedGC = expectedGC.equals(USE_G1_GC)
                ? USE_PARALLEL_GC
                : USE_G1_GC;
        output.shouldMatch(flagValuePattern(unexpectedGC, false));
    }

    private static String flagValuePattern(String flagName, boolean value) {
        return "(?m)^ *bool +" + flagName + " +:?= *" + value
                + " +\\{product\\}.*$";
    }
}
