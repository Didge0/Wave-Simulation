cmake_minimum_required(VERSION 3.20)

project(Wave_SIM VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ================= SDL3 =================
set(SDL3_DIR "C:/Library/SDL3")
find_package(SDL3 REQUIRED CONFIG)

# ================= Vector Library =================
file(GLOB_RECURSE SRCS_LIB
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Lib/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Lib/*.h
)

add_library(Vector STATIC ${SRCS_LIB})

target_include_directories(Vector PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Lib
)

# ================= Project sources (excluding Lib) =================
file(GLOB_RECURSE SRCS_APP
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

# Retire tout ce qui est dans src/Lib (déjà dans la lib Vector)
list(FILTER SRCS_APP EXCLUDE REGEX ".*/src/Lib/.*")

# ================= Executable =================
add_executable(${PROJECT_NAME}
    ${SRCS_APP}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        Vector
)
