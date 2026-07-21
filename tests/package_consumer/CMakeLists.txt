cmake_minimum_required(VERSION 3.20)
project(AxisPackageConsumer LANGUAGES CXX)

find_package(AxisEngine CONFIG REQUIRED)
add_executable(axis_package_consumer main.cpp)
target_link_libraries(axis_package_consumer PRIVATE Axis::Engine)
target_compile_features(axis_package_consumer PRIVATE cxx_std_20)
