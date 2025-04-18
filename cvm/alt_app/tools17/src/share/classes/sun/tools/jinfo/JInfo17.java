// This project is a modified version of OpenJDK, licensed under GPL v2.
// Modifications Copyright (C) 2025 ByteDance Inc.
/*
 * Copyright (c) 2006, 2019, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
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
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package sun.tools.jinfo;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.Arrays;

import com.sun.tools.attach.VirtualMachine;

import sun.tools.attach.HotSpotVirtualMachine;

/*
 * This class is the main class for the JInfo utility of JDK17. It parses its arguments
 * and decides if the command should be satisfied using the VM attach mechanism
 * or an SA tool.
 */
final public class JInfo17 {

    public static void main(String[] args) throws Exception {
        if (args.length == 0) {
            usage(1); // no arguments
        }
        checkForUnsupportedOptions(args);

        boolean doFlag = false;
        boolean doFlags = false;
        boolean doSysprops = false;
        int flag = -1;

        // Parse the options (arguments starting with "-" )
        int optionCount = 0;
        while (optionCount < args.length) {
            String arg = args[optionCount];
            if (!arg.startsWith("-")) {
                break;
            }

            optionCount++;

            if (arg.equals("-?") ||
                arg.equals("-h") ||
                arg.equals("--help") ||
                // -help: legacy.
                arg.equals("-help")) {
                usage(0);
            }

            if (arg.equals("-flag")) {
                doFlag = true;
                // Consume the flag
                if (optionCount < args.length) {
                    flag = optionCount++;
                    break;
                }
                usage(1);
            }

            if (arg.equals("-flags")) {
                doFlags = true;
                break;
            }

            if (arg.equals("-sysprops")) {
                doSysprops = true;
                break;
            }
        }

        int paramCount = args.length - optionCount;
        if (paramCount != 1) {
            usage(1);
        }

        String parg = args[optionCount];
        String pid = getVirtualMachinePid(parg);

        if (!doFlag && !doFlags && !doSysprops) {
            // Print flags and sysporps if no options given
            sysprops(pid);
            System.out.println();
            flags(pid);
            System.out.println();
            commandLine(pid);
        }
        if (doFlag) {
            if (flag < 0) {
                System.err.println("Missing flag");
                usage(1);
            }
            flag(pid, args[flag]);
        }
        if (doFlags) {
            flags(pid);
        }
        if (doSysprops) {
            sysprops(pid);
        }
    }

    private static void flag(String pid, String option) throws IOException {
        HotSpotVirtualMachine vm = (HotSpotVirtualMachine) attach(pid);
        String flag;
        InputStream in;
        int index = option.indexOf('=');
        if (index != -1) {
            flag = option.substring(0, index);
            String value = option.substring(index + 1);
            in = vm.setFlag(flag, value);
        } else {
            char c = option.charAt(0);
            switch (c) {
                case '+':
                    flag = option.substring(1);
                    in = vm.setFlag(flag, "1");
                    break;
                case '-':
                    flag = option.substring(1);
                    in = vm.setFlag(flag, "0");
                    break;
                default:
                    flag = option;
                    in = vm.printFlag(flag);
                    break;
            }
        }

        drain(vm, in);
    }

    private static void flags(String pid) throws IOException {
        HotSpotVirtualMachine vm = (HotSpotVirtualMachine) attach(pid);
        InputStream in = vm.executeJCmd("VM.flags");
        System.out.println("VM Flags:");
        drain(vm, in);
    }

    private static void commandLine(String pid) throws IOException {
        HotSpotVirtualMachine vm = (HotSpotVirtualMachine) attach(pid);
        InputStream in = vm.executeJCmd("VM.command_line");
        drain(vm, in);
    }

    private static void sysprops(String pid) throws IOException {
        HotSpotVirtualMachine vm = (HotSpotVirtualMachine) attach(pid);
        InputStream in = vm.executeJCmd("VM.system_properties");
        System.out.println("Java System Properties:");
        drain(vm, in);
    }

    // Attach to <pid>, exiting if we fail to attach
    private static VirtualMachine attach(String pid) {
        try {
            return VirtualMachine.attach(pid);
        } catch (Exception x) {
            String msg = x.getMessage();
            if (msg != null) {
                System.err.println(pid + ": " + msg);
            } else {
                x.printStackTrace();
            }
            System.exit(1);
            return null; // keep compiler happy
        }
    }

    // Read the stream from the target VM until EOF, then detach
    private static void drain(VirtualMachine vm, InputStream in) throws IOException {
        drainUTF8(in, System.out);
        vm.detach();
    }

    private static void checkForUnsupportedOptions(String[] args) {
        // Check arguments for -F, and non-numeric value
        // and warn the user that SA is not supported anymore
        int maxCount = 1;
        int paramCount = 0;

        for (String s : args) {
            if (s.equals("-F")) {
                SAOptionError("-F option used");
            }
            if (s.equals("-flag")) {
                maxCount = 2;
            }
            if (! s.startsWith("-")) {
                paramCount += 1;
            }
        }

        if (paramCount > maxCount) {
            SAOptionError("More than " + maxCount + " non-option argument");
        }
    }

    private static void SAOptionError(String msg) {
        System.err.println("Error: " + msg);
        System.err.println("Cannot connect to core dump or remote debug server. Use jhsdb17 jinfo instead");
        System.exit(1);
    }

     // print usage message
    private static void usage(int exit) {
        System.err.println("Usage:");
        System.err.println("    jinfo17 <option> <pid>");
        System.err.println("       (to connect to a running process)");
        System.err.println("");
        System.err.println("where <option> is one of:");
        System.err.println("    -flag <name>         to print the value of the named VM flag");
        System.err.println("    -flag [+|-]<name>    to enable or disable the named VM flag");
        System.err.println("    -flag <name>=<value> to set the named VM flag to the given value");
        System.err.println("    -flags               to print VM flags");
        System.err.println("    -sysprops            to print Java system properties");
        System.err.println("    <no option>          to print both VM flags and system properties");
        System.err.println("    -? | -h | --help | -help to print this help message");
        System.exit(exit);
    }

    private static String getVirtualMachinePid(String pidArg) {
        String singlePid;

        if (pidArg == null || pidArg.isEmpty()) {
            throw new IllegalArgumentException("Pid string is invalid");
        }
        if (pidArg.charAt(0) == '-') {
            throw new IllegalArgumentException("Unrecognized " + pidArg);
        }
        try {
            long pid = Long.parseLong(pidArg);
            singlePid = String.valueOf(pid);
            return singlePid;
        } catch (NumberFormatException nfe) {
            throw new IllegalArgumentException("Pid string is invalid");
        }
    }

    /*
     * Reads characters in UTF-8 format from the input stream and prints them
     * with the given print stream. Closes the input stream before it returns.
     *
     * @return The number of printed characters.
     */
    private static long drainUTF8(InputStream is, PrintStream ps) throws IOException {
        long result = 0;

        try (BufferedInputStream bis = new BufferedInputStream(is);
             InputStreamReader isr = new InputStreamReader(bis, "UTF-8")) {
            char c[] = new char[256];
            int n;

            do {
                n = isr.read(c);

                if (n > 0) {
                    result += n;
                    ps.print(n == c.length ? c : Arrays.copyOf(c, n));
                }
            } while (n > 0);
        }

        return result;
    }
}
