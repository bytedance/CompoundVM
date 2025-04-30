# Welcome to the CompoundVM!

[中文版](README_cn.md)

For many legacy Java applications (e.g. using Java 8), upgrading the application to
higher version of JDK often requires costly and time-consuming project migration.

CompoundVM (CVM) is a project that aims to bring higher version JVM performance to
lower version JDK. Now you can run your application with advanced JVM features with
almost zero cost to upgrade your project.

The current release is 8.0.0, which enables JVM 17 on JDK 8. We aim to keep up with
the latest JVM. CVM has been used by a number of services in production environment.
The current release supports linux/x86_64 platform only.

CVM is developed under the same licence as the upstream OpenJDK project.

## Features and Benefits

Higher version of JVM brings enhancements in garbage colleciton, JIT, etc.

+ Enhanced ParallelGC and G1GC, next generation ZGC, with higher throughput, lower latency, and less memory footprints
+ Enhanced JIT compiler, support more intrinsics with faster implementation
+ Drop-in replacement for existing JDK, easy to upgrade and rollback

## Using CVM

### Option 1: Download and install

You may download a pre-built CVM from its [release](https://github.com/bytedance/CompoundVM/releases) page, and uncompress the
package to your destination directory.

### Option 2: Build from source

You can build CVM from source, by running the following command:
`make -f cvm.mk cvm8default17`

For more options run `make -f cvm.mk help`

After CVM is installed, command `${CVM_DIR}/bin/java -version` will show the following output:
```
openjdk version "1.8.0_382"
OpenJDK Runtime Environment (build 1.8.0_382-cvm-b00)
OpenJDK 64-Bit Server VM (CompoundVM 8.0.0) (build 17.0.8+0, mixed mode)
```
Notice the VM version, JVM 17 has been enabled in a JDK 8!

## Contributing to CVM

See [CONTRIBUTING.md](CONTRIBUTING.md)
