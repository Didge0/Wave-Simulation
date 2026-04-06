cmake_minimum_required(VERSION 3.20)
project(Wave_SIM VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ================= SDL3 =================
set(SDL3_DIR "C:/Library/SDL3")
find_package(SDL3 REQUIRED CONFIG)

# ================= Internal Libraries =================
add_library(Vector STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/vector/vector.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/vector/vector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/vector/color.h
)
target_include_directories(Vector PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/vector
)

# ================= Third Party: Moteur3D =================
set(MOTEUR3D_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/moteur3d/include)
set(MOTEUR3D_LIB_FILE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/moteur3d/lib/Moteur.lib)

add_library(Matrice3D STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/moteur3d/src/Matrice/Matrice.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/moteur3d/src/Matrice/Matrice.h
)
target_include_directories(Matrice3D PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/moteur3d/src/Matrice
)

add_library(Moteur3D STATIC IMPORTED GLOBAL)
set_target_properties(Moteur3D PROPERTIES
    IMPORTED_LOCATION ${MOTEUR3D_LIB_FILE}
    INTERFACE_INCLUDE_DIRECTORIES ${MOTEUR3D_INCLUDE_DIR}
)

set(MENU_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/menu/menu.c)
set(MENU_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/menu.h)

# ================= Executable: 2D / Bench =================
add_executable(WAVE_SIM_2D
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/simu2D.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulation/waveSim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/render/sdl2d/SDLWaveSim.c
    ${MENU_SOURCE}
    ${MENU_HEADER}
)

target_include_directories(WAVE_SIM_2D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(WAVE_SIM_2D PRIVATE SDL3::SDL3 Vector)

# ================= Executable: 3D =================
add_executable(WAVE_SIM_3D
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/simu3D.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulation/waveSim.c
    ${MENU_SOURCE}
    ${MENU_HEADER}
)

target_include_directories(WAVE_SIM_3D PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(WAVE_SIM_3D PRIVATE SDL3::SDL3 Vector Matrice3D Moteur3D)
