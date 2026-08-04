#!/bin/sh

set -e

readonly mindeps_version="3.12.4"
readonly mindeps_prefix="cmake-mindeps"
readonly latest_version="3.31.6"
readonly latest_prefix="cmake"

case "$( uname -s )-$( uname -m )" in
    Linux-x86_64)
        shatool="sha256sum"
        mindeps_sha256sum="486edd6710b5250946b4b199406ccbf8f567ef0e23cfe38f7938b8c78a2ffa5f"
        mindeps_platform="Linux-x86_64"
        latest_sha256sum="5a1133ff103c71eb5120e2cc3de922733e7d8a26a98ae716397e8676adb367bf"
        latest_platform="linux-x86_64"
        ;;
    Linux-aarch64)
        shatool="sha256sum"
        mindeps_sha256sum="UNSUPPORTED"
        mindeps_platform="UNSUPPORTED"
        latest_sha256sum="b4cc788d63112b2749b40627e719eb5d3b8ed8f00c36d77189f4019cfe64bc9e"
        latest_platform="linux-aarch64"
        ;;
    Darwin-*)
        shatool="shasum -a 256"
        mindeps_sha256sum="95d76c00ccb9ecb5cb51de137de00965c5e8d34b2cf71556cf8ba40577d1cff3"
        mindeps_platform="Darwin-x86_64"
        latest_sha256sum="330b9514f5112e5ed4fb08b8b05803b776fd9b539a6ae12927d14dcc0ee2ba8d"
        latest_platform="macos-universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly mindeps_sha256sum
readonly mindeps_platform
readonly latest_sha256sum
readonly latest_platform


# Select the CMake version to install
readonly cmake_version="${1:-latest}"

case "$cmake_version" in
    latest)
        version="$latest_version"
        sha256sum="$latest_sha256sum"
        platform="$latest_platform"
        prefix="$latest_prefix"
        ;;
    mindeps)
        version="$mindeps_version"
        sha256sum="$mindeps_sha256sum"
        platform="$mindeps_platform"
        prefix="$mindeps_prefix"

        # Skip if we're not in a `mindeps` job.
        if ! echo "$CMAKE_CONFIGURATION" | grep -q -e 'mindeps'; then
            exit 0
        fi
        ;;
    *)
        echo "Unknown CMake version: $cmake_version"
        exit 1
esac
readonly version
readonly sha256sum
readonly platform
readonly prefix

readonly filename="cmake-$version-$platform"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
$shatool --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" "$prefix"

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin "$prefix/bin"
fi

# Use "latest" version of cmake for spack
if [ "$CI_JOB_NAME" = "build:spack-centos7" ]; then
    mkdir -p "$CI_PROJECT_DIR/build/spack"
    sed \
        -e "s/CMAKE_VERSION/$latest_version/" \
        -e "s,CMAKE_PREFIX,$PWD/cmake," \
        < "$CI_PROJECT_DIR/Utilities/spack/configs/gitlab-ci/packages.yaml.in" \
        > "$CI_PROJECT_DIR/build/spack/packages.yaml"
fi
