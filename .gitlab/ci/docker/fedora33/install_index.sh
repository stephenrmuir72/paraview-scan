#!/bin/sh

set -e

readonly version="5.9.20201204"
readonly tarball="nvidia-index-libs-${version}-linux.tar.bz2"
readonly sha256sum="2a5380a39001d77df701595963239bf164dbeb12e64710c0168ddf98f5f0e9ee"

readonly index_root="$HOME/index"

mkdir -p "$index_root"
cd "$index_root"

echo "$sha256sum  $tarball" > index.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/$tarball"
sha256sum --check index.sha256sum
tar -C "/usr/local" --strip-components=1 -xf "$tarball"

cd

rm -rf "$index_root"
