#!/bin/sh

# This project is a modified version of OpenJDK, licensed under GPL v2.
# Modifications Copyright (C) 2025 ByteDance Inc.
#
# This code is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 only, as
# published by the Free Software Foundation.
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
# @summary Unit test for jstack17 utility
#
# @library ../common
# @build Loop
# @run shell jstack17Basic.sh

. ${TESTSRC}/../common/ApplicationSetup.sh

JSTACK17="${TESTJAVA}/bin/jstack17"

startApplication Loop

# all return statuses are checked in this test
set +e

failed=0

# normal
$JSTACK17 $appJavaPid | grep -q -e "at Loop.main"
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: default option"; fi

# -l option
$JSTACK17 -l $appJavaPid | grep -q -e "Locked ownable synchronizers:"
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-l' option"; fi

# -e option
$JSTACK17 -e $appJavaPid | grep -q -e "allocated="
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-e' option"; fi

set -e

stopApplication

exit $failed
