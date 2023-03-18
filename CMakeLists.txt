cmake_minimum_required(VERSION 3.24)
project(ball-of-rivalry)

set(CMAKE_CXX_STANDARD 17)

set(SDL2_INCLUDE_DIR C:/LibSDL/VC/SDL2-2.26.3/include)
set(SDL2_LIB_DIR C:/LibSDL/VC/SDL2-2.26.3/lib/x64)

set(SDL2MIXER_INCLUDE_DIR "C:/LibSDL/VC/SDL2_mixer-2.6.3/include")
set(SDL2MIXER_LIB_DIR "C:/LibSDL/VC/SDL2_mixer-2.6.3/lib/x64")


include_directories(${SDL2_INCLUDE_DIR} ${SDL2MIXER_INCLUDE_DIR})
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIB_DIR}/SDL2.lib ${SDL2_LIB_DIR}/SDL2main.lib ${SDL2MIXER_LIB_DIR}/SDL2_mixer.lib)