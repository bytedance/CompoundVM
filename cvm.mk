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

CVM_ARCH := $(shell uname -m)
WORKSPACE := $(shell pwd)
SHELL := /bin/bash
BOOTJDK17 := $(WORKSPACE)/.bootjdks/jdk-17.0.7+7
BOOTJDK8 := $(WORKSPACE)/.bootjdks/jdk8u372-b07
# Variable ARCH conflicts with jdk8's build variable
ifeq ($(CVM_ARCH),x86_64)
	BOOTJDK17_URL := https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.7%2B7/OpenJDK17U-jdk_x64_linux_hotspot_17.0.7_7.tar.gz
	BOOTJDK8_URL := https://github.com/adoptium/temurin8-binaries/releases/download/jdk8u372-b07/OpenJDK8U-jdk_x64_linux_hotspot_8u372b07.tar.gz
	ARCH_DIR := amd64
	ARCH_DIR1 := x64
else ifeq ($(CVM_ARCH),aarch64)
	BOOTJDK17_URL := https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.7%2B7/OpenJDK17U-jdk_aarch64_linux_hotspot_17.0.7_7.tar.gz
	BOOTJDK8_URL := https://github.com/adoptium/temurin8-binaries/releases/download/jdk8u372-b07/OpenJDK8U-jdk_aarch64_linux_hotspot_8u372b07.tar.gz
	ARCH_DIR := aarch64
	ARCH_DIR1 := aarch64
else
	ARCH_ERROR := 1
endif
BUILDDIR := $(WORKSPACE)/cvm/build
VERSION := $(shell cat $(WORKSPACE)/cvm/conf/version)
OUTPUTDIR := $(WORKSPACE)/output
DISTRO_NAME := CompoundVM_$(VERSION)_linux_$(ARCH_DIR1)
DISTRO_JVM_PATCH_NAME := CompoundVM_$(VERSION)_jvm_patch_linux_$(ARCH_DIR1)
CVM8DIR := $(BUILDDIR)/jdk8
CVM8_JARDIR := $(CVM8DIR)/jre/lib
CVM8_LIBDIR := $(CVM8DIR)/jre/lib/$(ARCH_DIR)
MODE ?= release
JAR ?= $(BOOTJDK17)/bin/jar
JDK17_SRCROOT := $(WORKSPACE)
CVM8_SRCROOT := $(WORKSPACE)/cvm
JDK8_SRCROOT := $(CVM8_SRCROOT)/jdk8u

# Debug-specific jtreg options
ifeq ($(filter fastdebug slowdebug,$(MODE)),)
  JT8_DEBUG_OPTS :=
else
  JT8_DEBUG_OPTS := -exclude:$(CVM8_SRCROOT)/conf/jtreg-debug.list -timeout:4
endif
SRC_BUILDDIR_8 :=
SRC_BUILDDIR_17 :=
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

# build diagnosis tool executables for vm17
# $1 lib directory of jdk8
# $2 tool name
# $3 main class of tool
define compile_tools17_bin
	$(eval JDK8_LIB_DIR=$(1))
	$(eval TOOL_NAME=$(2))
	$(eval TOOL_MAIN_CLASS=$(3))
	@echo Compiling diagnosis tool $(TOOL_NAME)
	[[ -d $(BUILDDIR)/bin/ ]] || mkdir -p $(BUILDDIR)/bin/

	gcc -O2 -pie -fPIE\
	  -DJAVA_ARGS='{ "-J-ms8m", "$(TOOL_MAIN_CLASS)", }' \
	  -DAPP_CLASSPATH='{ "/lib/tools17.jar", "/lib/tools.jar", }' \
	  -o $(BUILDDIR)/bin/$(TOOL_NAME) \
	  $(CVM8_SRCROOT)/alt_app/tools17/src/share/bin/tool.c \
	  -L$(JDK8_LIB_DIR)/$(ARCH_DIR)/jli \
	  -Wl,-rpath,'$$ORIGIN/../lib/$(ARCH_DIR)/jli' \
	  -ljli
endef

-bootstrap: -check-arch -init-dirs $(BOOTJDK17)/ $(BOOTJDK8)/

-check-arch:
	if [ "$(ARCH_ERROR)" = "1" ]; then \
		echo "Unsupported architecture! only x86_64 and aarch64 are supported."; \
		exit 1; \
	fi

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
$(BOOTJDK17)/:
	$(call setup_boot_jdk,$(BOOTJDK17_URL),$@)
	#cp -f $(WORKSPACE)/bin/linux-$(CVM_ARCH)/hsdis-$(ARCH_DIR).so $$(dirname $$(find $@ -name libjava.so))

$(BOOTJDK8)/:
	$(call setup_boot_jdk,$(BOOTJDK8_URL),$@)
	#cp -f $(WORKSPACE)/bin/linux-$(CVM_ARCH)/hsdis-$(ARCH_DIR).so $$(dirname $$(find $@ -name libjava.so))

JDK8_JDK_SRC := $(JDK8_SRCROOT)/jdk/src
JDK8_SRC_TAR_URL := https://github.com/openjdk/jdk8u/archive/refs/tags/jdk8u452-ga.tar.gz
JDK8_SRC_TAR := $(notdir $(JDK8_SRC_TAR_URL))

$(JDK8_JDK_SRC):
	rm -f $(JDK8_SRC_TAR)
	wget -nc -O $(JDK8_SRC_TAR) $(JDK8_SRC_TAR_URL)
	[[ -d $(JDK8_SRCROOT) ]] || (mkdir -p $(JDK8_SRCROOT) && tar -xzf $(JDK8_SRC_TAR) -C $(JDK8_SRCROOT) --strip-components=1)

cvm8: jdk8vm17

cvm8default17: jdk8vm17
	echo "-server17 KNOWN" > $(CVM8_LIBDIR)/jvm.cfg
	echo "-cvm KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg
	echo "-server KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg
	echo "-client IGNORE" >> $(CVM8_LIBDIR)/jvm.cfg
	cp -f $(CVM8_LIBDIR)/jvm.cfg $(OUTPUTDIR)/$(DISTRO_NAME)/jre/lib/$(ARCH_DIR)/jvm.cfg

JVM_PATCH_ARTIFACTS := jre/lib/rt17.jar jre/lib/rt8.jar jre/lib/$(ARCH_DIR)/libjava17.so jre/lib/$(ARCH_DIR)/libjimage17.so jre/lib/$(ARCH_DIR)/libjdwp17.so jre/lib/$(ARCH_DIR)/server17 jre/lib/$(ARCH_DIR)/jvm.cfg
JVM_PATCH_ARTIFACTS_SOFTLINK := jre/lib/$(ARCH_DIR)/cvm

jvm-patch: cvm8default17
	@echo "###### Composing CVM8 jvm patch ######"
	mkdir -p $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)
	for file in $(JVM_PATCH_ARTIFACTS); do \
		cd $(OUTPUTDIR)/$(DISTRO_NAME) && cp -rf --parents $$file $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)/; \
	done
	cd $(OUTPUTDIR)/$(DISTRO_NAME) && cp -a --parents $(JVM_PATCH_ARTIFACTS_SOFTLINK) $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)/;

-clean-jdk8vm17:
	rm -fr $(BUILDDIR)/alt_kernel
	rm -fr $(BUILDDIR)/jdk8

clean:
	rm -fr $(BUILDDIR)
	cd $(JDK8_SRCROOT) && make clean
	cd $(JDK17_SRCROOT) && make clean

full-clean:
	rm -fr $(BUILDDIR) $(JDK17_SRCROOT)/build $(JDK8_SRCROOT)/build

jdk8vm17: -clean-jdk8vm17 -bootstrap build_jdk8u build_jdk17u altkernel
	@echo
	@echo "###### Composing CVM8 ######"
	$(eval SRC_BUILDDIR_17=$(shell find $(JDK17_SRCROOT)/build -type f -name build.log | grep $(MODE) | xargs dirname))
	$(eval SRC_BUILDDIR_8=$(shell find $(JDK8_SRCROOT)/build -type f -name build.log | grep $(MODE) | xargs dirname))
	$(eval JDK8_IMAGEDIR=$(shell find $(JDK8_SRCROOT)/build -type d -name j2sdk-image | grep $(MODE)))
	{ \
		cp -Lfr $(JDK8_IMAGEDIR) $(CVM8DIR) && \
		cp -f $(BUILDDIR)/rt17.jar $(CVM8_JARDIR)/ && \
		cp -f $(BUILDDIR)/rt8.jar $(CVM8_JARDIR)/ && \
		cp -f $(BUILDDIR)/tools17.jar $(CVM8DIR)/lib/ && \
		cp -f $(BUILDDIR)/bin/* $(CVM8DIR)/bin/ && \
		mkdir -p $(CVM8_LIBDIR)/server17 && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/server/libjvm.so $(CVM8_LIBDIR)/server17/libjvm.so && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjimage.so $(CVM8_LIBDIR)/libjimage17.so && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjava.so $(CVM8_LIBDIR)/libjava17.so && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjdwp.so $(CVM8_LIBDIR)/libjdwp17.so && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjimage.debuginfo $(CVM8_LIBDIR)/libjimage17.debuginfo && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjava.debuginfo $(CVM8_LIBDIR)/libjava17.debuginfo && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjdwp.debuginfo $(CVM8_LIBDIR)/libjdwp17.debuginfo && \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/server/libjvm.debuginfo $(CVM8_LIBDIR)/server17/libjvm.debuginfo && \
		[[ "x$$(grep server17 $(CVM8_LIBDIR)/jvm.cfg)" = "x" ]] && echo "-server17 KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg && \
		[[ "x$$(grep cvm $(CVM8_LIBDIR)/jvm.cfg)" = "x" ]] && echo "-cvm KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg; \
		pushd $(CVM8_LIBDIR) && ln -sf server17 cvm && popd; \
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

build_jdk8u: -bootstrap $(JDK8_JDK_SRC)
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
		make $(JDK_MAKE_OPTS) CONF=linux-$(CVM_ARCH)-normal-server-$(MODE) images; \
		[[ $$? -eq 0 ]] || exit 127; \
	}

# configure for jdk17u
# $1 extra configure options
define configure_jdk17u
	bash configure \
		--with-debug-level=$(MODE) \
		--with-boot-jdk=$(BOOTJDK17) \
		--with-hotspot-target-classlib=8 \
		--with-vendor-name="ByteDance" \
		--with-vendor-url="https://github.com/bytedance/CompoundVM" \
		--with-vendor-bug-url="https://github.com/bytedance/CompoundVM/issues" \
		--with-vendor-vm-bug-url="https://github.com/bytedance/CompoundVM/issues" \
		--without-version-pre \
		--without-version-opt \
		--with-cvm-version-string=$(VERSION) \
		--with-vendor-name="CompoundVM" \
		$(1)
endef

# compile hotspot and java.base from jdk17u
build_jdk17u: -bootstrap
	{ \
		if [[ "x$$(find ./build -type f -name config.log | grep $(MODE))" = "x" ]]; then \
			$(call configure_jdk17u) ; \
		fi; \
	}
	make $(JDK_MAKE_OPTS) CONF=linux-$(CVM_ARCH)-server-$(MODE) hotspot jdk.jdwp.agent

GTEST_VERSION ?= $(shell sed -n 's/^GTEST_VERSION=//p' $(WORKSPACE)/make/conf/github-actions.conf | head -n 1)
GTEST_ROOT := $(WORKSPACE)/.gtest
GTEST_ARCHIVE := $(GTEST_ROOT)/release-$(GTEST_VERSION).tar.gz
GTEST_URL := https://github.com/google/googletest/archive/refs/tags/v$(GTEST_VERSION).tar.gz
GTEST_DIR := $(GTEST_ROOT)/googletest-$(GTEST_VERSION)
GTEST_HEADER := $(GTEST_DIR)/googletest/include/gtest/gtest.h

$(GTEST_HEADER):
	@echo "Installing googletest $(GTEST_VERSION) to $(GTEST_DIR)"
	{ \
		set -e; \
		[[ -d $(GTEST_ROOT) ]] || mkdir -p $(GTEST_ROOT); \
		rm -f $(GTEST_ARCHIVE); \
		wget -q $(GTEST_URL) -O $(GTEST_ARCHIVE); \
		rm -fr $(GTEST_DIR); \
		tar xf $(GTEST_ARCHIVE) -C $(GTEST_ROOT); \
		rm -f $(GTEST_ARCHIVE); \
	}

HOTSPOT_GTEST_BUILD_DIR := $(JDK17_SRCROOT)/build/linux-$(CVM_ARCH)-server-$(MODE)
HOTSPOT_GTEST_LAUNCHER := $(HOTSPOT_GTEST_BUILD_DIR)/images/test/hotspot/gtest/server/gtestLauncher
HOTSPOT_GTEST_TEST ?= gtest:all
HOTSPOT_GTEST_SELECTOR := $(patsubst %/server,%,$(patsubst gtest:%,%,$(HOTSPOT_GTEST_TEST)))
HOTSPOT_GTEST_FILTER := $(if $(filter all,$(HOTSPOT_GTEST_SELECTOR)),,--gtest_filter=$(HOTSPOT_GTEST_SELECTOR)*)
HOTSPOT_GTEST_JDKDIR := $(OUTPUTDIR)/$(DISTRO_NAME)
HOTSPOT_GTEST_WORKDIR := $(HOTSPOT_GTEST_BUILD_DIR)/gtest-manual
HOTSPOT_GTEST_RESULT_DIR := $(HOTSPOT_GTEST_BUILD_DIR)/gtest-results

ifeq ($(SKIP_BUILD), true)
-configure_jdk17u_gtest: $(GTEST_HEADER) -bootstrap
else
-configure_jdk17u_gtest: $(GTEST_HEADER) cvm8default17
endif
	{ \
		if [[ ! -f $(HOTSPOT_GTEST_BUILD_DIR)/spec.gmk ]] || \
				! grep -Fqx 'GTEST_FRAMEWORK_SRC := $(GTEST_DIR)' $(HOTSPOT_GTEST_BUILD_DIR)/spec.gmk 2>/dev/null; then \
			$(call configure_jdk17u,--with-gtest=$(GTEST_DIR)) ; \
		fi; \
	}

build_gtest_hotspot17: -configure_jdk17u_gtest
	$(MAKE) $(JDK_MAKE_OPTS) CONF=linux-$(CVM_ARCH)-server-$(MODE) test-image-hotspot-gtest

ifeq ($(SKIP_BUILD), true)
test_gtest_hotspot17:
else
test_gtest_hotspot17: build_gtest_hotspot17
endif
	@echo
	@echo "Running hotspot gtest \"$(HOTSPOT_GTEST_TEST)\""
	@echo "  Launcher: $(HOTSPOT_GTEST_LAUNCHER)"
	@echo "  JDK under test: $(HOTSPOT_GTEST_JDKDIR)"
	@echo "  Work dir: $(HOTSPOT_GTEST_WORKDIR)"
	@echo "  Test report: $(HOTSPOT_GTEST_RESULT_DIR)"
	@echo
	@{ \
		mkdir -p $(HOTSPOT_GTEST_WORKDIR) $(HOTSPOT_GTEST_RESULT_DIR) && \
		cd $(HOTSPOT_GTEST_WORKDIR) && \
		$(HOTSPOT_GTEST_LAUNCHER) \
			-jdk $(HOTSPOT_GTEST_JDKDIR) \
			$(HOTSPOT_GTEST_FILTER) \
			--gtest_output=xml:$(HOTSPOT_GTEST_RESULT_DIR)/gtest.xml \
			--gtest_catch_exceptions=0 \
			> >(tee $(HOTSPOT_GTEST_RESULT_DIR)/gtest.txt); \
		exit_code=$$?; \
		echo $$exit_code > $(HOTSPOT_GTEST_RESULT_DIR)/exitcode.txt; \
		exit $$exit_code; \
	}

################ alternative kernel classes ########
# here we copy the JDK17 kernel classes to separate diretory,
# and tweak the code to fit into JDK8's boots.

altkernel: -bootstrap -tools17_jar -tools17_bin 
	$(eval ALT_KERNEL_JAR=$(BUILDDIR)/rt17.jar)
	$(eval ALT_KERNEL_BOOT_CP=$(BOOTJDK8)/jre/lib/rt.jar)
	$(call compile_alt_classes,$(CVM8_SRCROOT)/alt_kernel/src17u,$(BUILDDIR)/alt_kernel/classes_17,$(ALT_KERNEL_JAR),$(ALT_KERNEL_BOOT_CP))
	$(eval ALT_KERNEL_JAR=$(BUILDDIR)/rt8.jar)
	$(eval ALT_KERNEL_BOOT_CP=$(BUILDDIR)/alt_kernel/classes_17:$(BOOTJDK8)/jre/lib/rt.jar)
	$(call compile_alt_classes,$(CVM8_SRCROOT)/alt_kernel/src8u,$(BUILDDIR)/alt_kernel/classes_8,$(ALT_KERNEL_JAR),$(ALT_KERNEL_BOOT_CP))

-tools17_jar: $(BOOTJDK8)/
	$(eval TOOLS17_JAR=$(BUILDDIR)/tools17.jar)
	$(call compile_alt_classes,$(CVM8_SRCROOT)/alt_app/tools17/src,$(BUILDDIR)/tools17/classes,$(TOOLS17_JAR),$(BOOTJDK8)/jre/lib/rt.jar:$(BOOTJDK8)/lib/tools.jar)

-tools17_bin: $(BOOTJDK8)/
	$(call compile_tools17_bin,$(BOOTJDK8)/lib,jinfo17,sun.tools.jinfo.JInfo17)
	$(call compile_tools17_bin,$(BOOTJDK8)/lib,jstack17,sun.tools.jstack.JStack17)
	$(call compile_tools17_bin,$(BOOTJDK8)/lib,jmap17,sun.tools.jmap.JMap17)

############### Test ##################

JT8_WORKDIR=${BUILDDIR}/jtreg8/JTwork
JT8_REPORTDIR=${BUILDDIR}/jtreg8/JTreport
JT8_RERUNDIR=${BUILDDIR}/jtreg8/rerun
JT_TEST ?= .
JT_REPO ?= jdk

# using local JTreg installation instead of system's
MY_JT_HOME := $(WORKSPACE)/.jtreg
JTREG := $(MY_JT_HOME)/bin/jtreg

$(JTREG):
	$(eval JTREG_URL := https://builds.shipilev.net/jtreg/jtreg5.1-b01.zip)
	$(eval JTREG_ZIP := $(shell basename $(JTREG_URL)))
	@echo "Installing jtreg5.1 to $(MY_JT_HOME)"
	{ \
		rm -f $(JTREG_ZIP);\
		wget -q $(JTREG_URL); \
		unzip -o -q $(JTREG_ZIP) && mv jtreg .jtreg && rm -fr $(JTREG_ZIP); \
	}

# minimize the effort to download source code
ifeq ($(SKIP_BUILD), true)
-setup_jtreg8: -init-dirs $(JTREG) $(JDK8_JDK_SRC)
else
-setup_jtreg8: $(JTREG) cvm8default17
endif
	$(eval JT8_OPTS=-jdk:${CVM8DIR} -w:${JT8_WORKDIR} -r:${JT8_REPORTDIR} -concurrency:auto -a -ea -esa -ignore:quiet -agentvm -v:fail,error,time -javaoption:-cvm $(JT8_DEBUG_OPTS) ${JT8_OPTS})

# Setup bootstrap JDK from a given URL
# $1  root directory of jtreg
# $2  pattern to match testcase names
define run_jtreg8_test
	$(eval JT8_DIR = $(1))
	$(eval JT_TEST = $(2))
	$(eval JT_EXTRA_OPTS = $(3))
	$(eval CUR_CMD=JTREG_JAVA=${CVM8DIR}/bin/java $(JTREG) ${JT8_OPTS} ${JT_EXTRA_OPTS} ${JT_TEST})
	@echo
	@echo "Running JTreg8 \"${JT_TEST}\" in dir ${JT8_DIR}"
	@echo "  Report directory: ${JT8_REPORTDIR}"
	@echo "  Working directory: ${JT8_WORKDIR}"
	@echo "  Command: ${CUR_CMD}"
	@echo
	@{ cd ${JT8_DIR} && ${CUR_CMD}; }
endef

# Overwrite upstream source file with the modified version shipped in CompoundVM repo
# $1   repository name from within cvm/overlay
# $2   filepath relative to $1
# $3   destination repo directory
define overlay_single
	$(eval REPO=$(1))
	$(eval FILEPATH=$(2))
	$(eval DESTDIR=$(3))
	@{ test -e $(DESTDIR)/$(FILEPATH)_origin || cp -f $(DESTDIR)/$(FILEPATH) $(DESTDIR)/$(FILEPATH)_origin; }
	@{ cd cvm/overlay/$(REPO) && cp -f --parents $(FILEPATH) $(DESTDIR)/; }
endef

ifeq ($(CVM_ARCH),x86_64)
	JT_OPTS_EXCLUDE=-exclude:$(JDK8_SRCROOT)/$(JT_REPO)/test/ProblemList.txt -exclude:$(CVM8_SRCROOT)/conf/jtreg_jdk8_excludes.list -exclude:$(CVM8_SRCROOT)/conf/jtreg_hotspot8_excludes_x64.list
else ifeq ($(CVM_ARCH),aarch64)
	JT_OPTS_EXCLUDE=-exclude:$(JDK8_SRCROOT)/$(JT_REPO)/test/ProblemList.txt -exclude:$(CVM8_SRCROOT)/conf/jtreg_jdk8_excludes.list -exclude:$(CVM8_SRCROOT)/conf/jtreg_hotspot8_excludes_aarch64.list
else
	ARCH_ERROR := 1
endif

-overlay-jdk8:
	$(call overlay_single,jdk8u,jdk/test/com/sun/jdi/BreakpointWithFullGC.sh,$(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,jdk/test/com/sun/jdi/RedefineCrossEvent.java,$(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,jdk/test/java/lang/System/Versions.java,$(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,jdk/test/sun/misc/Version/Version.java,$(JDK8_SRCROOT))

-overlay-langtools8:
	$(call overlay_single,jdk8u,langtools/test/tools/javac/annotations/8218152/MalformedAnnotationProcessorTests.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javac/6508981/TestInferBinaryName.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javac/api/T6430241.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javac/diags/CheckExamples.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javac/T6873845.java, $(JDK8_SRCROOT))

-overlay-hotspot8:
	$(call overlay_single,jdk8u,hotspot/test/testlibrary/com/oracle/java/testlibrary/Platform.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/testlibrary/com/oracle/java/testlibrary/cli/CommandLineOptionTest.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/compiler/6859338/Test6859338.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/compiler/7196199/Test7196199.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/compiler/8004741/Test8004741.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/compiler/stable/StableConfiguration.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/testlibrary/whitebox/sun/hotspot/code/NMethod.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/arguments/TestAggressiveHeap.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/arguments/TestG1ConcRefinementThreads.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/arguments/TestG1HeapRegionSize.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/arguments/TestHeapFreeRatio.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/arguments/TestInitialTenuringThreshold.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/arguments/TestUnrecognizedVMOptionsHandling.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/arguments/TestUseCompressedOopsErgoTools.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/testlibrary/whitebox/sun/hotspot/WhiteBox.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/gc/startup_warnings/TestDefaultMaxRAMFraction.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/compiler/whitebox/IsMethodCompilableTest.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,hotspot/test/runtime/ErrorHandling/ErrorFileRedirectTest.java, $(JDK8_SRCROOT))

-overlay-jtreg:
	$(call overlay_single,jdk8u,test/jtreg-ext/requires/VMProps.java, $(JDK8_SRCROOT))

test_jtreg8: -setup_jtreg8 -overlay-jdk8  -overlay-langtools8 -overlay-jtreg
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST))

test_jtreg8_cvm8: -setup_jtreg8
	$(call run_jtreg8_test,$(CVM8_SRCROOT)/test,$(JT_TEST))

test_jtreg8_jdk: -setup_jtreg8 -overlay-jdk8
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_jdk_tier1: -setup_jtreg8 -overlay-jdk8
	$(eval JT_TEST = ":jdk_tier1")
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_jdk_core: -setup_jtreg8 -overlay-jdk8
	$(eval JT_TEST = ":jdk_core")
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_hotspot8: -setup_jtreg8 -overlay-jtreg -overlay-hotspot8
	$(eval JT_REPO = hotspot)
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_langtools: -setup_jtreg8 -overlay-langtools8
	$(eval JT_REPO = langtools)
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

################# Help ########################
help:
	@echo "Makefile for CVM project"
	@echo ""
	@echo "Build & Clean:"
	@echo "  make jdk8vm17      Build CVM8 with optional jvm-17"
	@echo "  make cvm8          Same as target jdk8vm17"
	@echo "  make cvm8default17 Same as target cvm8, but with jvm17 as default"
	@echo "  make full-clean    Delete all artifacts, including sub-modules"
	@echo "  make clean         Delete artifacts from directory build/"
	@echo ""
	@echo "Test:"
	@echo "  make test_jtreg8 JT_TEST=<test selection> JT_REPO=<repo dir>"
	@echo "                     Run CVM8 jtreg8 test with given selection"
	@echo "  make test_jtreg8_jdk JT_TEST=<test selection>"
	@echo "                     Run CVM8 jtreg8 tests in directory $(CVM8_SRCROOT)/jdk/test"
	@echo "  make test_jtreg8_langtools JT_TEST=<test selection>"
	@echo "                     Run CVM8 jtreg8 tests in directory $(CVM8_SRCROOT)/langtools/test"
	@echo "  make test_jtreg8_hotspot8 JT_TEST=<test selection>"
	@echo "                     Run CVM8 jtreg8 tests in directory $(CVM8_SRCROOT)/hotspot/test"
	@echo "  make test_jtreg8_cvm8 JT_TEST=<test selection>"
	@echo "                     Run additional jtreg8 tests for CVM8 in directory $(CVM8_SRCROOT)/test"
	@echo "  make test_gtest_hotspot17 HOTSPOT_GTEST_TEST=<gtest selection>"
	@echo "                     Run hotspot gtests directly against the CVM output image"
	@echo "                     e.g. HOTSPOT_GTEST_TEST='gtest:all'"
