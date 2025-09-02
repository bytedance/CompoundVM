# This project is a modified version of OpenJDK, licensed under GPL v2.
# Modifications Copyright (C) 2025 ByteDance Inc.
#
# This code is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 only, as
# published by the Free Software Foundation.  Oracle designates this
# particular file as subject to the "Classpath" exception as provided
# by Oracle in the LICENSE file that accompanied this code.
#
# This code is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# version 2 for more details (a copy is included in the LICENSE file that
# accompanied this code).
#
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.

WORKSPACE := $(shell pwd)
SHELL := /bin/bash
BOOTJDK25 := $(WORKSPACE)/.bootjdks/jdk-24.0.1
BOOTJDK8 := $(WORKSPACE)/.bootjdks/jdk8u372-b07
BUILDDIR := $(WORKSPACE)/cvm/build
VERSION := $(shell cat $(WORKSPACE)/cvm/conf/version)
OUTPUTDIR := $(WORKSPACE)/output
DISTRO_NAME := CompoundVM_$(VERSION)_linux_x64
DISTRO_JVM_PATCH_NAME := CompoundVM_$(VERSION)_jvm_patch_linux_x64
CVM8DIR := $(BUILDDIR)/jdk8
CVM8_JARDIR := $(CVM8DIR)/jre/lib
CVM8_LIBDIR := $(CVM8DIR)/jre/lib/amd64
MODE ?= slowdebug
JAR ?= $(BOOTJDK25)/bin/jar
JDK25_SRCROOT := $(WORKSPACE)
CVM8_SRCROOT := $(WORKSPACE)/cvm
JDK8_SRCROOT := $(CVM8_SRCROOT)/jdk8u
SRC_BUILDDIR_8 :=
SRC_BUILDDIR_25 :=
SCRIPTS_DIR ?= $(WORKSPACE)/scripts
SKIP_BUILD ?= false

# compile set of alternative kernel/application classes
# $1 source directory
# $2 output directory
# $3 jar name
# $4 boot classpath (the order matters!)
define compile_alt_classes
	$(eval ALT_CLS_SRC_DIR=$(1))
	$(eval ALT_CLS_OUT_DIR=$(2))
	$(eval ALT_CLS_JAR=$(3))
	$(eval ALT_CLS_BOOT_CLASSPATH=$(4))
	@echo Compiling source files from $(ALT_CLS_SRC_DIR) to $(ALT_CLS_OUT_DIR)

	#rm -fr $(ALT_CLS_OUT_DIR)
	[[ -d $(ALT_CLS_OUT_DIR) ]] || mkdir -p $(ALT_CLS_OUT_DIR)

	$(eval ALT_CLS_LIST=$(BUILDDIR)/alt_kernel.classlist)

	find $(ALT_CLS_SRC_DIR) -type f -name \*.java > $(ALT_CLS_LIST)
	$(BOOTJDK8)/bin/javac \
		-bootclasspath $(ALT_CLS_OUT_DIR):$(ALT_CLS_BOOT_CLASSPATH) \
		-nowarn -source 8 -target 8 -d $(ALT_CLS_OUT_DIR) @$(ALT_CLS_LIST)

	rm -f $(ALT_CLS_LIST)

	if [[ "x$(ALT_CLS_JAR)" != "x" ]]; then \
		( \
			cd $(ALT_CLS_OUT_DIR); \
			$(BOOTJDK8)/bin/jar cf ${ALT_CLS_JAR} *; \
		) \
	fi
endef

-bootstrap: -init-dirs $(BOOTJDK25)/ $(BOOTJDK8)/

-init-dirs:
	[[ -d $(BUILDDIR) ]] || mkdir -p $(BUILDDIR)
	[[ -d $(OUTPUTDIR) ]] || mkdir -p $(OUTPUTDIR)
	[[ -d $(OUTPUTDIR)/$(DISTRO_NAME) ]] || mkdir -p $(OUTPUTDIR)/$(DISTRO_NAME)
	[[ -d $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME) ]] || mkdir -p $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)

# Setup bootstrap JDK from a given URL
# $1  URL of JDK in tar.gz format
# $2  directory of JDK
define setup_boot_jdk
	$(eval DIR=$(shell dirname $(2)))
	[[ -d $(DIR) ]] || mkdir -p $(DIR)
	$(eval URL := $(1))
	$(eval TAR_FILE := $(shell basename $(URL)))
	rm -f $(TAR_FILE)
	wget -q $(URL) -O $(TAR_FILE)
	rm -fr $(2)
	tar xf $(TAR_FILE) -C .bootjdks
	rm -f $(TAR_FILE)
endef

# '/' is indispensable otherwise target name will be treated as a file
$(BOOTJDK25)/:
	$(call setup_boot_jdk,https://download.java.net/java/GA/jdk24.0.1/24a58e0e276943138bf3e963e6291ac2/9/GPL/openjdk-24.0.1_linux-x64_bin.tar.gz,$@)
	#cp -f $(WORKSPACE)/bin/linux-x86_64/hsdis-amd64.so $$(dirname $$(find $@ -name libjava.so))

$(BOOTJDK8)/:
	$(call setup_boot_jdk,https://github.com/adoptium/temurin8-binaries/releases/download/jdk8u372-b07/OpenJDK8U-jdk_x64_linux_hotspot_8u372b07.tar.gz,$@)
	#cp -f $(WORKSPACE)/bin/linux-x86_64/hsdis-amd64.so $$(dirname $$(find $@ -name libjava.so))

jdk8u/jdk/src:
	wget -nc https://github.com/openjdk/jdk8u/archive/refs/tags/jdk8u452-ga.tar.gz
	[[ -d $(JDK8_SRCROOT) ]] || (mkdir -p $(JDK8_SRCROOT) && tar -xzf jdk8u452-ga.tar.gz -C $(JDK8_SRCROOT) --strip-components=1)

cvm8: jdk8vm25

cvm8default25: jdk8vm25
	echo "-server25 KNOWN" > $(CVM8_LIBDIR)/jvm.cfg
	echo "-server KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg
	echo "-client IGNORE" >> $(CVM8_LIBDIR)/jvm.cfg
	echo "-server25 KNOWN" > $(OUTPUTDIR)/$(DISTRO_NAME)/jre/lib/amd64/jvm.cfg
	echo "-server KNOWN" >> $(OUTPUTDIR)/$(DISTRO_NAME)/jre/lib/amd64/jvm.cfg
	echo "-client IGNORE" >> $(OUTPUTDIR)/$(DISTRO_NAME)/jre/lib/amd64/jvm.cfg

JVM_PATCH_ARTIFACTS := jre/lib/rt25.jar jre/lib/rt8.jar jre/lib/amd64/libjava25.so jre/lib/amd64/libjimage25.so jre/lib/amd64/libjdwp25.so jre/lib/amd64/server25 jre/lib/amd64/jvm.cfg

jvm-patch: cvm8default25
	@echo "###### Composing CVM8 jvm patch ######"
	mkdir -p $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)
	for file in $(JVM_PATCH_ARTIFACTS); do \
		cd $(OUTPUTDIR)/$(DISTRO_NAME) && cp -rf --parents $$file $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)/; \
	done

-clean-jdk8vm25:
	rm -fr $(BUILDDIR)/alt_kernel
	rm -fr $(BUILDDIR)/jdk8

clean:
	rm -fr $(BUILDDIR)
	cd $(JDK8_SRCROOT) && make clean
	cd $(JDK25_SRCROOT) && make clean

full-clean:
	rm -fr $(BUILDDIR) $(JDK25_SRCROOT)/build $(JDK8_SRCROOT)/build

jdk8vm25: -clean-jdk8vm25 -bootstrap build_jdk8u build_jdk25u altkernel
	@echo
	@echo "###### Composing CVM8 ######"
	$(eval SRC_BUILDDIR_25=$(shell find $(JDK25_SRCROOT)/build -type f -name build.log | grep $(MODE) | xargs dirname))
	$(eval SRC_BUILDDIR_8=$(shell find $(JDK8_SRCROOT)/build -type f -name build.log | grep $(MODE) | xargs dirname))
	$(eval JDK8_IMAGEDIR=$(shell find $(JDK8_SRCROOT)/build -type d -name j2sdk-image | grep $(MODE)))
	{ \
		cp -Lfr $(JDK8_IMAGEDIR) $(CVM8DIR) && \
		mkdir -p $(CVM8_LIBDIR)/server25 && \
		cp -f $(BUILDDIR)/rt8.jar $(CVM8_JARDIR)/ && \
		cp -f $(BUILDDIR)/rt25.jar $(CVM8_JARDIR)/ && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/server/libjvm.so $(CVM8_LIBDIR)/server25/libjvm.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjimage.so $(CVM8_LIBDIR)/libjimage25.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjava.so $(CVM8_LIBDIR)/libjava25.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjdwp.so $(CVM8_LIBDIR)/libjdwp25.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjimage.debuginfo $(CVM8_LIBDIR)/libjimage25.debuginfo && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjava.debuginfo $(CVM8_LIBDIR)/libjava25.debuginfo && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjdwp.debuginfo $(CVM8_LIBDIR)/libjdwp25.debuginfo && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/server/libjvm.debuginfo $(CVM8_LIBDIR)/server25/libjvm.debuginfo && \
		[[ "x$$(grep server25 $(CVM8_LIBDIR)/jvm.cfg)" = "x" ]] && echo "-server25 KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg && \
		cp -rf $(CVM8DIR)/* $(OUTPUTDIR)/$(DISTRO_NAME)/; \
	}
ifeq ($(MODE), release)
	# Remove unwanted files from release build
	find $(OUTPUTDIR)/$(DISTRO_NAME) -name '*.debuginfo' -execdir rm -f {} +
	find $(OUTPUTDIR)/$(DISTRO_NAME) -name '*.diz' -execdir rm -f {} +
	rm -fr $(OUTPUTDIR)/$(DISTRO_NAME)/demo
endif
	@echo "###### Done ######"
	@echo

build_jdk8u: -bootstrap jdk8u/jdk/src
	{ cd $(JDK8_SRCROOT); \
		if [[ "x$$(find ./build -type f -name config.log | grep $(MODE))" = "x" ]]; then \
			bash configure --with-debug-level=$(MODE) \
											--with-boot-jdk=$(BOOTJDK8) \
											--with-milestone=fcs \
											--with-user-release-suffix="cvm" \
											--with-vendor-name="ByteDance" \
											--with-vendor-url="https://github.com/bytedance/CompoundVM" \
											--with-vendor-bug-url="https://github.com/bytedance/CompoundVM/issues" \
											--with-vendor-vm-bug-url="https://github.com/bytedance/CompoundVM/issues" \
										 ;\
		fi; \
		make $(JDK_MAKE_OPTS) CONF=linux-x86_64-normal-server-$(MODE) images; \
		[[ $$? -eq 0 ]] || exit 127; \
	}

# compile hotspot and java.base from jdk25u
build_jdk25u: -bootstrap
	{ \
		if [[ "x$$(find ./build -type f -name config.log | grep $(MODE))" = "x" ]]; then \
			bash configure --disable-warnings-as-errors \
											--with-debug-level=$(MODE) \
											--with-hotspot-target-classlib=8 \
											--with-boot-jdk=$(BOOTJDK25) \
											--with-vendor-name="ByteDance" \
											--with-vendor-url="https://github.com/bytedance/CompoundVM" \
											--with-vendor-bug-url="https://github.com/bytedance/CompoundVM/issues" \
											--with-vendor-vm-bug-url="https://github.com/bytedance/CompoundVM/issues" \
											--without-version-pre \
											--without-version-opt \
											--with-vendor-name="CompoundVM" \
											; \
		fi; \
	}
	make $(JDK_MAKE_OPTS) CONF=linux-x86_64-server-$(MODE) hotspot jdk.jdwp.agent

################ alternative kernel classes ########
# here we copy the JDK25 kernel classes to separate diretory,
# and tweak the code to fit into JDK8's boots.

altkernel: -bootstrap
	$(eval ALT_KERNEL_JAR=$(BUILDDIR)/rt8.jar)
	$(eval ALT_KERNEL_BOOT_CP=$(BOOTJDK8)/jre/lib/rt.jar)
	$(call compile_alt_classes,$(CVM8_SRCROOT)/alt_kernel/src8u,$(BUILDDIR)/alt_kernel/classes_8,$(ALT_KERNEL_JAR),$(ALT_KERNEL_BOOT_CP))
	$(eval ALT_KERNEL_JAR=$(BUILDDIR)/rt25.jar)
	$(eval ALT_KERNEL_BOOT_CP=$(BUILDDIR)/alt_kernel/classes_25:$(BOOTJDK8)/jre/lib/rt.jar)
	$(call compile_alt_classes,$(CVM8_SRCROOT)/alt_kernel/src25u,$(BUILDDIR)/alt_kernel/classes_25,$(ALT_KERNEL_JAR),$(ALT_KERNEL_BOOT_CP))
