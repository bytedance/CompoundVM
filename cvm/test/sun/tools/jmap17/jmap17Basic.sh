#!/bin/sh

#
# Copyright (c) 2005, 2011, Oracle and/or its affiliates. All rights reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
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
#
# Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
# or visit www.oracle.com if you need additional information or have any
# questions.
#


# @test
# @bug 1234567
# @summary Unit test for jmap17 utility
#
# @library ../common
# @build Loop
# @run shell jmap17Basic.sh

. ${TESTSRC}/../common/ApplicationSetup.sh

JMAP17="${TESTJAVA}/bin/jmap17"
JHAT="${TESTJAVA}/bin/jhat"

startApplication Loop

# all return statuses are checked in this test
set +e

failed=0

# -clstats option
# We have three classloaders: app, ext, and boot
${JMAP17} -clstats $appJavaPid | grep -q -e "Total = 3"
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-clstats' option"; fi

# -finalizerinfo option
${JMAP17} -finalizerinfo $appJavaPid
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-finalizerinfo' option"; fi

###
### Test -histo option
###

# There is only one instance of our Loop class, which should be shown by -histo option
patNum='[0-9]\+'
patSpaces='[[:space:]]\+'
patClsName='Loop'
regPattern="^${patSpaces}${patNum}:${patSpaces}1${patSpaces}${patNum}${patSpaces}${patClsName}$"

# -histo:all option
${JMAP17} -histo:all $appJavaPid | grep -q -e ${regPattern}
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-histo:all' option"; fi

# -histo:live option
${JMAP17} -histo:live $appJavaPid | grep -q -e ${regPattern}
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-histo:live' option"; fi

HISTO_FILE="java_pid${appJavaPid}.histo"

# -histo:file= option
${JMAP17} -histo:file=${HISTO_FILE} $appJavaPid && grep -q -e ${regPattern} ${HISTO_FILE}
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-histo:file=' option"; fi

rm ${HISTO_FILE}

# -histo:parallel=1 option
${JMAP17} -histo:parallel=1 $appJavaPid | grep -q -e ${regPattern}
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-histo:parallel=1' option"; fi

# -histo:parallel=4 option
${JMAP17} -histo:parallel=4 $appJavaPid | grep -q -e ${regPattern}
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-histo:parallel=4' option"; fi

###
### Test -dump option
###

DUMP_FILE="java_pid${appJavaPid}.hprof"

# -dump option
${JMAP17} -dump:format=b,file=${DUMP_FILE} $appJavaPid
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-dump' option"; fi
# check that heap dump is parsable
${JHAT} -parseonly true ${DUMP_FILE}
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-dump' file not parable"; fi

# dump file is large so remove it
rm ${DUMP_FILE}

# -dump:live option
${JMAP17} -dump:live,format=b,file=${DUMP_FILE} $appJavaPid
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-dump:live' option"; fi
# check that heap dump is parsable
${JHAT} -parseonly true ${DUMP_FILE}
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-dump:live' file not parable"; fi

# dump file is large so remove it
rm -f ${DUMP_FILE}

# -dump:live,gz=1 option
${JMAP17} -dump:live,format=b,file=${DUMP_FILE},gz=1 $appJavaPid
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-dump:live,gz=1' option"; fi
rm -f ${DUMP_FILE}

# -dump:live,gz=9 option
${JMAP17} -dump:live,format=b,file=${DUMP_FILE},gz=9 $appJavaPid
if [ $? != 0 ]; then failed=$((${failed}+1)); echo "Fail: '-dump:live,gz=9' option"; fi
rm -f ${DUMP_FILE}

set -e

stopApplication

exit $failed
