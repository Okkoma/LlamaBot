set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Qml Quick QuickWidgets Test)

# Tests unitaires C++ avec Qt Test
add_executable(ChatBotTests
    tst_llmservice.cpp
    ../Source/Application/Chat.cpp
    ../Source/Application/LLMService.cpp
    ../Source/Application/LLMServiceDefs.cpp
)

qt_add_resources(chatbot_tests
    "${CMAKE_SOURCE_DIR}/Source/Application/resources.qrc"
)

target_include_directories(ChatBotTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source/Application
        ${CMAKE_SOURCE_DIR}/Source/ThirdParty/llama.cpp/src
)

target_link_libraries(ChatBotTests
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickWidgets
        Qt6::Test
        llama
)

add_test(NAME ChatBotTests COMMAND ChatBotTests)

# Tests QML avec Qt Quick Test via qmltestrunner
find_program(QMLTESTRUNNER_EXECUTABLE qmltestrunner)
if (QMLTESTRUNNER_EXECUTABLE)
    add_test(
        NAME QmlTests
        COMMAND ${QMLTESTRUNNER_EXECUTABLE}
            -input ${CMAKE_CURRENT_SOURCE_DIR}/qml
            -import ${CMAKE_SOURCE_DIR}/Source/Application/qml
    )
else()
    message(WARNING "qmltestrunner introuvable : les tests QML ne seront pas exécutés.")
endif()



