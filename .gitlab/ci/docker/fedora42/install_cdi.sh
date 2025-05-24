#!/bin/sh

set -e

readonly cdi_repo="https://gitlab.dkrz.de/mpim-sw/libcdi.git"
readonly cdi_commit="cdi-2.5.2.1"

readonly cdi_root="$HOME/cdi"
readonly cdi_src="$cdi_root/src"
readonly cdi_build_root="$cdi_root/build"

git clone "$cdi_repo" "$cdi_src"
git -C "$cdi_src" checkout "$cdi_commit"

dnf install -y --setopt=install_weak_deps=False \
    libtool eccodes eccodes-devel

cdi_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    ln "$cdi_src"/cmake/FindNetCDF.cmake "$cdi_src"/cmake/FindnetCDF.cmake
    cmake -GNinja \
        -S "$cdi_src" \
        -B "$cdi_build_root/$subdir" \
        -DCMAKE_MODULE_PATH="$cdi_src/cmake" \
        -DCDI_NETCDF=ON \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS="-DHAVE_GETLINE=1" \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$cdi_build_root/$subdir" --target install
}

# MPI-less
cdi_build nompi /usr \
    -DCMAKE_INSTALL_LIBDIR=lib

rm -rf "$cdi_root"

dnf clean all
