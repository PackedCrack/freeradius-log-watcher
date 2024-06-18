#!/bin/bash


echo
echo "---------CPPCHECK----------"
if [ ! -d build-cppcheck ]; then
    mkdir build-cppcheck
fi

cppcheck --cppcheck-build-dir=build-cppcheck --error-exitcode=1 --enable=all --suppressions-list=.suppress.cppcheck --inline-suppr --std=c++20 $(find src/ -name '*.cpp' -o -name '*.hpp')

result=$?
if [ $result -ne 0 ]; then
    echo "Cppcheck failed with exit code $result"
    exit $result
fi
cd ..
echo "-------CPPCHECK DONE----------"
echo