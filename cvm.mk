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
BOOTJDK25 := $(WORKSPACE)/.bootjdks/jdk-25.0.1
BOOTJDK8 := $(WORKSPACE)/.bootjdks/jdk8u452-b09
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
SCRIPTS_DIR ?= $(CVM8_SRCROOT)/scripts
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

# Download package from a given URL and extract to local directory
# $1  URL of package in .tar.gz/.zip format
# $2  local package file path to save
# $3  expected MD5 checksum of the downloaded package
# $4  directory to hold extracted content
define setup_download_artifact
	$(eval URL=$(1))
	$(eval LPATH=$(2))
	$(eval MD5_EXP=$(3))
	$(eval DIR=$(4))
	{ \
		set -x; \
		rm -rf $(DIR) && mkdir -p $(DIR); \
		for i in `seq 4`; do \
			[[ $$i -gt 1 ]] && echo "Retrying to download $(URL)"; \
			[[ ! -f $(LPATH) ]] && wget -nc -q $(URL) -O $(LPATH); \
			if [[ x$$MD5_EXP = x ]]; then break; fi; \
			MD5SUM=`md5sum $(LPATH) | awk '{print $$1}'`; \
			if [[ $$MD5SUM = $(MD5_EXP) ]]; then \
				break; \
			else \
				echo "md5 checksum of downloaded $(LPATH) is wrong! expected=$(MD5_EXP), actual=$$MD5SUM"; \
				rm -f $(LPATH); \
				continue; \
			fi; \
		done; \
		[[ -f $(LPATH) ]] || (echo "Failed to download $(URL)" && exit 127); \
		if [[ $(LPATH) == *.tar.gz ]]; then tar -xzf $(LPATH) -C $(DIR) --strip-components=1; \
		elif [[ $(LPATH) == *.zip ]]; then \
			cd $(DIR) && unzip $(LPATH) && \
			cd `zipinfo -1 $(LPATH) | grep '/$$' | sort | head -n 1` && mv * $(DIR)/; \
		fi; \
	}
endef

# '/' is indispensable otherwise target name will be treated as a file
$(BOOTJDK25)/:
	$(call setup_download_artifact, \
		"https://download.java.net/java/GA/jdk25.0.1/2fbf10d8c78e40bd87641c434705079d/8/GPL/openjdk-25.0.1_linux-x64_bin.tar.gz", \
		"$(WORKSPACE)/.bootjdks/bootjdk-25.0.1.tar.gz", \
		"287121c969b100cbccee8d6a423681a0", \
		$@)
	#cp -f $(WORKSPACE)/bin/linux-x86_64/hsdis-amd64.so $$(dirname $$(find $@ -name libjava.so))

$(BOOTJDK8)/:
	$(call setup_download_artifact, \
		"https://github.com/adoptium/temurin8-binaries/releases/download/jdk8u452-b09/OpenJDK8U-jdk_x64_linux_hotspot_8u452b09.tar.gz", \
		"$(WORKSPACE)/.bootjdks/bootjdk-8u452.tar.gz", \
		"6ad1623041892ad264125cda04e11441", \
		$@)
	#cp -f $(WORKSPACE)/bin/linux-x86_64/hsdis-amd64.so $$(dirname $$(find $@ -name libjava.so))

$(JDK8_SRCROOT)/jdk:
	$(call setup_download_artifact, \
		"https://github.com/openjdk/jdk8u/archive/refs/tags/jdk8u452-ga.tar.gz", \
		"$(CVM8_SRCROOT)/jdk8u-src.tar.gz", \
		"680255696b3a541effb8d8afd2e75e5b", \
		$(JDK8_SRCROOT))

cvm8: jdk8vm25

cvm8default25: jdk8vm25
	echo "-server25 KNOWN" > $(CVM8_LIBDIR)/jvm.cfg
	echo "-cvm KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg
	echo "-server KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg
	echo "-client IGNORE" >> $(CVM8_LIBDIR)/jvm.cfg
	cp -f $(CVM8_LIBDIR)/jvm.cfg $(OUTPUTDIR)/$(DISTRO_NAME)/jre/lib/amd64/jvm.cfg

JVM_PATCH_ARTIFACTS := jre/lib/rt25.jar jre/lib/rt8.jar jre/lib/amd64/libjava25.so jre/lib/amd64/libjimage25.so jre/lib/amd64/libjdwp25.so jre/lib/amd64/server25 jre/lib/amd64/jvm.cfg
JVM_PATCH_ARTIFACTS_SOFTLINK := jre/lib/amd64/cvm

jvm-patch: cvm8default25
	@echo "###### Composing CVM8 jvm patch ######"
	mkdir -p $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)
	for file in $(JVM_PATCH_ARTIFACTS); do \
		cd $(OUTPUTDIR)/$(DISTRO_NAME) && cp -rf --parents $$file $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)/; \
	done
	cd $(OUTPUTDIR)/$(DISTRO_NAME) && cp -a --parents $(JVM_PATCH_ARTIFACTS_SOFTLINK) $(OUTPUTDIR)/$(DISTRO_JVM_PATCH_NAME)/;

-clean-jdk8vm25:
	rm -fr $(BUILDDIR)/alt_kernel
	rm -fr $(BUILDDIR)/jdk8

clean:
	rm -fr $(BUILDDIR) $(OUTPUTDIR)
	cd $(JDK8_SRCROOT) && make clean
	cd $(JDK25_SRCROOT) && make clean

full-clean:
	rm -fr $(BUILDDIR) $(JDK25_SRCROOT)/build $(JDK8_SRCROOT)/build $(OUTPUTDIR)

define update_debug_src
	$(eval SRCZIP=$(1))
	$(eval TEMPD=$(shell mktemp -d))
	mkdir $(TEMPD)/src && unzip -q $(SRCZIP) -d $(TEMPD)/src
	cd $(CVM8_SRCROOT)/alt_kernel/src25u && find . -type f -name "*.java" -exec cp --parents {} "${TEMPD}/src" \;
	cd $(CVM8_SRCROOT)/alt_kernel/src8u && find . -type f -name "*.java" -exec cp --parents {} "${TEMPD}/src" \;
	cd $(TEMPD)/src && zip -q -r $(SRCZIP) .
	rm -rf $(TEMPD)
endef

jdk8vm25: build_jdk8u build_jdk25u altkernel
	@echo
	@echo "###### Composing CVM8 ######"
	$(eval SRC_BUILDDIR_25=$(shell find $(JDK25_SRCROOT)/build -type f -name build.log | grep $(MODE) | xargs dirname))
	$(eval SRC_BUILDDIR_8=$(shell find $(JDK8_SRCROOT)/build -type f -name build.log | grep $(MODE) | xargs dirname))
	$(eval JDK8_IMAGEDIR=$(shell find $(JDK8_SRCROOT)/build -type d -name j2sdk-image | grep $(MODE)))
	{ \
		set -x; \
		cp -Lfr $(JDK8_IMAGEDIR) $(CVM8DIR) && \
		mkdir -p $(CVM8_LIBDIR)/server25 && \
		cp -f $(BUILDDIR)/rt8.jar $(CVM8_JARDIR)/ && \
		cp -f $(BUILDDIR)/rt25.jar $(CVM8_JARDIR)/ && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/server/libjvm.so $(CVM8_LIBDIR)/server25/libjvm.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjimage.so $(CVM8_LIBDIR)/libjimage25.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjava.so $(CVM8_LIBDIR)/libjava25.so && \
		patchelf --set-soname libjava25.so $(CVM8_LIBDIR)/libjava25.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjdwp.so $(CVM8_LIBDIR)/libjdwp25.so && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjimage.debuginfo $(CVM8_LIBDIR)/libjimage25.debuginfo && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjava.debuginfo $(CVM8_LIBDIR)/libjava25.debuginfo && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/libjdwp.debuginfo $(CVM8_LIBDIR)/libjdwp25.debuginfo && \
		cp -f $(SRC_BUILDDIR_25)/jdk/lib/server/libjvm.debuginfo $(CVM8_LIBDIR)/server25/libjvm.debuginfo; \
		[[ "x$$(grep server25 $(CVM8_LIBDIR)/jvm.cfg)" = "x" ]] && echo "-server25 KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg; \
		[[ "x$$(grep cvm $(CVM8_LIBDIR)/jvm.cfg)" = "x" ]] && echo "-cvm KNOWN" >> $(CVM8_LIBDIR)/jvm.cfg; \
		pushd $(CVM8_LIBDIR) && ln -sf server25 cvm && popd; \
		cp -rf $(CVM8DIR)/* $(OUTPUTDIR)/$(DISTRO_NAME)/; \
	}
ifeq ($(MODE), release)
	# Remove unwanted files from release build
	find $(OUTPUTDIR)/$(DISTRO_NAME) -name '*.debuginfo' -execdir rm -f {} +
	find $(OUTPUTDIR)/$(DISTRO_NAME) -name '*.diz' -execdir rm -f {} +
	rm -fr $(OUTPUTDIR)/$(DISTRO_NAME)/demo
endif
	$(call update_debug_src,$(OUTPUTDIR)/$(DISTRO_NAME)/src.zip)
	@echo "###### Done ######"
	@echo

build_jdk8u: -bootstrap $(JDK8_SRCROOT)/jdk
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
		make $(JDK_MAKE_OPTS) CONF=$(MODE) images; \
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
											--with-version-build=8 \
											--without-version-pre \
											--without-version-opt \
											--with-vendor-name="CompoundVM" \
											--with-cvm-version-string=$(VERSION) \
											; \
		fi; \
	}
	make $(JDK_MAKE_OPTS) CONF=$(MODE) hotspot jdk.jdwp.agent

################ alternative kernel classes ########
# here we copy the JDK25 kernel classes to separate diretory,
# and tweak the code to fit into JDK8's boots.

altkernel: -bootstrap
	$(eval ALT_KERNEL_JAR=$(BUILDDIR)/rt25.jar)
	$(eval ALT_KERNEL_BOOT_CP=$(BOOTJDK8)/jre/lib/rt.jar)
	$(call compile_alt_classes,$(CVM8_SRCROOT)/alt_kernel/src25u,$(BUILDDIR)/alt_kernel/classes_25,$(ALT_KERNEL_JAR),$(ALT_KERNEL_BOOT_CP))
	$(eval ALT_KERNEL_JAR=$(BUILDDIR)/rt8.jar)
	$(eval ALT_KERNEL_BOOT_CP=$(BUILDDIR)/alt_kernel/classes_25:$(BOOTJDK8)/jre/lib/rt.jar)
	$(call compile_alt_classes,$(CVM8_SRCROOT)/alt_kernel/src8u,$(BUILDDIR)/alt_kernel/classes_8,$(ALT_KERNEL_JAR),$(ALT_KERNEL_BOOT_CP))

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
	$(call setup_download_artifact, \
		"https://builds.shipilev.net/jtreg/jtreg5.1-b01.zip", \
		"$(MY_JT_HOME)/jtreg.zip", \
		"", \
		"$(MY_JT_HOME)")

# minimize the effort to download source code
ifeq ($(SKIP_BUILD), true)
-setup_jtreg8: -init-dirs $(JTREG) $(JDK8_SRCROOT)/jdk
else
-setup_jtreg8: $(JTREG) jdk8vm25
endif
	$(eval JT8_OPTS=-jdk:${CVM8DIR} -w:${JT8_WORKDIR} -r:${JT8_REPORTDIR} -concurrency:auto -a -ea -esa -ignore:quiet -agentvm -v:fail,error,time -javaoption:-cvm ${JT8_OPTS})

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
	@{ cd $(CVM8_SRCROOT)/overlay/$(REPO) && cp -f --parents $(FILEPATH) $(DESTDIR)/; }
endef

JT_OPTS_EXCLUDE=-exclude:$(JDK8_SRCROOT)/jdk/test/ProblemList.txt -exclude:$(CVM8_SRCROOT)/conf/jtreg_jdk8_excludes.list

-overlay-jdk8:
	$(call overlay_single,jdk8u,jdk/test/com/sun/jdi/BreakpointWithFullGC.sh,$(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,jdk/test/com/sun/jdi/RedefineCrossEvent.java,$(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,jdk/test/java/lang/System/Versions.java,$(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,jdk/test/sun/misc/Version/Version.java,$(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,jdk/test/java/lang/ref/OOMEInReferenceHandler.java,$(JDK8_SRCROOT))

-overlay-langtools8:
	$(call overlay_single,jdk8u,langtools/test/tools/javac/annotations/8218152/MalformedAnnotationProcessorTests.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javac/6508981/TestInferBinaryName.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javac/EarlyAssertWrapper.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javadoc/6964914/TestStdDoclet.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javadoc/6964914/TestUserDoclet.java, $(JDK8_SRCROOT))
	$(call overlay_single,jdk8u,langtools/test/tools/javah/T6893943.java, $(JDK8_SRCROOT))

test_jtreg8: -setup_jtreg8 -overlay-jdk8  -overlay-langtools8
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST))

test_cvm8: -setup_jtreg8
	$(call run_jtreg8_test,$(CVM8_SRCROOT)/test,$(JT_TEST))

test_jtreg8_jdk: -setup_jtreg8 -overlay-jdk8
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_jdk_tier1: -setup_jtreg8 -overlay-jdk8
	$(eval JT_TEST = ":jdk_tier1")
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_jdk_core: -setup_jtreg8 -overlay-jdk8
	$(eval JT_TEST = ":jdk_core")
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/jdk/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_hotspot: -setup_jtreg8
	$(eval JT_REPO = hotspot)
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

test_jtreg8_langtools: -setup_jtreg8 -overlay-langtools8
	$(eval JT_REPO = langtools)
	$(call run_jtreg8_test,$(JDK8_SRCROOT)/$(JT_REPO)/test,$(JT_TEST),$(JT_OPTS_EXCLUDE))

jtreg8_gen_rerun:
	@echo "Generating re-run scripts for ${JT_TEST}"
	$(eval JTR_PATH=${JT8_WORKDIR}/$(subst .java,.jtr,${JT_TEST}))
	@{ \
    [[ "x${JT_TEST}" = "x" ]] && { echo "Please specify JT_TEST=<tests selection>"; exit 128; }; \
		[[ -f "${JTR_PATH}" ]] || { echo "${JTR_PATH} file not found, please try to reproduce first: make test_jtreg JT_TEST=<tests selection>"; exit 128; }; \
		bash ${SCRIPTS_DIR}/gen_rerun.sh ${JTR_PATH}; \
	}

gdb_jtreg8: jtreg8_gen_rerun
	bash ${JTR_PATH}.gdb.sh

jdwp_jtreg8: jtreg8_gen_rerun
	bash ${JTR_PATH}.jdwp.sh

################# Help ########################
help:
	@echo "Makefile for CVM project"
	@echo ""
	@echo "Build & Clean:"
	@echo "  make jdk8vm25      Build CVM8 with optional jvm-17"
	@echo "  make cvm8          Same as target jdk8vm25"
	@echo "  make cvm8default25 Same as target cvm8, but with jvm25 as default"
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
	@echo "Debug:"
	@echo "  make gdb_jtreg8 JT_TEST=<test result jtr>"
	@echo "                     Start GDB session to debug given CVM8 jtreg8 testcase"
	@echo "  make jdwp_jtreg8 JT_TEST=<test result jtr>"
	@echo "                     Start JDWP server to debug given CVM8 jtreg8 testcase"
	@echo "  make jtreg8_gen_rerun JT_TEST=<test selection>"
	@echo "                     Generate re-run, jdwp, and gdb scripts for the given testcase"
