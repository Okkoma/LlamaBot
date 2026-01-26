# Tests unitaires ErrorSystem
qt_add_executable(Test_ErrorSystem
    ../../Source/Common/ErrorSystem.h
    ../../Source/Common/ErrorSystem.cpp
    tst_errorsystem.cpp
)
target_include_directories(Test_ErrorSystem PRIVATE ../../Source/Common)
target_link_libraries(Test_ErrorSystem PRIVATE Qt6::Core Qt6::Test)
add_test(NAME Test_ErrorSystem COMMAND Test_ErrorSystem)
