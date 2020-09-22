FROM kitware/paraview:ci-superbuild-centos7-20200210
MAINTAINER Utkarsh Ayachit <utkarsh.ayachit@kitware.com>

# ARGS empty by default
ARG INSTALL_OMPI
ARG INSTALL_OSMESA

COPY install_deps.sh /root/install_deps.sh
COPY install_osmesa_ompi.sh /root/install_osmesa_ompi.sh

RUN sh /root/install_deps.sh
RUN sh /root/install_osmesa_ompi.sh
