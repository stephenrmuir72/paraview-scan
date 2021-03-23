#!/bin/sh

# Install extra dependencies for ParaView
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core git-lfs

# Python dependencies
dnf install -y --setopt=install_weak_deps=False \
    python3 python3-devel python3-numpy python3-pip

dnf clean all
