#!/bin/sh

set -e

readonly VERSION="1.8.13"
readonly TARGET="doxygen-$VERSION"
readonly TARBALL="$TARGET.linux.bin.tar.gz"

cd .gitlab

curl -OL https://sourceforge.net/projects/doxygen/files/rel-$VERSION/$TARBALL
cmake/bin/cmake -E tar xf $TARBALL
mv $TARGET doxygen
