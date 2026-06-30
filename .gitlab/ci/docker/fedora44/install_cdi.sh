#!/bin/sh

set -e

readonly cdi_repo="https://gitlab.dkrz.de/mpim-sw/libcdi.git"
readonly cdi_commit="cdi-2.6.3"

readonly cdi_root="$HOME/cdi"
readonly cdi_src="$cdi_root/src"
readonly cdi_build_root="$cdi_root/build"

git clone "$cdi_repo" "$cdi_src"
git -C "$cdi_src" checkout "$cdi_commit"

# The CMake library target was renamed from `cdilib` to `libcdi` in 2.6.x
# (CMake prefixes the target name with `lib`, producing `liblibcdi.so`), but
# the shipped cdi-config.cmake.in template was not updated to match, so
# find_package(CDI) fails to locate the library.
patch -d "$cdi_src" -p1 <<'EOF'
--- a/src/cmake/cdi/cdi-config.cmake.in
+++ b/src/cmake/cdi/cdi-config.cmake.in
@@ -15,7 +15,7 @@
 mark_as_advanced(CDI_INCLUDE_DIRECTORY)

 find_library(CDI_LIBRARY
-  NAMES cdilib
+  NAMES libcdi cdilib
   PATHS "${CDI_ROOT_DIR}/lib"
   # use following line instead of previous if this file can be configured
   # PATHS "@libdir@"
EOF

dnf install -y --setopt=install_weak_deps=False \
    libtool eccodes eccodes-devel ruby ruby-devel

cdi_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$cdi_src" \
        -B "$cdi_build_root/$subdir" \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_TESTING=OFF \
        -DCDI_BUILD_APP=OFF \
        -DCDI_BUILD_UNKNOWN=OFF \
        -DCDI_ECCODES=OFF \
        -DCDI_EXTRA=OFF \
        -DCDI_IEG=OFF \
        -DCDI_LIBGRIB=OFF \
        -DCDI_LIBGRIBEX=OFF \
        -DCDI_NETCDF=ON \
        -DCDI_PTHREAD=OFF \
        -DCDI_SERVICE=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DCMAKE_MODULE_PATH="$cdi_src/cmake" \
        "$@"
    cmake --build "$cdi_build_root/$subdir" --target install
}

# MPI-less
cdi_build nompi /usr

rm -rf "$cdi_root"

dnf clean all
