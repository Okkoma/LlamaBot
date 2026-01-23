set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

# Tests unitaires ErrorSystem
qt_add_executable(Test_ErrorSystem
    ${CMAKE_SOURCE_DIR}/Source/Common/ErrorSystem.h
    ${CMAKE_SOURCE_DIR}/Source/Common/ErrorSystem.cpp
    tst_errorsystem.cpp
)
target_include_directories(Test_ErrorSystem PRIVATE ${CMAKE_SOURCE_DIR}/Source/Common)
target_link_libraries(Test_ErrorSystem PRIVATE Qt6::Core Qt6::Test)
add_test(NAME Test_ErrorSystem COMMAND Test_ErrorSystem)
