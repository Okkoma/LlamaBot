cmake_minimum_required(VERSION 3.16)

project(ServerChatStore VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recherche de Qt6
if(DEFINED ENV{Qt6})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{Qt6}")
endif()

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Network 
    Sql
    HttpServer
)

# Permet de compiler le serveur en standalone (en dehors du CMake racine)
# tout en réutilisant la librairie commune.
if(NOT TARGET LlamaBotCommon)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../Common ${CMAKE_CURRENT_BINARY_DIR}/Common)
endif()

# Recherche d'OpenSSL pour le cryptage
find_package(OpenSSL REQUIRED)

# Recherche de PostgreSQL (libpq)
find_package(PostgreSQL REQUIRED)

set(SERVER_SOURCES
    ServerApp.h ServerApp.cpp
    ChatDbManager.h ChatDbManager.cpp
)

qt_add_executable(ServerChatStore ${SERVER_SOURCES})

target_link_libraries(ServerChatStore PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::Sql
    Qt6::HttpServer
    OpenSSL::SSL
    OpenSSL::Crypto
    PostgreSQL::PostgreSQL
    LlamaBotCommon
)

set_target_properties(ServerChatStore PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
include(GNUInstallDirs)
install(TARGETS ServerChatStore
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
