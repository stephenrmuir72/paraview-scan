#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================

# Location where we'll install ParaView
# Don't forget to update Examples/CMakeLists.txt if you change this
# location (or test name)
set(install_root "${CMAKE_BINARY_DIR}/Testing/Temporary/InstallRoot")

# add a prep-test to cleanup old directories.
if (CMAKE_VERSION VERSION_LESS "3.17")
  add_test(
    NAME "ParaView::Install::Prepare"
    COMMAND "${CMAKE_COMMAND}" -E remove_directory "${install_root}")
else()
  add_test(
    NAME "ParaView::Install::Prepare"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${install_root}")
endif()

add_test(
  NAME "ParaView::Install"
  COMMAND "${CMAKE_COMMAND}"
          --install "${CMAKE_BINARY_DIR}"
          --config $<CONFIGURATION>
          --prefix "${install_root}")
set_tests_properties("ParaView::Install::Prepare"
  PROPERTIES
    FIXTURES_SETUP "ParaView::Install::Prepare")
set_tests_properties("ParaView::Install"
    PROPERTIES
      FIXTURES_REQUIRED "ParaView::Install::Prepare"
      FIXTURES_SETUP    "ParaView::Install"
      # 10 minutes should be enough time.
      TIMEOUT 600)
