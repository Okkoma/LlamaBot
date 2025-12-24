set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Qml Quick QuickWidgets Test)

# Tests unitaires de Chat avec mock LLMService
qt_add_executable(Test_Chat
    ../Source/Application/LLMServiceDefs.h
    ../Source/Application/Chat.h
    ../Source/Application/ChatImpl.h
    ../Source/Application/ChatImpl.cpp
    ../Source/Application/LLMService.h
    ../Source/Application/LLMService.cpp
    ../Source/Application/LLMServices.h
    ../Source/Application/LLMServices.cpp
    tst_chat.cpp
)

target_include_directories(Test_Chat PRIVATE ${CMAKE_SOURCE_DIR}/Source/Application)
target_link_libraries(Test_Chat PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)

# Tests unitaires LLMService avec mock Chat
qt_add_executable(Test_LLMServices
    ../Source/Application/LLMServiceDefs.h
    ../Source/Application/Chat.h
    ../Source/Application/ChatImpl.h
    ../Source/Application/ChatImpl.cpp
    ../Source/Application/LLMService.h
    ../Source/Application/LLMService.cpp
    ../Source/Application/LLMServices.h
    ../Source/Application/LLMServices.cpp
    tst_llmservices.cpp
)

target_include_directories(Test_LLMServices PRIVATE ${CMAKE_SOURCE_DIR}/Source/Application)
target_link_libraries(Test_LLMServices PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)

add_test(NAME Test_LLMServices COMMAND Test_LLMServices)
add_test(NAME Test_Chat COMMAND Test_Chat)


# Tests QML avec Qt Quick Test via qmltestrunner
find_program(QMLTESTRUNNER_EXECUTABLE qmltestrunner)
if (QMLTESTRUNNER_EXECUTABLE)
    add_test(NAME QmlTests COMMAND ${QMLTESTRUNNER_EXECUTABLE} 
            -input ${CMAKE_CURRENT_SOURCE_DIR}/qml
            -import ${CMAKE_SOURCE_DIR}/Source/Application/qml)
else()
    message(WARNING "qmltestrunner introuvable : les tests QML ne seront pas exécutés.")
endif()



