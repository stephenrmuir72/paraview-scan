#!/bin/sh

set -e

readonly cdi_repo="https://gitlab.dkrz.de/mpim-sw/libcdi.git"
readonly cdi_commit="cdi-2.0.4"

readonly cdi_root="$HOME/cdi"
readonly cdi_src="$cdi_root/src"
git clone -b "$cdi_commit" "$cdi_repo" "$cdi_src"

cdi_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cd "$cdi_src"
    git worktree add "$cdi_root/$subdir" "$cdi_commit"
    cd "$cdi_root/$subdir"

    ./autogen.sh
    ./configure --prefix=$prefix --with-netcdf=yes "$@"
    make
    make install
}

# MPI-less
cdi_build nompi /usr \
    --with-mpi=no

# MPICH
cdi_build mpich /usr/lib64/mpich \
    --with-mpi=yes

# OpenMPI
cdi_build openmpi /usr/lib64/openmpi \
    --with-mpi=yes

rm -rf "$cdi_root"
