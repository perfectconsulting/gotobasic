cmake_minimum_required(VERSION 3.24)

project(gotobasic VERSION 1.0.0)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/version.h
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled")
    set(SANITIZE_FLAGS "-fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_FLAGS}")
endif()

find_package(FLEX  REQUIRED)
find_package(BISON REQUIRED)

BISON_TARGET(
    BasicParser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/basic.y
    ${CMAKE_CURRENT_BINARY_DIR}/basic.tab.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/basic.tab.hpp
)

FLEX_TARGET(
    BasicLexer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/basic.l
    ${CMAKE_CURRENT_BINARY_DIR}/basic.lex.cpp
)

ADD_FLEX_BISON_DEPENDENCY(BasicLexer BasicParser)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/value.cpp
    src/error.cpp
    src/environment.cpp
    src/expression.cpp
    src/statement.cpp
    src/data_manager.cpp
    src/program_manager.cpp
    src/runtime_ip.cpp
    ${BISON_BasicParser_OUTPUTS}
    ${FLEX_BasicLexer_OUTPUTS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/src
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
