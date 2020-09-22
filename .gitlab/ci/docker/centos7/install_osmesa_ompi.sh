#!/bin/sh

[ -n "$INSTALL_OMPI" ] && yum install -y openmpi-devel
[ -n "$INSTALL_OSMESA" ] && yum install -y mesa-libOSMesa-devel

exit 0
