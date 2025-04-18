// This project is a modified version of OpenJDK, licensed under GPL v2.
// Modifications Copyright (C) 2025 ByteDance Inc.
/*
 * Copyright (c) 2005, 2019, Oracle and/or its affiliates. All rights reserved.
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

package sun.tools.jstack;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.Arrays;

import com.sun.tools.attach.VirtualMachine;
import sun.tools.attach.HotSpotVirtualMachine;

/*
 * This class is the main class for the JStack utility of JDK17. It parses its arguments
 * and decides if the command should be executed by the SA JStack tool or by
 * obtained the thread dump from a target process using the VM attach mechanism
 */
public class JStack17 {

    public static void main(String[] args) throws Exception {
        if (args.length == 0) {
            usage(1); // no arguments
        }

        checkForUnsupportedOptions(args);

        boolean locks = false;
        boolean extended = false;

        // Parse the options (arguments starting with "-" )
        int optionCount = 0;
        while (optionCount < args.length) {
            String arg = args[optionCount];
            if (!arg.startsWith("-")) {
                break;
            }
            if (arg.equals("-?")     ||
                arg.equals("-h")     ||
                arg.equals("--help") ||
                // -help: legacy.
                arg.equals("-help")) {
                usage(0);
            }
            else {
                if (arg.equals("-l")) {
                    locks = true;
                } else {
                    if (arg.equals("-e")) {
                        extended = true;
                    } else {
                        usage(1);
                    }
                }
            }
            optionCount++;
        }

        // Next we check the parameter count.
        int paramCount = args.length - optionCount;
        if (paramCount != 1) {
            usage(1);
        }

        // pass -l to thread dump operation to get extra lock info
        String pidArg = args[optionCount];
        String params[]= new String[] { "" };
        if (extended) {
            params[0] += "-e ";
        }
        if (locks) {
            params[0] += "-l";
        }

        String pid = getVirtualMachinePid(pidArg);
        runThreadDump(pid, params);
    }

    // Attach to pid and perform a thread dump
    private static void runThreadDump(String pid, String args[]) throws Exception {
        VirtualMachine vm = null;
        try {
            vm = VirtualMachine.attach(pid);
        } catch (Exception x) {
            String msg = x.getMessage();
            if (msg != null) {
                System.err.println(pid + ": " + msg);
            } else {
                x.printStackTrace();
            }
            System.exit(1);
        }

        // Cast to HotSpotVirtualMachine as this is implementation specific
        // method.
        InputStream in = ((HotSpotVirtualMachine)vm).remoteDataDump((Object[])args);
        // read to EOF and just print output
        drainUTF8(in, System.out);
        vm.detach();
    }

    private static void checkForUnsupportedOptions(String[] args) {
        // Check arguments for -F, -m, and non-numeric value
        // and warn the user that SA is not supported anymore

        int paramCount = 0;

        for (String s : args) {
            if (s.equals("-F")) {
                SAOptionError("-F option used");
            }

            if (s.equals("-m")) {
                SAOptionError("-m option used");
            }

            if (! s.startsWith("-")) {
                paramCount += 1;
            }
        }

        if (paramCount > 1) {
            SAOptionError("More than one non-option argument");
        }
    }

    private static void SAOptionError(String msg) {
        System.err.println("Error: " + msg);
        System.err.println("Cannot connect to core dump or remote debug server. Use jhsdb17 jstack instead");
        System.exit(1);
    }

    // print usage message
    private static void usage(int exit) {
        System.err.println("Usage:");
        System.err.println("    jstack17 [-l][-e] <pid>");
        System.err.println("        (to connect to running process)");
        System.err.println("");
        System.err.println("Options:");
        System.err.println("    -l  long listing. Prints additional information about locks");
        System.err.println("    -e  extended listing. Prints additional information about threads");
        System.err.println("    -? -h --help -help to print this help message");
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
