cmake_minimum_required(VERSION 3.0.0)
project(Server VERSION 0.1.0)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)

# Find the Boost libraries and headers
find_package(Boost REQUIRED COMPONENTS system)

# Add the Boost include directories
include_directories(${Boost_INCLUDE_DIRS})

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

get_filename_component(MAIN_PROJECT_DIR . ABSOLUTE)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MAIN_PROJECT_DIR}/bin )


add_executable(Server server.cpp)
add_executable(Client client.cpp ThreadPool/ThreadPool.cpp)
add_executable(ClientBoost Client_boost.cpp)

# Link to the pthread library
target_link_libraries(Server PUBLIC pthread)
target_link_libraries(ClientBoost ${Boost_LIBRARIES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
