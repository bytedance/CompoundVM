/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
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
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

/*
 * @test
 * @bug 8218152
 * @summary A bad annotation processor class file should fail with an error
 * @author Steven Groeger
 *
 * @library /tools/javac/lib
 * @build ToolBox
 * @run main MalformedAnnotationProcessorTests
 */

import java.io.*;
import java.util.*;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import javax.annotation.processing.Processor;

public class MalformedAnnotationProcessorTests {
    public static void main(String... args) throws Exception {
        new MalformedAnnotationProcessorTests().run();
    }

    void run() throws Exception {
        testBadAnnotationProcessor(Paths.get("."));
        testMissingAnnotationProcessor(Paths.get("."));
        testWrongClassFileVersion(Paths.get("."));
    }

    public void testBadAnnotationProcessor(Path base) throws Exception {
        Path apDir = base.resolve("annoprocessor");
        ToolBox.writeFile(apDir.resolve("META-INF").resolve("services")
                          .resolve(Processor.class.getCanonicalName()), "BadAnnoProcessor");
        ToolBox.writeFile(apDir.resolve("BadAnnoProcessor.class"), "badannoprocessor");

        Path classes = base.resolve("classes");
        Files.createDirectories(classes);

        List<String> actualErrors = new ArrayList<>();
        ToolBox.JavaToolArgs args = new ToolBox.JavaToolArgs();
        args.setSources("package test; public class Test {}")
            .appendArgs("-XDrawDiagnostics",
                        "-d", classes.toString(),
                        "-classpath", "",
                        "-processorpath", apDir.toString())
            .set(ToolBox.Expect.FAIL)
            .setErrOutput(actualErrors);
        ToolBox.javac(args);

        System.out.println(actualErrors.get(0));
        if (!actualErrors.get(0).contains("- compiler.err.proc.cant.load.class: " +
                                          "Incompatible magic value")) {
            throw new AssertionError("Unexpected errors reported: " + actualErrors);
        }
    }

    public void testMissingAnnotationProcessor(Path base) throws Exception {
        Path apDir = base.resolve("annoprocessor");
        ToolBox.writeFile(apDir.resolve("META-INF").resolve("services").resolve(Processor.class.getCanonicalName()),
                     "MissingAnnoProcessor");

        Path classes = base.resolve("classes");
        Files.createDirectories(classes);

        List<String> actualErrors = new ArrayList<>();
        ToolBox.JavaToolArgs args = new ToolBox.JavaToolArgs();
        args.setSources("package test; public class Test {}")
            .appendArgs("-XDrawDiagnostics",
                        "-d", classes.toString(),
                        "-classpath", "",
                        "-processorpath", apDir.toString())
            .set(ToolBox.Expect.FAIL)
            .setErrOutput(actualErrors);
        ToolBox.javac(args);

        if (!actualErrors.get(0).contains("- compiler.err.proc.bad.config.file: " +
            "javax.annotation.processing.Processor: Provider MissingAnnoProcessor not found")) {
            throw new AssertionError("Unexpected errors reported: " + actualErrors);
        }
    }

    public void testWrongClassFileVersion(Path base) throws Exception {
        Path apDir = base.resolve("annoprocessor");
        ToolBox.writeFile(apDir.resolve("META-INF").resolve("services").resolve(Processor.class.getCanonicalName()),
                     "WrongClassFileVersion");

        ToolBox.JavaToolArgs args = new ToolBox.JavaToolArgs();
        args.setSources("class WrongClassFileVersion {}")
            .appendArgs("-d", apDir.toString())
            .set(ToolBox.Expect.SUCCESS);
        ToolBox.javac(args);

        increaseMajor(apDir.resolve("WrongClassFileVersion.class"), 1);

        Path classes = base.resolve("classes");
        Files.createDirectories(classes);

        List<String> actualErrors = new ArrayList<>();
        args = new ToolBox.JavaToolArgs();
        args.setSources("package test; public class Test {}")
            .appendArgs("-XDrawDiagnostics",
                        "-d", classes.toString(),
                        "-classpath", "",
                        "-processorpath", apDir.toString())
            .set(ToolBox.Expect.FAIL)
            .setErrOutput(actualErrors);
        ToolBox.javac(args);

        // cvm supports a higher bytecode version, so the test would not fail. Keep
        // the higher version support and skip the test.
        if (!System.getProperty("java.vm.specification.version").equals("8"))
            return;
        if (!actualErrors.get(0).contains("- compiler.err.proc.cant.load.class: " +
            "WrongClassFileVersion has been compiled by a more recent version")) {
            throw new AssertionError("Unexpected errors reported: " + actualErrors);
        }
    }

    // Increase class file cfile's major version by delta
    // (note: based on test/langtools/tools/javac/6330997/T6330997.java)
    static void increaseMajor(Path cfile, int delta) {
        try (RandomAccessFile cls =
             new RandomAccessFile(cfile.toFile(), "rw");
             FileChannel fc = cls.getChannel()) {
            ByteBuffer rbuf = ByteBuffer.allocate(2);
            fc.read(rbuf, 6);
            ByteBuffer wbuf = ByteBuffer.allocate(2);
            wbuf.putShort(0, (short)(rbuf.getShort(0) + delta));
            fc.write(wbuf, 6);
            fc.force(false);
        } catch (Exception e){
            throw new RuntimeException("Failed: unexpected exception");
        }
     }
}
