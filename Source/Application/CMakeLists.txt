
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Qml Quick QuickWidgets)

set(PROJECT_SOURCES
        main.cpp
        Application.cpp
        Application.h
        ApplicationServices.cpp
        ApplicationServices.h
        LLMServiceDefs.cpp
        LLMServiceDefs.h        
        LLMService.cpp
        LLMService.h
        LlamaCppService.cpp
        LlamaCppService.h 
        OllamaService.cpp
        OllamaService.h
        OllamaModelStoreDialog.cpp
        OllamaModelStoreDialog.h         
        MainWindow.cpp
        MainWindow.h
        ChatBotPanel.cpp Chat.cpp
        ChatBotPanel.h Chat.h
        ChatController.h
)

qt_add_resources(PROJECT_SOURCES ressources.qrc)

qt_add_executable(ChatBot MANUAL_FINALIZATION ${PROJECT_SOURCES})
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ChatBot APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

target_link_libraries(ChatBot PRIVATE Qt6::Core Qt6::Network Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickWidgets llama)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ChatBot)

set_target_properties(ChatBot PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if (NOT ANDROID)
    # Install Section
    execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/bin/data" "${CMAKE_CURRENT_BINARY_DIR}/data")
endif ()

include(GNUInstallDirs)
install(TARGETS ChatBot
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(ChatBot)


