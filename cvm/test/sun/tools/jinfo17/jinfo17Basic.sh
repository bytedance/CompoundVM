#!/bin/sh


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
