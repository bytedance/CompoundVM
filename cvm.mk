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
BOOTJDK17 := $(WORKSPACE)/.bootjdks/jdk-17.0.7+7
BOOTJDK8 := $(WORKSPACE)/.bootjdks/jdk8u372-b07
BUILDDIR := $(WORKSPACE)/cvm/build
OUTPUTDIR := $(WORKSPACE)/output
CVM8DIR := $(BUILDDIR)/jdk8
CVM8_JARDIR := $(CVM8DIR)/jre/lib
CVM8_LIBDIR := $(CVM8DIR)/jre/lib/amd64
MODE ?= release
JAR ?= $(BOOTJDK17)/bin/jar
JDK17_SRCROOT := $(WORKSPACE)
CVM8_SRCROOT := $(WORKSPACE)/cvm
JDK8_SRCROOT := $(CVM8_SRCROOT)/jdk8u
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
	  -L$(JDK8_LIB_DIR)/amd64/jli \
	  -Wl,-rpath,'$$ORIGIN/../lib/amd64/jli' \
	  -ljli
endef

-bootstrap: -init-dirs $(BOOTJDK17)/ $(BOOTJDK8)/

-init-dirs:
	[[ -d $(BUILDDIR) ]] || mkdir -p $(BUILDDIR)
	[[ -d $(OUTPUTDIR) ]] || mkdir -p $(OUTPUTDIR)

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
	$(call setup_boot_jdk,https://github.com/adoptium/temurin17-binaries/releases/download/jdk-17.0.7%2B7/OpenJDK17U-jdk_x64_linux_hotspot_17.0.7_7.tar.gz,$@)
	#cp -f $(WORKSPACE)/bin/linux-x86_64/hsdis-amd64.so $$(dirname $$(find $@ -name libjava.so))

$(BOOTJDK8)/:
	$(call setup_boot_jdk,https://github.com/adoptium/temurin8-binaries/releases/download/jdk8u372-b07/OpenJDK8U-jdk_x64_linux_hotspot_8u372b07.tar.gz,$@)
	#cp -f $(WORKSPACE)/bin/linux-x86_64/hsdis-amd64.so $$(dirname $$(find $@ -name libjava.so))

jdk8u/jdk/src:
	wget -nc https://github.com/openjdk/jdk8u/archive/refs/tags/jdk8u382-b03.tar.gz
	[[ -d $(JDK8_SRCROOT) ]] || (mkdir -p $(JDK8_SRCROOT) && tar -xzf jdk8u382-b03.tar.gz -C $(JDK8_SRCROOT) --strip-components=1)

cvm8: jdk8vm17

cvm8default17: jdk8vm17
	echo "-server17 KNOWN" > $(CVM8_LIBDIR)//jvm.cfg
	echo "-server KNOWN" >> $(CVM8_LIBDIR)//jvm.cfg
	echo "-client IGNORE" >> $(CVM8_LIBDIR)//jvm.cfg
	echo "-server17 KNOWN" > $(OUTPUTDIR)/jdk8/jre/lib/amd64//jvm.cfg
	echo "-server KNOWN" >> $(OUTPUTDIR)/jdk8/jre/lib/amd64/jvm.cfg
	echo "-client IGNORE" >> $(OUTPUTDIR)/jdk8/jre/lib/amd64//jvm.cfg

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
		cp -Lfr $(JDK8_IMAGEDIR) $(CVM8DIR); \
		cp -f $(SRC_BUILDDIR_8)/jdk/objs/libjli/libjli.debuginfo $(CVM8_LIBDIR)/jli/; \
		cp -f $(BUILDDIR)/rt17.jar $(CVM8_JARDIR)/; \
		cp -f $(BUILDDIR)/rt8.jar $(CVM8_JARDIR)/; \
		cp -f $(BUILDDIR)/tools17.jar $(CVM8DIR)/lib/; \
		cp -f $(BUILDDIR)/bin/* $(CVM8DIR)/bin/; \
		mkdir -p $(CVM8_LIBDIR)/server17; \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/server/libjvm.so $(CVM8_LIBDIR)/server17/libjvm.so; \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjimage.so $(CVM8_LIBDIR)/libjimage17.so; \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjava.so $(CVM8_LIBDIR)/libjava17.so; \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjdwp.so $(CVM8_LIBDIR)/libjdwp17.so; \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjava.debuginfo $(CVM8_LIBDIR)/libjava17.debuginfo; \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/libjimage.debuginfo $(CVM8_LIBDIR)/libjimage17.debuginfo; \
		cp -f $(SRC_BUILDDIR_17)/jdk/lib/server/libjvm.debuginfo $(CVM8_LIBDIR)/server17/libjvm.debuginfo; \
		[[ "x$$(grep server17 $(CVM8_LIBDIR)/jvm.cfg)" = "x" ]] && echo "-server17 KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg; \
		cp -rf $(CVM8DIR) $(OUTPUTDIR)/; \
	}
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

# compile hotspot and java.base from jdk17u
build_jdk17u: -bootstrap
	{ \
		if [[ "x$$(find ./build -type f -name config.log | grep $(MODE))" = "x" ]]; then \
			bash configure --with-debug-level=$(MODE) \
											--with-boot-jdk=$(BOOTJDK17) \
											--with-hotspot-target-classlib=8 \
											--with-vendor-name="ByteDance" \
											--with-vendor-url="https://github.com/bytedance/CompoundVM" \
											--with-vendor-bug-url="https://github.com/bytedance/CompoundVM/issues" \
											--with-vendor-vm-bug-url="https://github.com/bytedance/CompoundVM/issues" \
											--without-version-pre \
											--without-version-opt \
											--with-cvm-version-string="8.0.0" \
											--with-vendor-name="CompoundVM" \
											; \
		fi; \
	}
	make $(JDK_MAKE_OPTS) CONF=linux-x86_64-server-$(MODE) hotspot jdk.jdwp.agent

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
-setup_jtreg8: -init-dirs $(JTREG) jdk8u/jdk/src
else
-setup_jtreg8: $(JTREG) jdk8vm17
endif
$(eval JT8_OPTS=-jdk:${CVM8DIR} -w:${JT8_WORKDIR} -r:${JT8_REPORTDIR} -a -ea -esa -ignore:quiet -ovm -v:fail,error,time -javaoption:-server17 -javaoptions:-Djdk.test.docker.image.name=ubuntu -javaoptions:-Djdk.test.docker.image.version=latest ${JT8_OPTS})

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

JT_OPTS_EXCLUDE=-exclude:$(JDK8_SRCROOT)/jdk/test/ProblemList.txt -exclude:$(CVM8_SRCROOT)/conf/jtreg_jdk8_excludes.list

test_jtreg8: -setup_jtreg8
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST))

test_cvm8: -setup_jtreg8
	$(call run_jtreg8_test,$(CVM8_SRCROOT)/test,$(JT_TEST))

test_jtreg8_jdk: -setup_jtreg8
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_jdk_tier1: -setup_jtreg8
	$(eval JT_TEST = ":jdk_tier1")
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_jdk_core: -setup_jtreg8
	$(eval JT_TEST = ":jdk_core")
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_hotspot: -setup_jtreg8
	$(eval JT_REPO = hotspot)
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_langtools: -setup_jtreg8
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
	@echo "                     Run CVM8 jtreg8 tests in directory jdk8u/jdk/test"
	@echo "  make test_jtreg8_langtools JT_TEST=<test selection>"
	@echo "                     Run CVM8 jtreg8 tests in directory jdk8u/langtools/test"
	@echo "  make test_jtreg8_hotspot JT_TEST=<test selection>"
	@echo "                     Run CVM8 jtreg8 tests in directory jdk8u/hotspot/test"
	@echo "  make test_cvm8 JT_TEST=<test selection>"
	@echo "                     Run additional jtreg8 tests for CVM8 in directory test"
