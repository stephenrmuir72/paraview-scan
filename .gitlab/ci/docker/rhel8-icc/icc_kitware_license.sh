#!/bin/sh

dnf install --setopt=install_weak_deps=False -y \
    bind-utils
dnf clean all

readonly license_ip="10.83.83.100"

mkdir -p /opt/intel/licenses
cat >/opt/intel/licenses/USE_SERVER.lic <<EOF
SERVER license.kitware.com ANY 28518
USE_SERVER
EOF

cat >>/etc/hosts <<EOF
$license_ip	license license.kitware.com
EOF
