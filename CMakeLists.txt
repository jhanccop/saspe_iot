cmake_minimum_required(VERSION 3.0.0)
project(siren_saspe_device VERSION 0.1.0)

include(CTest)
enable_testing()

add_library(siren_saspe_device siren_saspe_device.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
