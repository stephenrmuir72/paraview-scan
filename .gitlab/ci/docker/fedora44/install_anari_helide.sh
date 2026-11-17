#!/bin/sh

set -e

if [ "$( uname -m )" != "x86_64" ]; then
    exit 0
fi

readonly anari_repo="https://github.com/KhronosGroup/ANARI-SDK.git"
readonly anari_commit="v0.15.0"

readonly anari_root="$HOME/ANARI-SDK"
readonly anari_src="$anari_root/src"
readonly anari_build="$anari_root/build"

git clone -b "$anari_commit" "$anari_repo" "$anari_src"

cmake -GNinja \
    -S "$anari_src" \
    -B "$anari_build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/anari \
    -DBUILD_HELIDE_DEVICE:BOOL=ON \
    -DBUILD_VIEWER:BOOL=OFF \
    -DBUILD_EXAMPLES:BOOL=OFF \
    -DBUILD_CTS:BOOL=OFF \
    -DBUILD_TESTING:BOOL=OFF
cmake --build "$anari_build" --parallel --target install

rm -rf "$anari_root"
