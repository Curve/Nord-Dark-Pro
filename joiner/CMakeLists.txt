cmake_minimum_required(VERSION 3.1)
project(joiner VERSION 0.1 DESCRIPTION "Joins Nord Pro and One Dark Pro")

file(GLOB src
    "main.cpp"
)

add_subdirectory(vivid)
add_executable(${PROJECT_NAME} ${src})
target_link_libraries(${PROJECT_NAME} vivid)
target_include_directories(${PROJECT_NAME} PRIVATE "json/single_include")
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Werror -Wextra -pedantic)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
set_target_properties(${PROJECT_NAME}   PROPERTIES 
                                        CXX_STANDARD 17
                                        CXX_EXTENSIONS OFF
                                        CXX_STANDARD_REQUIRED ON)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES PROJECT_NAME ${PROJECT_NAME})