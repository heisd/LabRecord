cmake_minimum_required(VERSION 3.10)
project(ImplicitConversion)
# 设置cpp版本
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
# 
include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(Implicit.exe src/Implicit.cpp)
add_executable(NoImplicit.exe src/NoImplicit.cpp)




