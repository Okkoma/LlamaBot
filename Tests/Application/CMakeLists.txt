# Tests unitaires de Chat avec mock LLMService
qt_add_executable(Test_Chat
    mock_services.h
    ../../Source/Application/LLMServiceDefs.h
    ../../Source/Application/Chat.h
    ../../Source/Application/ChatImpl.h
    ../../Source/Application/ChatImpl.cpp
    ../../Source/Application/LLMService.h
    ../../Source/Application/LLMService.cpp
    ../../Source/Application/LLMServices.h
    ../../Source/Application/LLMServices.cpp
    mock_services.cpp
    tst_chat.cpp
)

target_link_libraries(Test_Chat PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)
add_test(NAME Test_Chat COMMAND Test_Chat)

# Tests unitaires LLMService avec mock Chat
qt_add_executable(Test_LLMServices
    mock_services.h
    ../../Source/Application/ChatImpl.h
    ../../Source/Application/ChatImpl.cpp
    ../../Source/Application/LLMService.h
    ../../Source/Application/LLMService.cpp
    ../../Source/Application/LLMServiceDefs.h
    ../../Source/Application/LLMServices.h
    ../../Source/Application/LLMServices.cpp
    ../../Source/Application/Chat.h
    mock_services.cpp
    tst_llmservices.cpp 
)
target_link_libraries(Test_LLMServices PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)
add_test(NAME Test_LLMServices COMMAND Test_LLMServices)

qt_add_executable(Test_RAG
    ../../Source/Application/VectorStore.h
    ../../Source/Application/VectorStore.cpp
    ../../Source/Application/DocumentProcessor.h
    ../../Source/Application/DocumentProcessor.cpp
    tst_rag.cpp
)
target_link_libraries(Test_RAG PRIVATE Qt6::Core Qt6::Test poppler-qt6)
add_test(NAME Test_RAG COMMAND Test_RAG)

qt_add_executable(Test_Ollama
    mock_services.h
    ../../Source/Application/LLMServiceDefs.h
    ../../Source/Application/LLMServices.h
    ../../Source/Application/LLMServices.cpp
    ../../Source/Application/LLMService.h
    ../../Source/Application/LLMService.cpp
    ../../Source/Application/Chat.h
    ../../Source/Application/ChatImpl.h
    ../../Source/Application/ChatImpl.cpp
    ../../Source/Application/OllamaService.h
    ../../Source/Application/OllamaService.cpp
    mock_services.cpp
    tst_ollama.cpp
)
target_link_libraries(Test_Ollama PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test)
add_test(NAME Test_Ollama COMMAND Test_Ollama)

qt_add_executable(Test_LlamaCpp
    mock_services.h
    ../../Source/Application/LLMServiceDefs.h
    ../../Source/Application/LLMServices.h
    ../../Source/Application/LLMServices.cpp
    ../../Source/Application/LLMService.h
    ../../Source/Application/LLMService.cpp
    ../../Source/Application/Chat.h
    ../../Source/Application/ChatImpl.h
    ../../Source/Application/ChatImpl.cpp
    ../../Source/Application/LlamaCppService.h
    ../../Source/Application/LlamaCppService.cpp
    mock_services.cpp
    tst_llamacpp.cpp
)
# Note: LlamaCpp needs llama.cpp headers/libs if real generation is tested, 
# but for basic service logic we might get away with it if mocks/stubs are used or if focused on config.
target_link_libraries(Test_LlamaCpp PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Test llama)
add_test(NAME Test_LlamaCpp COMMAND Test_LlamaCpp)

qt_add_executable(Test_StorageLocal
    mock_services.h
    mock_llmservices.h
    ../../Source/Application/LLMServiceDefs.h
    ../../Source/Application/LLMServices.h
    ../../Source/Application/LLMServices.cpp
    ../../Source/Application/LLMService.h
    ../../Source/Application/LLMService.cpp
    ../../Source/Application/Chat.h
    ../../Source/Application/ChatImpl.h
    ../../Source/Application/ChatImpl.cpp
    ../../Source/Application/ChatStorage.h
    ../../Source/Application/ChatStorageLocal.h
    ../../Source/Application/ChatStorageLocal.cpp
    ../../Source/Application/ChatConverter.cpp
    ../../Source/Common/ErrorSystem.h
    ../../Source/Common/ErrorSystem.cpp
    mock_services.cpp
    tst_storagelocal.cpp
)
target_include_directories(Test_StorageLocal PRIVATE ../../Source/Common)
target_link_libraries(Test_StorageLocal PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Sql Qt6::Test)
add_test(NAME Test_StorageLocal COMMAND Test_StorageLocal)

qt_add_executable(Test_Encryption
    ../../Source/Application/Encryption.h
    ../../Source/Application/Encryption.cpp
    tst_encryption.cpp
)
target_link_libraries(Test_Encryption PRIVATE Qt6::Core Qt6::Test OpenSSL::SSL OpenSSL::Crypto)
add_test(NAME Test_Encryption COMMAND Test_Encryption)

# Tests QML avec Qt Quick Test via qmltestrunner
find_program(QMLTESTRUNNER_EXECUTABLE qmltestrunner)
if (QMLTESTRUNNER_EXECUTABLE)
    add_test(NAME QmlTests COMMAND ${QMLTESTRUNNER_EXECUTABLE} 
            -input ../qml
            -import ../Source/Application/qml)
else()
    message(WARNING "qmltestrunner introuvable : les tests QML ne seront pas exécutés.")
endif()



