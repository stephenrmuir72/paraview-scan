cmake_minimum_required(VERSION 3.8)
project(PVAPIRemoteConnection)

find_package(ParaView REQUIRED)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

set(sources
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cxx)

add_executable(PVAPIRemoteConnectionExample ${sources})
target_link_libraries(PVAPIRemoteConnectionExample PRIVATE
    ParaView::RemotingCore
    ParaView::RemotingApplication
    ParaView::RemotingViews)
