# SPDX-License-Identifier: MIT
# Tanmatsu VFD Clock Plugin

cmake_minimum_required(VERSION 3.16)

set(PLUGIN_NAME "vfdclock")
set(TANMATSU_PLUGIN_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../tanmatsu-launcher/tools/plugin-sdk")

project(${PLUGIN_NAME} C)

# Include the plugin build system
include(${TANMATSU_PLUGIN_SDK}/plugin-build.cmake)

# Plugin source files
set(PLUGIN_SOURCES
    src/main.c
)

# Build the plugin
build_tanmatsu_plugin(${PLUGIN_NAME} "${PLUGIN_SOURCES}")
