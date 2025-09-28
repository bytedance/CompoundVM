<div align="center">

# CompoundVM

![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/bytedance/CompoundVM/.github/workflows/main.yml?branch=jdk17u-target8) [![License](https://img.shields.io/github/license/bytedance/CompoundVM)](https://github.com/bytedance/CompoundVM/blob/main/LICENSE) [![GitHub downloads](https://img.shields.io/github/downloads/bytedance/CompoundVM/total)](https://github.com/bytedance/CompoundVM/releases)

[中文版](README_cn.md)

</div>

## Introduction

For many legacy Java applications (e.g. using Java 8), upgrading the application to
higher version of JDK often requires costly and time-consuming project migration.

CompoundVM (CVM) is a project that aims to bring higher version JVM performance to
lower version JDK. Now you can run your application with advanced JVM features with
almost zero cost to upgrade your project.

The current release enables JVM 17 on JDK 8. We aim to keep up with
the latest JVM. JVM 25 on JDK 8 is now under development.
CVM has been used by a number of services in production environment.
The current release supports x86_64/aarch64 on linux platforms.

CVM is developed under the same licence as the upstream OpenJDK project.

## Features and Benefits

Higher version of JVM brings enhancements in garbage colleciton, JIT, etc.

+ Enhanced ParallelGC and G1GC, next generation ZGC, with higher throughput, lower latency, and less memory footprints
+ Enhanced JIT compiler, support more intrinsics with faster implementation
+ Drop-in replacement for existing JDK, easy to upgrade and rollback

## Performance Results

CVM has been throughly tested on various application scenarios, including Java Microbenchmark Harness (JMH),
SPECjbb2015, Flink nexmark, etc. Compared to jdk8u372, some of the performance results are as follows:

<table border="1" cellpadding="5" cellspacing="0" style="border-collapse: collapse; text-align: center;">
  <thead>
    <tr>
      <th rowspan="2">Application</th>
      <th colspan="2">Performance Improvement</th>
    </tr>
    <tr>
      <th>x86_64</th>
      <th>aarch64</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>JMH java.util all cases average</td>
      <td>30%</td>
      <td>44%</td>
    </tr>
    <tr>
      <td>JMH java.util.stream all cases average</td>
      <td>45%</td>
      <td>56%</td>
    </tr>
    <tr>
      <td>SPECjbb2015 critical-jOPS</td>
      <td>90%</td>
      <td>35%</td>
    </tr>
    <tr>
      <td>SPECjbb2015 max-jOPS</td>
      <td>5%</td>
      <td>1%</td>
    </tr>
    <tr>
      <td>Flink nexmark all queries average</td>
      <td>10%</td>
      <td>-</td>
    </tr>
  </tbody>
</table>

## Using CVM

### Option 1: Download and install

You may download a pre-built CVM from its [release](https://github.com/bytedance/CompoundVM/releases) page, and uncompress the
package to your destination directory.

### Option 2: Build from source

Recommended GCC version: 8.x, 9.x. Run the following command:
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

## Contact Us

Scan QR code to join discussion group

![qr](cvm/conf/wechat-group.png)
