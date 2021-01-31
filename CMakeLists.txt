cmake_minimum_required(VERSION 3.14)
project(BeautyVis)

option(GPROF_PROFILING "Turns on GPROF Profiler." OFF)


#INCLUDES/LINKER
set(AppPath "${PROJECT_SOURCE_DIR}/source/Application")
set(DependencyFolder "${PROJECT_SOURCE_DIR}/dependencies")

# Evil but who cares
file(GLOB_RECURSE AppDirectory "${AppPath}/States/*.cpp"
                               "${AppPath}/*.cpp"
                               "${DependencyFolder}/kiss_fft/*.c"
                               "${DependencyFolder}/miniaudio/miniaudio.c")
                               

list(APPEND CMAKE_CXX_FLAGS_DEBUG "-D_GLIBCXX_DEBUG")
list(APPEND CMAKE_MODULE_PATH cmake)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#STANDARD C/C++ STUFF
set(CMAKE_CXX_STANDARD 17)


#COMPILATION
add_executable(BeautyVis ${AppDirectory} "source/main.cpp")

if (GPROF_PROFILING)
    target_compile_options(BeautyVis PRIVATE -pg)
    target_link_options(BeautyVis PRIVATE -pg)       
endif()


target_include_directories(BeautyVis PRIVATE "${AppPath}" "${AppPath}/States" "${DependencyFolder}/kiss_fft" "${DependencyFolder}/miniaudio")

set(gcc_like_compilers "$<COMPILE_LANG_AND_ID:CXX,Clang,GNU,AppleClang>")

target_compile_options(BeautyVis PRIVATE 
                    "$<${gcc_like_compilers}:$<BUILD_INTERFACE:-Wall;-Wextra>>"
                    )


# For now only windows is officialy supported, but its trivial to support other ones
# just need find package raylib file
target_link_libraries(BeautyVis PUBLIC raylib winmm gdi32)
