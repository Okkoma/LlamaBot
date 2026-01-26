cmake_minimum_required(VERSION 3.16)

project(LlamaBotTest VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Qml Quick QuickWidgets Test)

if (NOT ANDROID)
    enable_testing()
    add_definitions(-DTEST)
    add_subdirectory(Common)
    add_subdirectory(Application)
endif()
