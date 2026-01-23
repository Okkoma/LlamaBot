
set(CMAKE_AUTOMOC ON)

# Librairie commune utilisée par Application et les serveurs
add_library(LlamaBotCommon
    ErrorSystem.h
    ErrorSystem.cpp
)

add_library(LlamaBot::Common ALIAS LlamaBotCommon)

target_link_libraries(LlamaBotCommon
    PUBLIC
        Qt6::Core
)

target_include_directories(LlamaBotCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(LlamaBotCommon PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)


