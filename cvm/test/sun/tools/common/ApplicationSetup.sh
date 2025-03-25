#!/bin/sh

#
# Copyright (c) 2005, 2012, Oracle and/or its affiliates. All rights reserved.
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

# Checks that TESTJAVA, TESTSRC, and TESTCLASSES environment variables are set.
#
# Creates the following constants for use by the caller:
#   JAVA        - java launcher

if [ -z "${TESTJAVA}" ]; then
  echo "ERROR: TESTJAVA not set.  Test cannot execute.  Failed."
  exit 1
fi

if [ -z "${TESTSRC}" ]; then
  echo "ERROR: TESTSRC not set.  Test cannot execute.  Failed."
  exit 1
fi

if [ -z "${TESTCLASSES}" ]; then
  echo "ERROR: TESTCLASSES not set.  Test cannot execute.  Failed."
  exit 1
fi

JAVA="${TESTJAVA}/bin/java -server17"

# Support functions to start and stop a given application

# Starts a given app as background process, usage:
#   startApplication <class> [args...]
#
# The following variables are set:
#
# appJavaPid  - application's Java pid
# appOutput   - file containing stdout and stderr from the app
#
# Waits for at least one line of output from the app to indicate
# that it is up and running.
#
startApplication()
{
  appOutput="${TESTCLASSES}/Application.out"

  ${JAVA} -XX+UsePerfData -classpath "${TESTCLASSPATH:-${TESTCLASSES}}" "$@" > "$appOutput" 2>&1 &
  appJavaPid="$!"

  echo "INFO: waiting for $1 to initialize..."
  _cnt=0
  while true; do
    # if the app doesn't start then the JavaTest/JTREG timeout will
    # kick in so this isn't really a endless loop
    sleep 1
    out=`tail -1 "$appOutput"`
    if [ -n "$out" ]; then
      # we got some output from the app so it's running
      break
    fi
    _cnt=`expr $_cnt + 1`
    echo "INFO: waited $_cnt second(s) ..."
  done
  unset _cnt

  echo "INFO: $1 is process $appJavaPid"
  echo "INFO: $1 output is in $appOutput"
}

stopApplication()
{
  set +e
  echo "INFO: killing $appJavaPid"
  kill -TERM "$appJavaPid"  # try a polite SIGTERM first
  sleep 2
  # send SIGKILL (-9) just in case SIGTERM didn't do it
  # but don't show any complaints
  kill -KILL "$appJavaPid" > /dev/null 2>&1
  wait "$appJavaPid"
  set -e
}
