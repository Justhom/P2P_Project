cmake_minimum_required(VERSION 3.16)
project(P2P-network LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Exécutable serveur
add_executable(server_sync src/server_sync.cpp)
add_executable(client_sync src/client_sync.cpp)

add_executable(server_async src/server_async.cpp)
add_executable(client_async src/client_async.cpp)

# 👉 1) Inclure Asio (standalone)
target_include_directories(server_sync PRIVATE ${CMAKE_SOURCE_DIR}/external/asio-master/include)
target_include_directories(client_sync PRIVATE ${CMAKE_SOURCE_DIR}/external/asio-master/include)

target_include_directories(server_async PRIVATE ${CMAKE_SOURCE_DIR}/external/asio-master/include)
target_include_directories(client_async PRIVATE ${CMAKE_SOURCE_DIR}/external/asio-master/include)

# 👉 2) Dire à Asio qu'on est en standalone (sans Boost)
target_compile_definitions(server_sync PRIVATE ASIO_STANDALONE)
target_compile_definitions(client_sync PRIVATE ASIO_STANDALONE)

target_compile_definitions(server_async PRIVATE ASIO_STANDALONE)
target_compile_definitions(client_async PRIVATE ASIO_STANDALONE)

# 👉 3) Avertissements utiles (déjà chez toi)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(server_sync PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(client_sync PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(server_async PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(client_async PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 👉 4) (Optionnel mais recommandé) lier les threads
find_package(Threads REQUIRED)
target_link_libraries(server_sync PRIVATE Threads::Threads)
target_link_libraries(client_sync PRIVATE Threads::Threads)

target_link_libraries(server_async PRIVATE Threads::Threads)
target_link_libraries(client_async PRIVATE Threads::Threads)