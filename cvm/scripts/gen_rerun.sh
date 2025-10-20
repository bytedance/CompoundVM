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

#!/usr/bin/env bash

function gen_rerun {
  JTR=$1
  if [[ "x${JTR}" = "x" ]] || [[ ! -f "${JTR}" ]]; then
    echo ".jtr file not found ${JTR}"
    exit 127
  fi

  RERUN_SH="${JTR}.rerun.sh"
  GDB_SH="${JTR}.gdb.sh"
  JDWP_SH="${JTR}.jdwp.sh"
  DUAL_SH="${JTR}.dual.sh"

  rm -f ${RERUN_SH} ${GDB_SH} ${JDWP_SH} ${DUAL_SH}

  LN_START=$((`grep -n '\-\-rerun:' ${JTR} | tail -n 1 | awk -F: '{print $1}'` + 1))
  # last valid line
  LN_END=$((`grep -n '^result: ' ${JTR} | tail -n 1 | awk -F: '{print $1}'` - 1))
  head -n ${LN_END} ${JTR} | tail -n $(( ${LN_END} - ${LN_START} )) | sed 's/\\\\/\\/g' > ${RERUN_SH}

  while read -r LINE; do
    if [[ "x$(echo ${LINE} | grep -v '=' | grep '/bin/java ')" != "x" ]]; then
      echo "gdb --init-command=$PWD/cvm/scripts/cvm.gdb --args ${LINE}" >> ${GDB_SH}
      echo "gdb --init-command=$PWD/cvm/scripts/cvm.gdb --args ${LINE}" >> ${DUAL_SH}
      echo "${LINE}" >> ${JDWP_SH}
      continue
    fi
    if [[ "x$(echo ${LINE} | grep '^-server25')" != "x" ]]; then
      echo "${LINE}" >> ${GDB_SH}
      echo "${LINE}" >> ${JDWP_SH}
      echo "${LINE}" >> ${DUAL_SH}
      echo "-agentlib:jdwp25=transport=dt_socket,address=8888,server=y,suspend=y \\" >> ${JDWP_SH}
      echo "-agentlib:jdwp25=transport=dt_socket,address=8888,server=y,suspend=y \\" >> ${DUAL_SH}
      continue
    fi
    echo ${LINE} >> ${JDWP_SH}
    echo ${LINE} >> ${GDB_SH}
    echo ${LINE} >> ${DUAL_SH}
  done < ${RERUN_SH}

  echo "Re-run: ${RERUN_SH}"
  echo "Launch GDB: ${GDB_SH}"
  echo "Launch JDWP: ${JDWP_SH}"
  echo "Dual debugging: ${DUAL_SH}"

  chmod +x ${RERUN_SH} ${GDB_SH} ${JDWP_SH} ${DUAL_SH}
}

gen_rerun $*
