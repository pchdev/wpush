cmake_minimum_required(VERSION 3.5)

project(wpush-cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp enums.hpp
    wjack.hpp wmidi.hpp wpush.hpp
    wjack.cpp wmidi.cpp wpush.cpp)

target_link_libraries(${PROJECT_NAME} -ljack)
