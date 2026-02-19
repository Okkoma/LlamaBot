# Qt automoc, autouic, autorcc
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (ACTIVE_QML_DEBUG)
    add_definitions("-DLLAMABOT_QML_DEBUG")
endif()

set(PROJECT_SOURCES
    main.cpp
    ApplicationServices.cpp ApplicationServices.h
    LLMServiceDefs.h
    LLMServices.h LLMServices.cpp
    LLMService.h LLMService.cpp
    LlamaCppService.h LlamaCppService.cpp
    OllamaService.h OllamaService.cpp
    Encryption.h Encryption.cpp
    ChatImpl.h ChatImpl.cpp ChatConverter.cpp
    ChatStorage.h
    ChatStorageLocal.h ChatStorageLocal.cpp
    ChatStorageDistant.h ChatStorageDistant.cpp
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

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_library(LlamaBotQml STATIC)
qt_add_qml_module(LlamaBotQml
    URI LlamaBotQml
    VERSION 1.0
    OUTPUT_DIRECTORY LlamaBotQml

    QML_FILES
        qml/APISelector.qml
        qml/AssetContent.qml
        qml/ChatDrawer.qml
        qml/ChatView.qml
        qml/InputArea.qml
        qml/LoadingSpinner.qml
        qml/Main.qml
        qml/MessageDelegate.qml
        qml/ModelSelector.qml
        qml/ModelStoreDialog.qml
        qml/SettingsDialog.qml
        qml/+android/ChatDrawer.qml
        qml/+android/InputArea.qml        
        qml/+android/Main.qml
        qml/+android/MessageDelegate.qml
        qml/+android/ModelStoreDialog.qml
        qml/+android/SettingsDialog.qml
        
    SOURCES
        Application.h
        Application.cpp
        Chat.h
        ChatController.h
        ChatController.cpp
        ThemeManager.h
        ThemeManager.cpp
        Clipboard.h
        Clipboard.cpp
        ModelSource.h ModelSource.cpp
        ModelStore.h ModelStore.cpp
        HuggingFaceModelSource.h HuggingFaceModelSource.cpp
        OllamaModelSource.h OllamaModelSource.cpp
        ErrorMessenger.h ErrorMessenger.cpp
)  

target_link_libraries(LlamaBotQml PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2 Qt6::Sql)
target_link_libraries(LlamaBotQml PRIVATE LlamaBotCommon)

target_link_libraries(LlamaBot PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2 Qt6::Sql)
target_link_libraries(LlamaBot PRIVATE llama)
target_link_libraries(LlamaBot PRIVATE poppler-qt6)
target_link_libraries(LlamaBot PRIVATE LlamaBotCommon)
target_link_libraries(LlamaBot PRIVATE LlamaBotQml)

if (ANDROID)
    add_android_openssl_libraries(LlamaBot)
    target_include_directories(LlamaBot PRIVATE ${CMAKE_SOURCE_DIR}/Source/ThirdParty/android_openssl/ssl_3/include)
    target_link_libraries(LlamaBot PRIVATE 
        ${CMAKE_SOURCE_DIR}/Source/ThirdParty/android_openssl/ssl_3/${CMAKE_ANDROID_ARCH_ABI}/libssl_3.so
        ${CMAKE_SOURCE_DIR}/Source/ThirdParty/android_openssl/ssl_3/${CMAKE_ANDROID_ARCH_ABI}/libcrypto_3.so
    )
else()
    target_link_libraries(LlamaBot PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.okkomastudio.llamabot)

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
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    QT_ANDROID_PACKAGE_NAME "com.okkomastudio.llamabot"
    QT_ANDROID_APP_NAME "LlamaBot"
    QT_ANDROID_APP_ICON "@drawable/ic_launcher"
    QT_ANDROID_VERSION_NAME "1.0"
    QT_ANDROID_VERSION_CODE 10   
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
    install(FILES ${CMAKE_SOURCE_DIR}/bin/data/ressources/llamabot.desktop DESTINATION share/applications/)
    install(FILES ${CMAKE_SOURCE_DIR}/bin/data/images/icon128.png DESTINATION share/icons/hicolor/128x128/apps RENAME llamabot.png)
    install(FILES ${CMAKE_SOURCE_DIR}/bin/data/images/icon256.png DESTINATION share/icons/hicolor/256x256/apps RENAME llamabot.png)
    install(FILES ${CMAKE_SOURCE_DIR}/bin/data/images/icon512.png DESTINATION share/icons/hicolor/512x512/apps RENAME llamabot.png)
    install(FILES ${CMAKE_SOURCE_DIR}/bin/data/images/icon1024.png DESTINATION share/icons/hicolor/1024x1024/apps RENAME llamabot.png)
endif()

qt_import_qml_plugins(LlamaBot)

qt_finalize_executable(LlamaBot)


