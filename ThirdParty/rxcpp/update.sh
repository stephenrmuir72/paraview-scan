#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="rxcpp"
readonly ownership="RxCpp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/rxcpp.git"
readonly tag="for/paraview-master-gec38393-20220103"
readonly paths="
Rx/v2/src/rxcpp/
Rx/v2/license.txt

license.md
Readme.html
README.kitware.md

CMakeLists.paraview.txt

.gitattributes
"

extract_source () {
    git_archive
    pushd "$extractdir"
    mv -v "$name-reduced/Rx/v2/src/rxcpp" "$name-reduced/Rx/v2/src/vtkrxcpp"
    mv -v "$name-reduced/CMakeLists.paraview.txt" "$name-reduced/CMakeLists.txt"
    # Add missing newlines.
    echo >> "$name-reduced/Rx/v2/license.txt"
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
