cmake_minimum_required(VERSION 3.21)

project(PartycjePudelkaCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(partycje_pudelka_cpp
  src/main.cpp
  src/MainWindow.cpp
  src/MainWindow.h
  src/RenderWidget.cpp
  src/RenderWidget.h
  src/PartitionSolver.cpp
  src/PartitionSolver.h
  src/Model.h
)

target_link_libraries(partycje_pudelka_cpp PRIVATE Qt6::Widgets)

target_compile_options(partycje_pudelka_cpp PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

set_target_properties(partycje_pudelka_cpp PROPERTIES
  WIN32_EXECUTABLE ON
)
