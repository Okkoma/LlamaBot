set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 est déjà trouvé au niveau racine

set(PROJECT_SOURCES
    main.cpp
    Application.cpp Application.h
    ApplicationServices.cpp ApplicationServices.h
    LLMServiceDefs.h
    LLMServices.h LLMServices.cpp
    LLMService.h LLMService.cpp
    LlamaCppService.h LlamaCppService.cpp
    OllamaService.h OllamaService.cpp
    ModelSource.h ModelSource.cpp
    ModelStoreDialog.h ModelStoreDialog.cpp
    HuggingFaceModelSource.h HuggingFaceModelSource.cpp
    OllamaModelSource.h OllamaModelSource.cpp
    Chat.h ChatImpl.h ChatImpl.cpp
    ChatController.h ChatController.cpp
    ChatStorage.h
    ChatStorageLocal.h ChatStorageLocal.cpp
    ThemeManager.h ThemeManager.cpp
    Clipboard.h Clipboard.cpp
    DocumentProcessor.h DocumentProcessor.cpp
    RAGService.h RAGService.cpp
    VectorStore.h VectorStore.cpp
)

qt_add_resources(PROJECT_SOURCES ressources.qrc)

qt_add_executable(LlamaBot MANUAL_FINALIZATION ${PROJECT_SOURCES})
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET LlamaBot APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

target_link_libraries(LlamaBot PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2 Qt6::Sql llama)
target_link_libraries(LlamaBot PRIVATE poppler-qt6)
target_link_libraries(LlamaBot PRIVATE LlamaBotCommon)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.okkomastudio.LlamaBot)

set_target_properties(LlamaBot PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(LlamaBot PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if (NOT ANDROID)
    execute_process (COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin")
    execute_process (COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/bin/data" "${CMAKE_BINARY_DIR}/bin/data")
    # Copy icons to build directory for proper icon building into project
    execute_process (COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/bin/data/images" "${CMAKE_CURRENT_BINARY_DIR}/images")
endif ()

include(GNUInstallDirs)
install(TARGETS LlamaBot
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install desktop file and icon for Linux
if(UNIX AND NOT APPLE)
    install(FILES ${CMAKE_SOURCE_DIR}/bin/data/resources/llamabot.desktop DESTINATION share/applications/)
    install(FILES ${CMAKE_SOURCE_DIR}/bin/data/images/icon1.png DESTINATION share/icons/hicolor/512x512/apps/ RENAME llamabot.png)
endif()

qt_finalize_executable(LlamaBot)


