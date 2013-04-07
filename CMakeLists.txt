cmake_minimum_required (VERSION 2.6)

project (openvdb_softimage)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} e:/library/development/CMake/Modules)

find_package (Softimage)
find_package (boost)

if (Boost_FOUND)
 set (BOOST_INCLUDE_PATH ${Boost_INCLUDE_DIRS})
 set (BOOST_LIBRARIES ${Boost_LIBRARIES})
endif()

find_package (zlib)
set (ILMBASE_ROOT "C:/Program Files/ILMBase")
find_package (ilmbase REQUIRED)
find_package (tbb)

add_definitions(-DOPENVDB_STATIC -DNOMINMAX)

message (STATUS "---configuring openvdb_softimage plugin---")

set (INCLUDE_DIRS
 "c:/program files/openvdb/include"
 ${ILMBASE_INCLUDE_DIRECTORY}
 ${BOOST_INCLUDE_PATH}
 ${TBB_INCLUDE_DIR}
 ${ZLIB_INCLUDE_DIRS}
)

set (LINK_DIRS
 "c:/program files/openvdb/lib"
)

include_directories (${INCLUDE_DIRS})

link_directories (${LINK_DIRS})

set (SOURCES
 openvdb_softimage.cpp
)

set (HEADERS
)

set (LINK_LIBS
 openvdb
 ${ILMBASE_HALF_LIB}
 ${TBB_LIBRARIES}
 ${ZLIB_LIBRARIES}
)

add_softimage_plugin (openvdb_softimage ${SOURCES} ${HEADERS})

target_link_libraries (openvdb_softimage ${LINK_LIBS})