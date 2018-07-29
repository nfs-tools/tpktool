cmake_minimum_required(VERSION 3.11)
project(tpktool)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.67.0 COMPONENTS system filesystem)

add_executable(tpktool main.cpp utils.hpp DDS.hpp TexturePack.hpp ChunkReader.hpp TexturePackReader.cpp TexturePackReader.hpp WorldReader.cpp WorldReader.hpp utils.cpp JDLZ.h JDLZ.cpp BitConverter.h BitConverter.cpp WorldWriter.cpp WorldWriter.hpp TexturePackWriter.cpp TexturePackWriter.hpp)

if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(tpktool LINK_PUBLIC ${Boost_LIBRARIES})
endif ()