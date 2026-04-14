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
 * @summary Verify that the hs_err crash report contains the correct CompoundVM
 *          vendor version, runtime version, and bug report URL
 * @run main/othervm CrashReportTest
 */

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class CrashReportTest {

    private static final String EXPECTED_URL = "https://github.com/bytedance/CompoundVM/issues";
    private static final String EXPECTED_VENDOR_VERSION = "ByteDance CompoundVM";
    private static final String EXPECTED_RUNTIME_VERSION = "17.0.16+0";

    public static class Crasher {
        public static void main(String[] args) throws Exception {
            Class<?> uc = Class.forName("sun.misc.Unsafe");
            java.lang.reflect.Field f = uc.getDeclaredField("theUnsafe");
            f.setAccessible(true);
            Object unsafe = f.get(null);
            java.lang.reflect.Method putInt = uc.getMethod("putInt", long.class, int.class);
            putInt.invoke(unsafe, 0L, 0);
        }
    }

    public static void main(String[] args) throws Exception {
        String javaHome = System.getProperty("java.home");
        String javaBin = javaHome + File.separator + "bin" + File.separator + "java";
        String classpath = System.getProperty("java.class.path");
        String testClasses = System.getProperty("test.classes", classpath);

        List<String> cmd = new ArrayList<>();
        cmd.add(javaBin);
        cmd.add("-server17");
        cmd.add("-Xmx64m");
        cmd.add("-XX:+IgnoreUnrecognizedVMOptions");
        cmd.add("-cp");
        cmd.add(testClasses);
        cmd.add("CrashReportTest$Crasher");

        System.out.println("Launching child process to trigger JVM crash...");
        System.out.println("Command: " + String.join(" ", cmd));

        ProcessBuilder pb = new ProcessBuilder(cmd);
        pb.redirectErrorStream(true);
        Process process = pb.start();

        List<String> outputLines = new ArrayList<>();
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                outputLines.add(line);
                System.out.println("[child] " + line);
            }
        }

        int exitCode = process.waitFor();
        System.out.println("Child process exited with code: " + exitCode);

        if (exitCode == 0) {
            throw new RuntimeException("Child process should have crashed (non-zero exit), but exited normally.");
        }

        String hsErrPath = findHsErrPath(outputLines);
        if (hsErrPath == null) {
            throw new RuntimeException("Could not find hs_err file path in child process output.\n"
                    + "Output was:\n" + String.join("\n", outputLines));
        }

        System.out.println("Found hs_err file: " + hsErrPath);

        File hsErrFile = new File(hsErrPath);
        if (!hsErrFile.exists()) {
            throw new RuntimeException("hs_err file does not exist: " + hsErrFile.getAbsolutePath());
        }

        List<String> hsErrLines = readFile(hsErrFile);

        verifyJREVersion(hsErrLines);
        verifyBugUrl(hsErrLines);

        hsErrFile.delete();
        System.out.println("TEST PASSED");
    }

    private static String findHsErrPath(List<String> lines) {
        Pattern pattern = Pattern.compile("#\\s*(\\S*hs_err_pid\\d+\\.log)");
        for (String line : lines) {
            Matcher matcher = pattern.matcher(line);
            if (matcher.find()) {
                return matcher.group(1);
            }
        }
        return null;
    }

    private static List<String> readFile(File file) throws Exception {
        List<String> lines = new ArrayList<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                lines.add(line);
            }
        }
        return lines;
    }

    private static void verifyJREVersion(List<String> lines) throws Exception {
        Pattern pattern = Pattern.compile(
                "# JRE version:.*" + Pattern.quote(EXPECTED_VENDOR_VERSION)
                        + ".*\\(build " + Pattern.quote(EXPECTED_RUNTIME_VERSION) + "\\)");
        for (String line : lines) {
            if (pattern.matcher(line).find()) {
                System.out.println("PASS: JRE version line contains vendor version and runtime version.");
                System.out.println("  Line: " + line);
                return;
            }
        }
        throw new RuntimeException(
                "Could not find JRE version line with vendor version '" + EXPECTED_VENDOR_VERSION
                        + "' and runtime version '" + EXPECTED_RUNTIME_VERSION + "' in hs_err file.");
    }

    private static void verifyBugUrl(List<String> lines) throws Exception {
        boolean foundBugLine = false;
        for (String line : lines) {
            if (line.contains("If you would like to submit a bug report, please visit:")) {
                foundBugLine = true;
                continue;
            }
            if (foundBugLine) {
                String trimmed = line.replaceFirst("^#\\s*", "").trim();
                if (!trimmed.isEmpty()) {
                    System.out.println("Found bug report URL in hs_err: " + trimmed);
                    if (!trimmed.equals(EXPECTED_URL)) {
                        throw new RuntimeException(
                                "Bug report URL mismatch!\n"
                                        + "  Expected: " + EXPECTED_URL + "\n"
                                        + "  Actual:   " + trimmed);
                    }
                    System.out.println("PASS: Bug report URL matches expected value.");
                    return;
                }
            }
        }
        if (!foundBugLine) {
            throw new RuntimeException("Could not find 'submit a bug report' line in hs_err file.");
        }
        throw new RuntimeException("Found 'submit a bug report' line but no URL followed.");
    }
}
