FROM fedora:42
MAINTAINER Vicente Adolfo Bolea Sanchez <vicente.bolea@kitware.com>

COPY install_deps.sh /root/install_deps.sh
RUN sh /root/install_deps.sh

# Set the default compiler to GCC 14
ENV CC=/usr/bin/gcc-14
ENV CXX=/usr/bin/g++-14
ENV FC=/usr/bin/gfortran-14

COPY install_cuda.sh /root/install_cuda.sh
RUN sh /root/install_cuda.sh

COPY install_adios.sh /root/install_adios.sh
RUN sh /root/install_adios.sh

COPY install_openvr.sh /root/install_openvr.sh
RUN sh /root/install_openvr.sh

COPY install_index.sh /root/install_index.sh
RUN sh /root/install_index.sh

COPY install_cdi.sh /root/install_cdi.sh
RUN sh /root/install_cdi.sh

COPY install_catalyst.sh /root/install_catalyst.sh
RUN sh /root/install_catalyst.sh

# COPY install_pythonqt.sh /root/install_pythonqt.sh
# RUN sh /root/install_pythonqt.sh

COPY install_openturns.sh /root/install_openturns.sh
RUN sh /root/install_openturns.sh
