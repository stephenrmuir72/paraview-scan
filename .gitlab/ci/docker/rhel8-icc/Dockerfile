FROM kitware/intelcompiler:2020-rhel8
MAINTAINER Ben Boeckel <ben.boeckel@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

COPY icc_kitware_license.sh /root/icc_kitware_license.sh
RUN sh /root/icc_kitware_license.sh
