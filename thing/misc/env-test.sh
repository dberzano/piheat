#!/bin/bash
piheatBaseDir="$(dirname "${BASH_SOURCE}")"
piheatBaseDir="$( cd "${piheatBaseDir}/.." ; pwd )"
export PYTHONPATH="${piheatBaseDir}/pylib:${PYTHONPATH}"
export PATH="${piheatBaseDir}/usr/bin:${PATH}"
export PS1="[test-piheat] $PS1"
unset piheatBaseDir
