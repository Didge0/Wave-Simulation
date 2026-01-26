cmake_minimum_required(VERSION 3.20)

project(Wave_SIM VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ================= SDL3 =================
set(SDL3_DIR "C:/Library/SDL3")
find_package(SDL3 REQUIRED CONFIG)

# ================= Vector Library =================
file(GLOB_RECURSE SRCS_LIB src/Lib/*.c src/Lib/*.h)

add_library(Vector STATIC ${SRCS_LIB})

target_include_directories(Vector PUBLIC
    src/Lib
)

# ================= Executable =================
add_executable(${PROJECT_NAME}
    src/main.c
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        Vector
)
