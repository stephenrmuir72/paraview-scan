#!/bin/sh

set -e

readonly version="0.2.15-background-init"
readonly build_date="20210413.1"

case "$( uname -s )-$(uname -m)" in
    Linux-x86_64)
        shatool="sha256sum"
        sha256sum="5814ccfeebf8c4cacefa7bf82ec5304ffd5ef03dea5d5b35339943ec5d3236aa"
        platform="x86_64-unknown-linux-gnu"
        ;;
    Darwin-x86_64|Darwin-arm64)
        shatool="shasum -a 256"
        sha256sum="b6a6c702a932165b262e67145a26657a7bada5a0bce4d81d570e3a734fc987ce"
        platform="universal-apple-darwin"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="sccache-v$version-$platform"

#readonly url="https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/sccache/v$version-$build_date/"
readonly url="https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/sccache/0.2.1501"

cd .gitlab

echo "$sha256sum  $filename" > sccache.sha256sum
curl -OL "$url/$filename"
$shatool --check sccache.sha256sum
mv "$filename" sccache
chmod +x sccache

mkdir shortcuts
cp ./sccache shortcuts/gcc
cp ./sccache shortcuts/g++
