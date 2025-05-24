#!/bin/sh

set -e

readonly version="6.0.0.20250409"
readonly tarball="nvidia-index-libs-${version}-linux.tar.bz2"
readonly sha256sum="3f84973945ba95ef8dc96f29ed82225174801fd849ffddbb33ab91b4bdaf60d8"

readonly index_root="$HOME/index"

mkdir -p "$index_root"
cd "$index_root"

echo "$sha256sum  $tarball" > index.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/$tarball"
sha256sum --check index.sha256sum
tar -C "/usr/local" --strip-components=1 -xf "$tarball"

cd

rm -rf "$index_root"
