cmake_minimum_required(VERSION 3.20)
project(nowa-gra-2d LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# First external dependencies (Phase 4): OpenSSL for transport encryption
# and password hashing (scrypt), SQLite for account/character persistence.
# See docs/TECH_STACK.md for why — this was deliberately deferred until a
# real need existed, not added speculatively.
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

add_executable(server
  src/main.cpp
  src/net/server.cpp
  src/net/tls.cpp
  src/db/database.cpp
  src/auth/password.cpp
)

target_include_directories(server PRIVATE src ${SQLITE3_INCLUDE_DIRS})
target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto ${SQLITE3_LIBRARIES})
