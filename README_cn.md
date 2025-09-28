<div align="center">

# CompoundVM

![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/bytedance/CompoundVM/.github/workflows/main.yml?branch=jdk17u-target8) [![License](https://img.shields.io/github/license/bytedance/CompoundVM)](https://github.com/bytedance/CompoundVM/blob/main/LICENSE) [![GitHub downloads](https://img.shields.io/github/downloads/bytedance/CompoundVM/total)](https://github.com/bytedance/CompoundVM/releases)

</div>

## 项目简介

很多存量的Java业务仍然在使用老旧的 Java 版本（如 Java 8），对于这些存量业务，升级 JDK 往往需要付出较大成本。
CompoundVM (CVM) 项目旨在解决这个痛点，通过将高版本 JVM 与低版本 JDK 组合，帮助业务用最小成本获得 Java 生态最新的性能收益。

CVM 当前已经发布 JVM17 与 OpenJDK8 的组合 JDK，JVM25 与 JDK8 的组合正在开发中。
此项目目前已经在多个线上生产业务长期稳定运行。

项目基于 OpenJDK 开发，当前已支持 x86_64/aarch64 linux 平台。

## 主要特性

CVM的主要特性如下：
+ 更加完善的 Parallel GC 和 G1 GC，以及新引入的 ZGC，带来更大吞吐量，更低时延，更少内存占用
+ 性能更高的 JIT 编译器，包括更全面的intrinsic支持和高性能的实现
+ 可直接替换现有 JDK8 使用，升级过程简单可控

## 性能数据

CVM已经在多种应用场景上进行功能和性能测试，包括JMH, SPECjbb2015, Flink nexmark等。与jdk8u372相比，
一些性能数据如下：

<table border="1" cellpadding="5" cellspacing="0" style="border-collapse: collapse; text-align: center;">
  <thead>
    <tr>
      <th rowspan="2">应用场景</th>
      <th colspan="2">性能提升</th>
    </tr>
    <tr>
      <th>x86_64</th>
      <th>aarch64</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>JMH java.util 所有case平均</td>
      <td>30%</td>
      <td>44%</td>
    </tr>
    <tr>
      <td>JMH java.util.stream 所有case平均</td>
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
      <td>Flink nexmark 所有query平均</td>
      <td>10%</td>
      <td>-</td>
    </tr>
  </tbody>
</table>

## 如何使用

### 选项一：下载安装

可以直接在[release](https://github.com/bytedance/CompoundVM/releases)页面下载，解压后直接作为JDK使用。

### 选项二：从源码构建

可以从源码构建，推荐使用 gcc 8.x/9.x，运行如下命令：
`make -f cvm.mk cvm8default17`

更多选项参见`make -f cvm.mk help`


CVM安装/构建成功后，运行安装目录下的`${CVM_DIR}/bin/java -version`会输出如下信息，代表CVM已经成功使能：
```
openjdk version "1.8.0_382"
OpenJDK Runtime Environment (build 1.8.0_382-cvm-b00)
OpenJDK 64-Bit Server VM (CompoundVM 8.0.0) (build 17.0.8+0, mixed mode)
```

## 参与贡献

参见[CONTRIBUTING.md](CONTRIBUTING.md)

## 联系我们

扫码加入交流群

![qr](cvm/conf/wechat-group.png)
