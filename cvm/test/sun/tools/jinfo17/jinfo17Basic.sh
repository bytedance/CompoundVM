#!/bin/sh

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

# @test
# @bug 1234567
# @summary Unit test for jinfo17 utility
#
# @library ../common
# @build Loop
# @run shell jinfo17Basic.sh

. ${TESTSRC}/../common/ApplicationSetup.sh

JINFO17="${TESTJAVA}/bin/jinfo17"

startApplication Loop

# all return statuses are checked in this test
set +e

failed=0

# -flag option
${JINFO17} -flag +PrintConcurrentLocks $appJavaPid
if [ $? != 0 ]; then failed=$((${failed}+1)); fi

${JINFO17} -flag PrintConcurrentLocks $appJavaPid | grep -q -e "-XX:+PrintConcurrentLocks"
if [ $? != 0 ]; then failed=$((${failed}+1)); fi

${JINFO17} -flag -PrintConcurrentLocks $appJavaPid
if [ $? != 0 ]; then failed=$((${failed}+1)); fi

${JINFO17} -flag PrintConcurrentLocks $appJavaPid | grep -q -e "-XX:-PrintConcurrentLocks"
if [ $? != 0 ]; then failed=$((${failed}+1)); fi

# -sysprops option
${JINFO17} -sysprops $appJavaPid | grep -q -e "java.specification.version=1.8"
if [ $? != 0 ]; then failed=$((${failed}+1)); fi

${JINFO17} -sysprops $appJavaPid | grep -q -e "java.vm.specification.version=17"
if [ $? != 0 ]; then failed=$((${failed}+1)); fi

set -e

stopApplication

exit $failed
