#!/usr/bin/env bash

VERSION=$(cat cvm/conf/version)

make -f cvm.mk cvm8default17 MODE=release

rm -rf output/CompoundVM_${VERSION}_jvm_patch*
mv output/CompoundVM_${VERSION}_linux_*/* output/
rm -rf output/CompoundVM_${VERSION}_linux_*
