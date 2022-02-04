#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="rxqt"
readonly ownership="RxQt Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/rxqt.git"
readonly tag="for/paraview-master-g38e510f-20220204"
readonly paths="
include/*.hpp

LICENSE
README.md
README.kitware.md

CMakeLists.txt

.gitattributes
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
