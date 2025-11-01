cmake_minimum_required(VERSION 3.16)
project(hello-network LANGUAGES CXX)

# C++20 pour les features modernes utiles au P2P
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Binaire principal
add_executable(hello src/main.cpp)

# Sur macOS/Linux, active plus d’avertissements en Debug
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(hello PRIVATE -Wall -Wextra -Wpedantic)
endif()