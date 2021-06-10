#!/bin/bash

set -eux -o pipefail
shopt -s failglob extglob

ZUUL_JOB_NAME=$(jq < ~/zuul-env.json -r '.job')
ZUUL_TENANT=$(jq < ~/zuul-env.json -r '.tenant')
ZUUL_PROJECT_SRC_DIR=$HOME/$(jq < ~/zuul-env.json -r '.project.src_dir')
ZUUL_PROJECT_SHORT_NAME=$(jq < ~/zuul-env.json -r '.project.short_name')
ZUUL_PROJECT_NAME=$(jq < ~/zuul-env.json -r '.project.name')
ZUUL_GERRIT_HOSTNAME=$(jq < ~/zuul-env.json -r '.project.canonical_hostname')

CI_PARALLEL_JOBS=$(awk -vcpu=$(getconf _NPROCESSORS_ONLN) 'BEGIN{printf "%.0f", cpu*1.3+1}')
CMAKE_OPTIONS=""
CFLAGS=""
CXXFLAGS=""
LDFLAGS=""

if [[ $ZUUL_JOB_NAME =~ .*-clang.* ]]; then
    export CC=clang
    export CXX=clang++
    export LD=clang
fi

if [[ $ZUUL_JOB_NAME =~ .*-ubsan ]]; then
    export CFLAGS="-fsanitize=undefined ${CFLAGS}"
    export CXXFLAGS="-fsanitize=undefined ${CXXFLAGS}"
    export LDFLAGS="-fsanitize=undefined ${LDFLAGS}"
    export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
fi

if [[ $ZUUL_JOB_NAME =~ .*-asan ]]; then
    export CFLAGS="-fsanitize=address ${CFLAGS}"
    export CXXFLAGS="-fsanitize=address ${CXXFLAGS}"
    export LDFLAGS="-fsanitize=address ${LDFLAGS}"
fi

if [[ $ZUUL_JOB_NAME =~ .*-tsan ]]; then
    export CFLAGS="-fsanitize=thread ${CFLAGS}"
    export CXXFLAGS="-fsanitize=thread ${CXXFLAGS}"
    export LDFLAGS="-fsanitize=thread ${LDFLAGS}"
fi

if [[ $ZUUL_JOB_NAME =~ .*-cover.* ]]; then
    export CFLAGS="${CFLAGS} --coverage"
    export CXXFLAGS="${CXXFLAGS} --coverage"
    export LDFLAGS="${LDFLAGS} --coverage"
fi

PREFIX=~/target
mkdir ${PREFIX}
BUILD_DIR=~/build
mkdir ${BUILD_DIR}
export PATH=${PREFIX}/bin:$PATH
export LD_LIBRARY_PATH=${PREFIX}/lib64:${PREFIX}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export PKG_CONFIG_PATH=${PREFIX}/lib64/pkgconfig:${PREFIX}/lib/pkgconfig:${PREFIX}/share/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}

build_n_test() {
    pushd ~/src/${ZUUL_GERRIT_HOSTNAME}/$1
    git describe --tags || git log -n1
    popd
    mkdir -p ${BUILD_DIR}/$1
    pushd ${BUILD_DIR}/$1
    cmake -GNinja ${CMAKE_OPTIONS} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug} -DCMAKE_INSTALL_PREFIX=${PREFIX} ~/src/${ZUUL_GERRIT_HOSTNAME}/$1
    ninja-build install -j${CI_PARALLEL_JOBS}
    shift
    ctest -j${CI_PARALLEL_JOBS} --output-on-failure "$@"
    popd
}

build_n_test github/CESNET/libyang -DENABLE_BUILD_TESTS=ON -DENABLE_VALGRIND_TESTS=OFF
build_n_test github/onqtam/doctest -DDOCTEST_WITH_TESTS=OFF
build_n_test ${ZUUL_PROJECT_NAME} -DBUILD_TESTING=ON

pushd ${BUILD_DIR}/${ZUUL_PROJECT_NAME}
if [[ $JOB_PERFORM_EXTRA_WORK == 1 ]]; then
    ninja-build doc
    pushd html
    zip -r ~/zuul-output/docs/html.zip .
    popd
fi

if [[ $LDFLAGS =~ .*--coverage.* ]]; then
    gcovr -j ${CI_PARALLEL_JOBS} --object-directory ${BUILD_DIR}/${ZUUL_RPOJECT_NAME} --root ${ZUUL_PROJECT_SRC_DIR} --xml --output ${BUILD_DIR}/${ZUUL_PROJECT_NAME}/coverage.xml
fi
