#!/bin/sh

set -e

# Install tools to manage repositories.
dnf install -y --setopt=install_weak_deps=False \
    'dnf-command(config-manager)'

# Install the CUDA repository.
dnf config-manager addrepo \
    --from-repofile=https://developer.download.nvidia.com/compute/cuda/repos/fedora41/x86_64/cuda-fedora41.repo

# CUDA toolchain
dnf install -y --setopt=install_weak_deps=False \
    cuda-compiler-12-9 cuda-cudart-devel-12-9 cuda-toolkit-12-9

dnf clean all
