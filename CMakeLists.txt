cmake_minimum_required(VERSION 3.15)
project(CustomInterpreter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(interpreter
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/Environment.cpp
    src/Resolver.cpp
    src/Interpreter.cpp
)

target_include_directories(interpreter PRIVATE ${CMAKE_SOURCE_DIR}/src)

# MSVC 경고 레벨 설정
if(MSVC)
    target_compile_options(interpreter PRIVATE /W4)
else()
    target_compile_options(interpreter PRIVATE -Wall -Wextra)
endif()
